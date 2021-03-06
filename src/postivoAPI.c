/*
 * Postivo API is aplication create for handling the web service "Postvio".
 * API creates via gSOAP and works with gSOAPSSL.
 * In each function is used prefix "post". It shortcut of postivo.
 */

#include "soap/Postivo_x002eplBinding.nsmap"

#include "postivoAPI.h"
#include "documentFile.h"//used in: postDispatch
#include "recipient.h"
#include "shipments.h"//used in: postGetDispatchStatus
#include "senders.h"
#include <string.h>//strcmp

/// <global instances>
struct soap * g_soap;
char * s_endAction = NULL;//prepare to using servers
char * s_soapAction = NULL;//prepare to using servers
/// </global instances>

/// <summary>
/// Prints the error code.
/// Created only for clean code.
/// </summary>
/// <param name = "errorCode"> Code of error. </param>
void printErrorMessage(const char * errorCode)
{
  printf("---------------------------------------------------\n");//for transparency
  printf("The request has been sent but something went wrong.\n");
  printf("[Error]: %s\n",errorCode);
}

/// <summary>
/// Prepare the "g_soap", to work with the Postivo API.
/// </summary>
void initPostivoAPI()
{
  g_soap = soap_new1(SOAP_C_UTFSTRING | SOAP_XML_INDENT | SOAP_ENC_SSL | SOAP_XML_TREE);//create soap
  soap_ssl_init();//init SSL
  soap_init(g_soap);//init soap lib

  soap_set_imode(g_soap, SOAP_C_UTFSTRING);//input
  soap_set_omode(g_soap, SOAP_C_UTFSTRING);//output

  if (soap_ssl_client_context(g_soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL))//init SSL with no AUTHENTICATION
  {
    soap_print_fault(g_soap, stderr);//print error
    exit(1);//exit because without ssl, cannot connect to HTTPS
  }
}

/// <summary>
/// Releasing the memory.
/// </summary>
void closePostivoAPI()
{
  if(s_endAction != NULL)//if endAction is not empty 
    free(s_endAction);//release memory
  soap_end(g_soap);     //delete deserialized temp data
  soap_destroy(g_soap); //delete deserialized data
  soap_free(g_soap);    //free context
}

/// <summary>
/// Creates a request to send a letter.
/// Currently sents to webservice a only 1 file to only a 1 recipient.
/// In the future, will be possible to send to a more than 1 recipient and more than 1 file.
/// When gets any error, returns it code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postDispatch(char * login, char * password)
{
  struct ns1__DocumentFile m_file;
  char * s_filePath = NULL;
  char * s_fileTitle = NULL;
  inputFilePath(&s_filePath,&s_fileTitle);//prepare file
  loadDocumentFile(s_filePath, s_fileTitle, &m_file);//loading and encrypting a file

  struct ArrayOfDocumentFiles m_arrayDocuments;
  m_arrayDocuments.__ptr = calloc(1, sizeof(struct ns1__DocumentFile*));//allocating the memory to only file
  m_arrayDocuments.__size = 1;//set size the array to only one
  m_arrayDocuments.__ptr[0] = &m_file;

  struct ns1__Recipient m_recipient;
  createRecipient(&m_recipient);//create a recipient
  
  struct ArrayOfRecipients m_arrayRecipients;
  m_arrayRecipients.__ptr = calloc(1, sizeof(struct ns1__Recipient*));//allocating the memory to only a one recipient
  m_arrayRecipients.__size = 1;//set size the array to only one
  m_arrayRecipients.__ptr[0] = &m_recipient;

  struct ns2__dispatchResponse m_dispatchStatus;
  if(soap_call_ns2__dispatch(g_soap, s_endAction, s_soapAction, login, password, "1", &m_arrayDocuments, &m_arrayRecipients, 0, &m_dispatchStatus) == SOAP_OK)//try execute the soap method
  {
    if(!strcmp(m_dispatchStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull\n");
      printf("The request has been sent.\n");
      printfShipments(m_dispatchStatus.return_->shipments);
    }
    else
      printErrorMessage(m_dispatchStatus.return_->result_USCOREdescription);
  }
  else
    soap_print_fault(g_soap, stderr);//print error

  if(s_filePath != NULL)
    free(s_filePath);
  if(s_fileTitle != NULL)
    free(s_fileTitle);

  freeRecipient(&m_recipient);//release a recipient when parameteres != NULL
  free(m_arrayRecipients.__ptr);
  free(m_arrayDocuments.__ptr);
  freeDocumentFile(&m_file);
}

/// <summary>
/// Receives a created request to send a letter.
/// Currently receives from webservice a only 1 file to only a 1 recipient.
/// In the future, will be possible to receives to a more than 1 recipient and more than a 1 file.
/// When gets any error, returns it code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postGetDispatchStatus(char * login, char * password)
{
  struct ArrayOfDispatchIds m_arrayDispatchIDs;
  m_arrayDispatchIDs.__ptr = calloc(1,sizeof(char *));
  m_arrayDispatchIDs.__size = 1;
  getShipmentID(&m_arrayDispatchIDs.__ptr[0]);


  struct ns2__getDispatchStatusResponse m_dispatchStatus;
  if(soap_call_ns2__getDispatchStatus(g_soap, s_endAction, s_soapAction, login, password, &m_arrayDispatchIDs, &m_dispatchStatus) == SOAP_OK)
  {
    if(!strcmp(m_dispatchStatus.return_->result, "OK"))//if is no error
    {
      printf("---------------------------\n");//for transparency
      printf("Successfull\n");
      printf("The status has been received.\n");
      printf("Result:\n");
      printfShipments(m_dispatchStatus.return_->shipments);
    }
    else
      printErrorMessage(m_dispatchStatus.return_->result_USCOREdescription);
  }
  else
    soap_print_fault(g_soap, stderr);//print error
}

/// <summary>
/// Connects to webservice and gets the default account info.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postGetBalance(char * login, char * password)
{
  struct ns2__getBalanceResponse m_balanceStatus;//object which is returning in the SOAP function

  if(soap_call_ns2__getBalance(g_soap, s_endAction, s_soapAction, login, password, &m_balanceStatus) == SOAP_OK)//try execute the soap method
  {
    if(!strcmp(m_balanceStatus.return_->result, "OK"))//if is no error
    {
      printf("------------------------\n");//for transparency
      printf("Account balance:%0.2f \n",(*m_balanceStatus.return_->balance));
      printf("Account limit:%0.2f\n",(*m_balanceStatus.return_->limit));
      if((*m_balanceStatus.return_->postpaid) == 0)
        printf("Account type: PREPAID\n");
      else
        printf("Account type: POST-PAID\n");
    }
    else
      printErrorMessage(m_balanceStatus.return_->result_USCOREdescription);
  }
  else
    soap_print_fault(g_soap, stderr);//print error
}

/// <summary>
/// Connects to webserivce and gets the price of dispatch.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postGetPrice(char * login, char * password)
{
  struct ns1__DocumentFile m_file;
  char * s_filePath = NULL;
  char * s_fileTitle = NULL;
  inputFilePath(&s_filePath,&s_fileTitle);//prepare file
  loadDocumentFile(s_filePath, s_fileTitle, &m_file);//loading and encrypting a file

  struct ArrayOfDocumentFiles m_arrayDocuments;
  m_arrayDocuments.__ptr = calloc(1,sizeof(struct ns1__DocumentFile*));//allocating the memory to only file
  m_arrayDocuments.__size = 1;//set size the array to only one
  m_arrayDocuments.__ptr[0] = &m_file;

  struct ns1__Recipient  m_recipient;
  createRecipient(&m_recipient);//create a recipient

  struct ArrayOfRecipients m_arrayRecipients;
  m_arrayRecipients.__ptr = calloc(1,sizeof(struct ns1__Recipient*));//allocating the memory to only a one recipient
  m_arrayRecipients.__size = 1;//set size the array to only one
  m_arrayRecipients.__ptr[0] = &m_recipient;


  struct ns2__getPriceResponse m_priceStatus;
  if(soap_call_ns2__getPrice(g_soap, s_endAction, s_soapAction, login, password, "1", &m_arrayDocuments, &m_arrayRecipients, 0, &m_priceStatus) == SOAP_OK)
  {
    if(!strcmp(m_priceStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull\n");
      printf("The status has been received.\n");
      printfShipmentsPrices(m_priceStatus.return_->shipments_USCOREprice);
    }
    else
      printErrorMessage(m_priceStatus.return_->result_USCOREdescription);
  }
  else
    soap_print_fault(g_soap, stderr);//print error

  if(s_filePath != NULL)
    free(s_filePath);
  if(s_fileTitle != NULL)
    free(s_fileTitle);

  freeRecipient(&m_recipient);//release a recipient when parameteres != NULL
  free(m_arrayRecipients.__ptr);
  free(m_arrayDocuments.__ptr);
  freeDocumentFile(&m_file);
}

/// <summary>
/// Connects to service and gets config profiles.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postGetConfigProfiles(char * login, char * password)
{
  struct ns2__getConfigProfilesResponse m_configProfilesStatus;
  if(soap_call_ns2__getConfigProfiles(g_soap, s_endAction, s_soapAction, login, password, "1", &m_configProfilesStatus) == SOAP_OK)
  {
    if(!strcmp(m_configProfilesStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull\n");
      printf("The status has been received.\n");
    }
    else
      printErrorMessage(m_configProfilesStatus.return_->result_USCOREdescription);
  }
  else
    soap_print_fault(g_soap, stderr);
}

/// <summary>
/// Connects to service and gets all senders.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postGetSenders(char * login, char * password)
{
  struct ns2__getSendersResponse m_sendersStatus;
  if(soap_call_ns2__getSenders(g_soap, s_endAction, s_soapAction, login, password, &m_sendersStatus) == SOAP_OK)
  {
    if(!strcmp(m_sendersStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull\n");
      printf("The status has been received.\n");
      printf("%d \n", m_sendersStatus.return_->senders->__size);
      printfSenders(m_sendersStatus.return_->senders);
    }
    else
      printErrorMessage(m_sendersStatus.return_->result_USCOREdescription);
  }
  else
    soap_print_fault(g_soap, stderr);
}

/// <summary>
/// Generates a certificate which is confirmation 
/// of sending and receiving parcels.
/// Also writes a received certificate file to disk.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postGetCertificate(char * login, char * password)
{
  char * dispatchID;
  getShipmentID(&dispatchID);

  int i_confirmationType = 0;  
  printf("1)Confirmation of sending the parcel \n");
  printf("2)Confirmation of receipt of the parcel by the addressee \n");
  printf("Enter the type of confirmation:");
  scanf("%d", &i_confirmationType);

  struct ns2__getCertificateResponse m_certificateStatus;
  if(soap_call_ns2__getCertificate(g_soap, s_endAction, s_soapAction, login, password, dispatchID, i_confirmationType, &m_certificateStatus))
  {
    if(!strcmp(m_certificateStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      if(m_certificateStatus.return_->cert_USCOREavailable == 1)//successfull 
      {
        printf("Successfull \n");
        printf("Confirmation has been generated. \n");

        char * s_certificateLocation = malloc(sizeof(char) * 255);//allocate place in memory
        printf("Enter the place where the certification will be generate:");
        scanf("%s", s_certificateLocation);//get location to save a file

        saveDocumentFile(s_certificateLocation, m_certificateStatus.return_->cert_USCOREcontent);//decode and save document 

        free(s_certificateLocation);//release memory
      }
      else//error
      {
        printf("Error \n");
        printf("Confirmation has not been generated");
      }
    }
    else
      printErrorMessage(m_certificateStatus.return_->result_USCOREdescription);//connection error
  }
  else
    soap_print_fault(g_soap, stderr);
}
/// <summary>
/// Adds sender to the account contacts.
/// Using this function, user accepting the Postivo Rules(Gets 2.00 zł netto from user account).
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postAddSender(char * login, char * password)
{
  struct ns1__SenderData m_senderData;
  createSender(&m_senderData);

  struct ns2__addSenderResponse m_senderStatus;
  if(soap_call_ns2__addSender(g_soap, s_endAction, s_soapAction, login, password, &m_senderData, 1, &m_senderStatus) == SOAP_OK)
  {
    if(!strcmp(m_senderStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull \n");
      printf("Sender has been added. \n");
    }
    else
      printErrorMessage(m_senderStatus.return_->result_USCOREdescription);//connection error
  }
  else
    soap_print_fault(g_soap, stderr);
  
  freeSender(&m_senderData);//release sender
}

/// <summary>
/// Verifies a sender via technical code. 
/// To get technical code, must write to Postivo.
/// If Postivo accept sender, sends this code via post.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postVerifySender(char * login, char * password)
{
  char * s_verifyCode = malloc(sizeof(char) * 255);
  int i_senderID;

  printf("Enter the sender ID:");
  scanf("%d", &i_senderID);
  printf("Enter the verify code:");
  scanf("%s", s_verifyCode);

  struct ns2__verifySenderResponse m_verifyStatus;
  if(soap_call_ns2__verifySender(g_soap, s_endAction, s_soapAction, login, password, i_senderID, s_verifyCode, &m_verifyStatus) == SOAP_OK)
  {
    if(!strcmp(m_verifyStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull \n");
      printf("Sender has been verified. \n");
    }
    else
      printErrorMessage(m_verifyStatus.return_->result_USCOREdescription);//connection error
  }
  else
    soap_print_fault(g_soap, stderr);
  
  free(s_verifyCode);
}

/// <summary>
/// Removes a sender from user contacts.
/// User must inputes a login and a password.
/// When gets any error, returns code.
/// List of error codes: https://postivo.pl/docs/Dokumentacja_API_Postivo.pdf
/// </summary>
/// <param name = "login"> Login </param>
/// <param name = "password"> Password </param>
void postRemoveSender(char * login, char * password)
{
  int i_senderID;
  printf("Enter the sender ID to remove:");
  scanf("%d", &i_senderID);

  struct ns2__removeSenderResponse m_removeStatus;
  if(soap_call_ns2__removeSender(g_soap, s_endAction, s_soapAction, login, password, i_senderID, &m_removeStatus) == SOAP_OK)
  {
    if(!strcmp(m_removeStatus.return_->result, "OK"))//if is no error
    {
      printf("--------------------------\n");//for transparency
      printf("Successfull \n");
      printf("Sender has been verified. \n");
    }
    else
      printErrorMessage(m_removeStatus.return_->result_USCOREdescription);//connection error
  }
  else
    soap_print_fault(g_soap, stderr);
}

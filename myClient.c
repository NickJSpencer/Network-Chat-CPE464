/******************************************************************************
* tcp_client.c
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "networks.h"
#include "linkedList.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define MAX_INPUT_BUF 1400
#define xstr(a) str(a)
#define str(a) #a

void retrieveInputOrPackets(int maxSocket);

void sendToServer(int socketNum);
void receiveFromServer(int socketNum);
void sendHandleToServer(int socketNum, char *handle);

int parseBlockHandle(int socketNum, char *msgBuf);
int parseUnblockHandle(int socketNum, char *msgBuf);

int parseInput(int socketNum, char msgBuf[MAXBUF]);
int parseMessage(int socketNum, char msgBuf[MAXBUF]);
void splitAndSendMsgToServer(int socketNum, char *msg, char **destinations, int numDests);
int sendMsgToServer(int socketNum, char *msg, char **destinations, int numDests);

void handlePacket(Header header, int socketNum, char *buf);

void receiveHandleError(int socketNum, char *bufPtr);
void receiveMsg(int socketNum, char *buf);

void receiveHandleCount(int socketNum, char *buf);
void receiveHandle(int socketNum, char *buf);

void disconnect();

void checkArgs(int argc, char * argv[]);

int serverSocket = 0;
int clientSocket = 0;

/* Client program that connects to a server and talks to other clients */
int main(int argc, char * argv[])
{   
   checkArgs(argc, argv);
   
	/* set up the TCP Client socket  */
	serverSocket = tcpClientSetup(argv[1], argv[2], argv[3], DEBUG_FLAG);
   sendHandleToServer(serverSocket, argv[1]);
   clientHandle = argv[1];
   
   /* Begin retrieving and sending packets to and from the server */
   int maxSocket = max(serverSocket, clientSocket);
   retrieveInputOrPackets(maxSocket);
      
	return SUCCESS;
}

void retrieveInputOrPackets(int maxSocket)
{
   fd_set sockets;

   /* Loop forever; other parts of the code will cleanly terminate.
    * This function will never return. */
   while(1)
   {
      /* Clear and set the server and client sockets */
      FD_ZERO(&sockets);
      FD_SET(serverSocket, &sockets);
      FD_SET(clientSocket, &sockets);

      /* Wait for input from the terminal or a packet from the server */
      if (select(maxSocket + 1, &sockets, NULL, NULL, NULL) < 0)
      {
         perror("select");
         exit(FAILURE);
      }

      /* receive packet from server */
      if (FD_ISSET(serverSocket, &sockets))
      {
         receiveFromServer(serverSocket);
      }

      /* parse input from terminal */
      if (FD_ISSET(clientSocket, &sockets))
      {
         sendToServer(serverSocket);
      }
   }
}

void sendToServer(int socketNum)
{
   char msgBuf[MAX_INPUT_BUF];
   char *msgPtr = msgBuf;
   char c;

   /* Get up to 1400 chars from the terminal */
	while ((c = getchar()) != '\n' && c != EOF)
   {
      if (msgPtr - msgBuf < 1400)
      {
         *msgPtr = c;
      }
      msgPtr += sizeof(char);
      
   }
   *msgPtr = '\0';
   
   /* Properly deal with the input */
   parseInput(socketNum, msgBuf);
}

/* Send this client's handle to the server as its first sent packet */
void sendHandleToServer(int socketNum, char *handle)
{
   int sent = 0;
   char packet[MAXBUF];
   char *packetPtr = packet;
   uint8_t handleLen = strlen(handle);
   
   Header header = createHeader(FLAG_1, sizeof(header) + sizeof(handleLen) + handleLen);
   
   memcpy(packetPtr, &header, sizeof(Header));
   packetPtr += sizeof(Header);
   
   memcpy(packetPtr, &handleLen, sizeof(uint8_t));
   packetPtr += sizeof(uint8_t);
   
   memcpy(packetPtr, handle, handleLen);
   packetPtr += handleLen;
      
   sent = send(socketNum, packet, packetPtr - packet, 0);
	if (sent < 0)
	{
		perror("send");
		exit(FAILURE);
	}
}

int parseInput(int socketNum, char msgBuf[MAXBUF])
{
   /* Make sure the first char is '%' */
   if (msgBuf[0] != '%')
   {
      printf("Invalid command\n");
      printf("$: ");
      fflush(stdout);
      return FAILURE;
   }
   
   /* Command is a message that will be sent to the server */
   if (msgBuf[1] == 'M' || msgBuf[1] == 'm')
   {
      parseMessage(socketNum, msgBuf);
      printf("$: "); fflush(stdout);
   }
   
   /* Command is to block a handle */
   else if (msgBuf[1] == 'B' || msgBuf[1] == 'b')
   {
      parseBlockHandle(socketNum, msgBuf);
      printf("$: "); fflush(stdout);
   }
   
   /* Command is to unblock a handle */
   else if (msgBuf[1] == 'U' || msgBuf[1] == 'u')
   {
      parseUnblockHandle(socketNum, msgBuf);
      printf("$: "); fflush(stdout);
   }
   
   /* Command is to list out all the handles connected to the server */
   else if (msgBuf[1] == 'L' || msgBuf[1] == 'l')
   {
      sendHeader(socketNum, FLAG_10);
   }
   
   /* Command is to disconnect from the server and shutdown */
   else if (msgBuf[1] == 'E' || msgBuf[1] == 'e')
   {
      sendHeader(socketNum, FLAG_8);
   }
   
   /* Otherwise, invalid command */
   else
   {
      printf("Invalid command\n");
      printf("$: ");
      fflush(stdout);
      return FAILURE;
   }
   
   return SUCCESS;
}

/* Block a handle */
int parseBlockHandle(int socketNum, char *msgBuf)
{
   strtok(msgBuf, " ");
   char *token = strtok(NULL, " ");
   Node *node;
   
   /* If there is a handle to try blocking */
   if(token != NULL)
   {
      node = find(token);
      if (node != NULL)
      {
         printf("Block failed, handle %s is already blocked.\n", token);
         return FAILURE;
      }
      
      /* Block this handle */
      add(token, 0);
   }
   
   /* Print out all blocked handles */
   node = getHead();
   printf("Blocked: ");
   printf("%s", node->handle);
   node = node->next;
   while(node != NULL)
   {
      printf(", %s", node->handle);
      node = node->next;
   }
   printf("\n");
   
   return SUCCESS;
}

/* unblock a handle */
int parseUnblockHandle(int socketNum, char *msgBuf)
{
   strtok(msgBuf, " ");
   char *token = strtok(NULL, " ");
   
   /* No handle is provided, print an error statement */
   if (token == NULL)
   {
      printf("Unblock failed, no handle provided.\n");
      return FAILURE;
   }
   
   /* If the handle is not block, print and error statement */
   Node *node = find(token);
   if (node == NULL)
   {
      printf("Unblock failed, handle %s is not blocked.\n", token);
      return FAILURE;
   }
   
   /* Delete the handle from the blocked handles LL */
   if (delByHandle(token))
   {
      printf("delete failed for handle: %s\n", token);
      return FAILURE;
   }
   
   printf("Handle %s unblocked.\n", token);
   
   return SUCCESS;
}

int parseMessage(int socketNum, char msgBuf[MAXBUF])
{
   char tempBuf[MAXBUF];
   memcpy(tempBuf, msgBuf, MAXBUF);
   
   strtok(tempBuf, " ");
   char *token = strtok(NULL, " ");
   if (token == NULL)
   {
      printf("Too few handles entered.\n");
      return FAILURE;
   }
   char *temp;
   
   /* Get number of destinations for this message */
   int numDests = strtol(token, &temp, 10);
   
   /* If number is not provided, there will be 1 destination */
   if (numDests == 0)
   {
      numDests = 1;
      strtok(msgBuf, " ");
   }
   
   /* If there is not a number 1-9, print and error statement */
   else if (numDests <= 0 || numDests > 10)
   {
      fprintf(stderr, "Invalid number of destinations\n");
      return FAILURE;
   }
   
   /* Get all destinations */
   char *destinations[numDests];
   int i;
   for (i = 0; i < numDests; i++)
   {
      token = strtok(NULL, " ");
      if (token == NULL)
      {
         printf("Too few handles entered.\n");
         return FAILURE;
      }
      destinations[i] = token;
   }
   
   token = strtok(NULL, "");
   
   /* If there is no message, send a blank message */
   if (token == NULL)
   {
      token = "";
   }
   
   /* Send message */
   splitAndSendMsgToServer(socketNum, token, destinations, numDests);
   
   return SUCCESS;
}

/* Split message into chunks of 200 chars and send to server */
void splitAndSendMsgToServer(int socketNum, char *msg, char **destinations, int numDests)
{
   char *msgPtr = msg;
   
   /* If message is over 200 chars, take first 200 and pass the rest of the message back into this function */
   if (strlen(msg) > 200)
   {
      char smallMsg[MAXBUF];
      memcpy(smallMsg, msg, 200);
      smallMsg[200] = '\0';
      
      sendMsgToServer(socketNum, smallMsg, destinations, numDests);
      
      msgPtr += 200;
      
      splitAndSendMsgToServer(socketNum, msgPtr, destinations, numDests);
   }
   
   /* If message is less than 200 chars, send it! */
   else
   {
      sendMsgToServer(socketNum, msg, destinations, numDests);
   }
}

/* Send a message 200 chars or less to the server in a packet created in this function */
int sendMsgToServer(int socketNum, char *msg, char **destinations, int numDests)
{
   char buf[MAXBUF];
   char *bufPtr = buf;
   
   bufPtr += sizeof(Header);
   
   uint8_t clientHandleLen = strlen(clientHandle);
   memcpy(bufPtr, &clientHandleLen, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   memcpy(bufPtr, clientHandle, clientHandleLen);
   bufPtr += clientHandleLen;
   
   uint8_t destLen = numDests;
   memcpy(bufPtr, &destLen, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   int i;
   for (i = 0; i < destLen; i++)
   {
      uint8_t len = strlen(destinations[i]);
      memcpy(bufPtr, &len, sizeof(uint8_t));
      bufPtr += sizeof(uint8_t);
      
      memcpy(bufPtr, destinations[i], len);
      bufPtr += len;
   }
   
   memcpy(bufPtr, msg, strlen(msg) + 1);
   bufPtr += strlen(msg) + 1;
   
   uint16_t totalLen = bufPtr - buf;
   Header header = createHeader(FLAG_5, totalLen);
   memcpy(buf, &header, sizeof(Header));
   
   safeSend(socketNum, buf, (size_t) totalLen, 0);
      
   return SUCCESS;
}

/* Recieve a packet from the server and deal with it */
void receiveFromServer(int socketNum)
{
   char buf[MAXBUF];
   char *bufPtr = buf;
   Header header;
	int messageLen = 0;
	
	/* Get the data for the header from the server */
	if ((messageLen = recv(socketNum, buf, sizeof(Header), MSG_WAITALL)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
   
   /* If the length is 0, the server is terminated, so this client should terminate too */
   if (messageLen == 0)
   {
      del(socketNum);
      close(socketNum);
      printf("\nServer Terminated\n");
      exit(SUCCESS);
   }
      
   memcpy(&header, bufPtr, sizeof(Header));
   
   header.length = ntohs(header.length);
   bufPtr += sizeof(Header);
   
   /* If the packet is bigger than just the header, grab the rest */
   if (header.length > sizeof(Header))
   {
      if ((messageLen = recv(socketNum, bufPtr, header.length - sizeof(Header), MSG_WAITALL)) < 0)
      {
         perror("recv");
         exit(FAILURE);
      }
   }
   
   /* Parse packet */
   handlePacket(header, socketNum, buf);
}

/* Parse packet and determine what should be done with its contents */
void handlePacket(Header header, int socketNum, char *buf)
{
   switch (header.flags)
   {
      case (FLAG_2): /* Server accepts this client */
      {
         printf("$: "); fflush(stdout);
         break;
      }
      case (FLAG_3): /* Server already has a client with this handle. Terminate this client */
      {
         fprintf(stderr, "Handle already in use: %s\n", clientHandle);
         close(serverSocket);
         close(clientSocket);
         exit(FAILURE);
      }
      case (FLAG_5): /* Receive a message from another client (through the server, of course!) */
      {
         receiveMsg(socketNum, buf);
         break;
      }
      case (FLAG_7): /* a destination handle does not exist on the server from a previously sent message */
      {
         receiveHandleError(socketNum, buf);
         printf("$: "); fflush(stdout);
         break;
      }
      case (FLAG_9): /* Server accepts this client's disconnection. Terminate this client */
      {
         disconnect();
         break;
      }
      case (FLAG_11): /* Handle names are incoming! This packet says how many */
      {
         receiveHandleCount(socketNum, buf);
         break;
      }
      case (FLAG_12): /* A handle name is in this packet to display */
      {
         receiveHandle(socketNum, buf);
         break;
      }
      case (FLAG_13): /* No more handle names are coming. Client can resume normal use */
      {
         printf("$: "); fflush(stdout);
         break;
      }
      default:
      {
         break;
      }
   }
}

/* A handle does not exist from a previous message sent */
void receiveHandleError(int socketNum, char *bufPtr)
{
   /* Parse packet to get the handle name */
   Header header;
   memcpy(&header, bufPtr, sizeof(Header));
   bufPtr += sizeof(Header);
   
   uint8_t handleLen;
   memcpy(&handleLen, bufPtr, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   /* A handle can only be 100 chars or less */
   char handle[100];
   memcpy(handle, bufPtr, handleLen);
   handle[handleLen] = '\0';
   bufPtr += handleLen;
   
   /* Print and error statement with the name of the handle that does not exist on the server */
   printf("\nClient with handle %s does not exist.\n", handle);
}

/* Receive a message from the server from another client */
void receiveMsg(int socketNum, char *buf)
{
   /* Parse the message packet to get the source handle and its message */
   uint8_t sourceHandleLen;
   
   buf += sizeof(Header);
   
   memcpy(&sourceHandleLen, buf, sizeof(uint8_t));
   buf += sizeof(uint8_t);
   
   char sourceHandle[sourceHandleLen + 1];
   memcpy(sourceHandle, buf, sourceHandleLen);
   sourceHandle[sourceHandleLen] = '\0';
   buf += sourceHandleLen;
      
   if (find(sourceHandle) != NULL)
   {
      return;
   }
   
   uint8_t numHandles;
   memcpy(&numHandles, buf, sizeof(uint8_t));
   buf += sizeof(uint8_t);
      
   int i;
   for (i = 0; i < numHandles; i++)
   {
      uint8_t len;
      memcpy(&len, buf, sizeof(uint8_t));
      buf += sizeof(uint8_t) + len;
   }
   
   /* Display source handle and its message */
   printf("\n%s: %s\n", sourceHandle, buf);
   printf("$: "); fflush(stdout);
}

/* Receive the number of handles that are about to come in */
void receiveHandleCount(int socketNum, char *buf)
{
   /* Parse packet to get the number of clients on the server */
   uint32_t handleCount;
   
   buf += sizeof(Header);
   
   memcpy(&handleCount, buf, sizeof(uint32_t));
   handleCount = ntohl(handleCount);
   
   /* Display how many clients there are */
   printf("Number of clients: %d\n", handleCount);
}

/* Receive a name of a handle from the server */
void receiveHandle(int socketNum, char *buf)
{
   /* Parse the packet to get the name of the handle of a client on the server */
   uint8_t handleLen;
   char handle[MAXBUF];
   buf += sizeof(Header);

   memcpy(&handleLen, buf, sizeof(uint8_t));
   buf += sizeof(uint8_t);
   
   memcpy(handle, buf, handleLen);
   handle[handleLen] = '\0';
   
   /* Print the handle of the client */
   printf("  %s\n", handle);
}

/* Shutdown properly */
void disconnect()
{
   Node *node = getHead();
   Node *prev = node;
   
   /* Free all the nodes that contain the blocked handle information */
   while(node != NULL)
   {
      node = node->next;
      
      free(prev->handle);
      free(prev);
      
      prev = node;
   }
   
   /* Close the sockets */
   close(serverSocket);
   close(clientSocket);
   
   exit(SUCCESS);
}

/* Check to make sure the command line args are valid */
void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle host-name port-number \n", argv[0]);
		exit(FAILURE);
	}
   
   /* Handle name cannot be 100 chars. 99 chars is used as a precaution */
   if (strlen(argv[1]) > 99)
   {
      fprintf(stderr, "Invalid handle, handle longer than 100 characters: %s\n", argv[1]);
      exit(FAILURE);
   }
   
   /* Handle cannot start with a number */
   if (strtol(argv[1], NULL, 10))
   {
      fprintf(stderr, "Invalid handle, handle starts with a number.\n");
      exit(FAILURE);
   }
}

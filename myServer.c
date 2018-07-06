/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
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

#include "networks.h"
#include "linkedList.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void retrievePackets(int serverSocket);

void recvFromClient(int clientSocket);
void handlePacket(Header header, int clientSocket, char *buf);
void receiveHandle(Header header, char* bufPtr, int socketNum);
void sendHandleAlreadyExists(int socketNum);
void sendConfirmHandle(int socketNum);
void transmitMsg(Header header, char *buf, int clientSocket);
void sendHendleErrorMsg(int clientSocket, char *handle);

void transmitHandles(int clientSocket);
void transmitNumHandles(int clientSocket);
void transmitHandle(int clientSocket, char *handle);

void disconnectClient(int clientSocket);

int checkArgs(int argc, char *argv[]);

/* Server program that supports clients to connect and talk to each other */
int main(int argc, char *argv[])
{
	int serverSocket = 0;
	int portNumber = 0;
	
   /* Set up the port and server socket */
	portNumber = checkArgs(argc, argv);
	serverSocket = tcpServerSetup(portNumber);

   /* Begin retrieving packets */
   retrievePackets(serverSocket);
	
	return SUCCESS;
}

/* Retrieves packets forever. Only a CTRL-C will shut down the server */
void retrievePackets(int serverSocket)
{
   fd_set sockets;
   Node *node;
   int maxSocket = 3;
   
   while(1)
   {
      /* Reset all the sockets */
      FD_ZERO(&sockets);
      FD_SET(serverSocket, &sockets);
      
      /* Which includes all the sockets for each connected client */
      node = getHead();
      while (node != NULL)
      {
         maxSocket = max(maxSocket, node->socket);
         FD_SET(node->socket, &sockets);
         node = node->next;
      }
      
      /* Wait for packets to arrive */
      if (select(maxSocket + 1, &sockets, NULL, NULL, NULL) < 0)
      {
         perror("select");
         exit(FAILURE);
      }
      
      /* A new client wants to connect! */
      if (FD_ISSET(serverSocket, &sockets))
      {
         tcpAccept(serverSocket, DEBUG_FLAG);
      }

      /* Check to see which clients sent the packet[s] */
      node = getHead();
      Node *next;
      while (node != NULL)
      {
         next = node->next;
         if (FD_ISSET(node->socket, &sockets))
         {
            recvFromClient(node->socket);
            FD_CLR(node->socket, &sockets);
         }
         node = next;
      }
   }
}

/* Receive and deal with an incoming packet */
void recvFromClient(int clientSocket)
{
	char buf[MAXBUF];
   char *bufPtr = buf;
   Header header;
	int messageLen = 0;
	
	/* Get the header from the incoming packet */
	if ((messageLen = recv(clientSocket, buf, sizeof(Header), MSG_WAITALL)) < 0)
	{
		perror("recv call");
		exit(-1);
	}
   
   /* If there is no packet, this client disconnected, so it should be removed from the list of clients */
   if (messageLen == 0)
   {
      del(clientSocket);
      close(clientSocket);
      return;
   }
         
   memcpy(&header, bufPtr, sizeof(Header));
   
   bufPtr += sizeof(Header);
   
   /* Grab the rest of the packet if it contains more than just the header */
   if (ntohs(header.length) > sizeof(Header))
   {
      if ((messageLen = recv(clientSocket, bufPtr, ntohs(header.length) - sizeof(Header), MSG_WAITALL)) < 0)
      {
         perror("recv call");
         exit(-1);
      }
   }
   
   /* Handle the incoming packet */
   handlePacket(header, clientSocket, buf);
}

void handlePacket(Header header, int clientSocket, char *buf)
{
   switch (header.flags)
   {
      case (FLAG_1): /* A new client wants to connect and has shared its handle! */
      {
         receiveHandle(header, buf, clientSocket);
         break;
      }
      case (FLAG_5): /* A message is incoming to be forwarded */
      {
         transmitMsg(header, buf, clientSocket);
         break;
      }
      case (FLAG_8): /* A client wants to disconnect from the server */
      {
         disconnectClient(clientSocket);
         break;
      }
      case (FLAG_10): /* A client wants a list of all the handles of the clients */
      {
         transmitHandles(clientSocket);
         break;
      }
      default:
      {
         break;
      }
   }
}

/* Receive a new client's handle */
void receiveHandle(Header header, char* bufPtr, int socketNum)
{
   /* Parse the packet to get the name of the handle */
   uint8_t handleLen;
   uint8_t *handleLenPtr = &handleLen;
   char handle[MAXBUF];
   Header hdr;
   
   memcpy(&hdr, bufPtr, sizeof(Header));
   bufPtr += sizeof(Header);
   
   memcpy(handleLenPtr, bufPtr, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   memcpy(handle, bufPtr, handleLen);
   handle[handleLen] = '\0';
   
   /* Check to see if the handle already exists. If not, add it to the LL and accept the new client */
   Node *node = getHead();
   int isHandleExists = 0;
   while (node != NULL)
   {
      /* Check to see if the handle already exists */
      if (node->handle != NULL)
      {
         if (!strcmp(node->handle, handle))
         {
            isHandleExists = 1;
         }
      }

      /* Since all new clients were put at the end of the LL, we know if the sockets are equal,
       * we have already checked the entire LL */
      if (node->socket == socketNum)
      {
         if (isHandleExists)
         {
            del(socketNum);
            sendHandleAlreadyExists(socketNum);
         }
         else
         {
            if ((node->handle = realloc(node->handle, strlen(handle) + 1)) == NULL)
            {
               perror("malloc");
            }
            memcpy(node->handle, handle, strlen(handle) + 1);
            
            /* Send confirmation to the client that they have been accepted */
            sendConfirmHandle(socketNum);
         }
         break;
      }
      node = node->next;
   }
}

/* Sends a packet to the client telling them their handle is valid and they are ready to start messaging! */
void sendConfirmHandle(int socketNum)
{  
   sendHeader(socketNum, FLAG_2);
}

/* Sends a packet to the client telling them their handle exists and they are not accepted */
void sendHandleAlreadyExists(int socketNum)
{
   sendHeader(socketNum, FLAG_3);
}

/* Forward a message to all destination clients */
void transmitMsg(Header header, char *buf, int clientSocket)
{
   /* Parse the packet to get all the destination clients */
   uint8_t receivingHandleLen;
   Header hdr;
   char *bufPtr = buf;
   
   memcpy(&hdr, bufPtr, sizeof(Header));
   bufPtr += sizeof(Header);
   
   memcpy(&receivingHandleLen, bufPtr, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   char receivingHandle[receivingHandleLen + 1];
   memcpy(receivingHandle, bufPtr, receivingHandleLen);
   receivingHandle[receivingHandleLen] = '\0';
   bufPtr += receivingHandleLen;
      
   uint8_t numHandles;
   memcpy(&numHandles, bufPtr, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   /* For each destination handle, forward this exact packet to that client */
   int i;
   char handles[numHandles][100];
   for (i = 0; i < numHandles; i++)
   {
      uint8_t len;
      memcpy(&len, bufPtr, sizeof(uint8_t));
      bufPtr += sizeof(uint8_t);

      memcpy(handles[i], bufPtr, len);
      bufPtr += len;
      handles[i][len] = '\0';
      
      Node *node = find(handles[i]);
      
      /* If the client exists, send it to them */
      if (node != NULL)
      {         
         safeSend(node->socket, buf, (size_t) ntohs(header.length), 0);
      }
      
      /* If the client does not exist, send an error packet to the sending client */
      else
      {         
         sendHendleErrorMsg(clientSocket, handles[i]);
      }
   }
}

/* Sends an error packet to the sending client to inform them a destination handle does not exist */
void sendHendleErrorMsg(int clientSocket, char *handle)
{
   /* Assemble packet with invalid destination packet */
   char buf[MAXBUF];
   char *bufPtr = buf;
   
   bufPtr += sizeof(Header);
   
   uint8_t handleLen = strlen(handle);
   memcpy(bufPtr, &handleLen, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   memcpy(bufPtr, handle, handleLen);
   bufPtr += handleLen;
   
   uint16_t totalLen = bufPtr - buf;
   Header header = createHeader(FLAG_7, totalLen);
   memcpy(buf, &header, sizeof(Header));
   
   /* Send the packet to the sending client */
   safeSend(clientSocket, buf, (size_t) totalLen, 0);
}

/* Send the current handles to the client that requested them in a series of packets */
void transmitHandles(int clientSocket)
{
   /* Send a packet with the number of handles */
   transmitNumHandles(clientSocket);
   
   /* Send a packet for each handle with the name of the handle */
   Node *node = getHead();
   while(node != NULL)
   {
      transmitHandle(clientSocket, node->handle);
      
      node = node->next;
   }
   
   /* Send a packet informing the client there are no more handles incoming */
   sendHeader(clientSocket, FLAG_13);
}

/* Send a packet with the number of handles */
void transmitNumHandles(int clientSocket)
{
   /* Assemble a packet with the number of handles */
   uint32_t count = 0;
   Node *node = getHead();
   uint16_t packetLen = sizeof(Header) + sizeof(uint32_t);
   
   /* Get number of handles */
   while(node != NULL)
   {
      count++;
      node = node->next;
   }
   
   char buf[MAXBUF];
   char *bufPtr = buf;
   
   Header header = createHeader(FLAG_11, packetLen);
   memcpy(bufPtr, &header, sizeof(Header));
   bufPtr += sizeof(Header);
   
   count = htonl(count);
   memcpy(bufPtr, &count, sizeof(uint32_t));
   
   /* Send the packet */
   safeSend(clientSocket, buf, packetLen, 0);
}

/* Send a packet for each handle with the name of the handle */
void transmitHandle(int clientSocket, char *handle)
{
   char buf[MAXBUF];
   char *bufPtr = buf;
   uint16_t packetLen = sizeof(Header) + sizeof(uint8_t) + strlen(handle);

   Header header = createHeader(FLAG_12, packetLen);
   memcpy(bufPtr, &header, sizeof(Header));
   bufPtr += sizeof(Header);
   
   uint8_t handleLen = strlen(handle);
   memcpy(bufPtr, &handleLen, sizeof(uint8_t));
   bufPtr += sizeof(uint8_t);
   
   memcpy(bufPtr, handle, strlen(handle));
   
   /* Send the packet */
   safeSend(clientSocket, buf, packetLen, 0);
}

/* Remove a client from the list of active clients and send a confirmation to the client they can terminate safely */
void disconnectClient(int clientSocket)
{
   del(clientSocket);
   
   sendHeader(clientSocket, FLAG_9);
   
   close(clientSocket);
}

/* Check to make sure the cmd line args are valid */
int checkArgs(int argc, char *argv[])
{
	int portNumber = 0;

   /* There cannot be more than 2 args */
	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
   /* If there is 2 args, grab the port number from the 2nd arg */
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}


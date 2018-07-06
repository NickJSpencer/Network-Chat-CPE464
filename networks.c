
// Hugh Smith April 2017
// Network code to support TCP client server connections

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
#include "gethostbyname6.h"
#include "linkedList.h"

/* This function sets the server socket.  It lets the system
determine the port number.  The function returns the server
socket number and prints the port number to the screen.  */

int tcpServerSetup(int portNumber)
{
	int server_socket= 0;
	struct sockaddr_in6 server;      /* socket address for local side  */
	socklen_t len= sizeof(server);  /* length of local address        */

	/* create the tcp socket  */
	server_socket= socket(AF_INET6, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		perror("socket call");
		exit(FAILURE);
	}

	server.sin6_family= AF_INET6;         		
	server.sin6_addr = in6addr_any;   //wild card machine address
	server.sin6_port= htons(portNumber);         

	/* bind the name (address) to a port */
	if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("bind call");
		exit(FAILURE);
	}
	
	//get the port name and print it out
	if (getsockname(server_socket, (struct sockaddr*)&server, &len) < 0)
	{
		perror("getsockname call");
		exit(FAILURE);
	}

	if (listen(server_socket, BACKLOG) < 0)
	{
		perror("listen call");
		exit(FAILURE);
	}
	
	printf("Server Port Number %d \n", ntohs(server.sin6_port));
	
	return server_socket;
}

// This function waits for a client to ask for services. It returns
// the client socket number.

int tcpAccept(int server_socket, int debugFlag)
{
	struct sockaddr_in6 clientInfo;   
	int clientInfoSize = sizeof(clientInfo);
	int client_socket= 0;

	if ((client_socket = accept(server_socket, (struct sockaddr*) &clientInfo, (socklen_t *) &clientInfoSize)) < 0)
	{
		perror("accept call");
		exit(FAILURE);
	}
   
   add("", client_socket);
   
	return(client_socket);
}

int tcpClientSetup(char *handle, char * serverName, char * port, int debugFlag)
{
	// This is used by the client to connect to a server using TCP
	
	int socket_num;
	uint8_t * ipAddress = NULL;
	struct sockaddr_in6 server;      
	
	// create the socket
	if ((socket_num = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("socket call");
		exit(FAILURE);
	}

	// setup the server structure
	server.sin6_family = AF_INET6;
	server.sin6_port = htons(atoi(port));
	
	// get the address of the server 
	if ((ipAddress = getIPAddress6(serverName, &server)) == NULL)
	{
		exit(FAILURE);
	}

	if(connect(socket_num, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		perror("connect call");
		exit(FAILURE);
	}
	
	return socket_num;
}

/* Send a packet with only a header with the given flag */
ssize_t sendHeader(int socketNum, uint8_t flag)
{
   char sendBuf[MAXBUF];
   Header header = createHeader(flag, sizeof(Header));
   
   memcpy(sendBuf, &header, sizeof(Header));
   
   /* Send the packet */
   return safeSend(socketNum, sendBuf, sizeof(Header), 0);
}

/* Create a header with the given flag and length of packet */
Header createHeader(uint8_t flag, uint16_t length)
{
   Header header;
   header.length = htons(length);
   header.flags = flag;
   return header;
}

/* Send a packet with proper error checking */
ssize_t safeSend(int socket, const void *buffer, size_t length, int flags)
{
   int sent = send(socket, buffer, length, flags);
   if (sent < 0)
	{
		perror("send");
		exit(FAILURE);
	}
   
   return sent;
}

/* Get max of 2 integers */
int max(int x, int y)
{
   if (x >= y)
   {
      return x;
   }
   return y;
}
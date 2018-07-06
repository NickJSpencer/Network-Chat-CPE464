
/* 	Code originally give to Prof. Smith by his TA in 1994.
	No idea who wrote it.  Copy and use at your own Risk
*/


#ifndef __NETWORKS_H__
   #define __NETWORKS_H__
   #include <stdint.h>

   #define BACKLOG 10
   #define MAXBUF 1024
   
   #define SUCCESS 0
   #define FAILURE 1
   
   #define FLAG_1 1
   #define FLAG_2 2
   #define FLAG_3 3
   #define FLAG_5 5
   #define FLAG_7 7
   #define FLAG_8 8
   #define FLAG_9 9
   #define FLAG_10 10
   #define FLAG_11 11
   #define FLAG_12 12
   #define FLAG_13 13
   
   typedef struct __attribute__ ((__packed__)) header {
      uint16_t length;
      uint8_t flags;
   } Header;

   char *clientHandle;
   
   // for the server side
   int tcpServerSetup(int portNumber);
   int tcpAccept(int server_socket, int debugFlag);

   // for the client side
   int tcpClientSetup(char *handle, char * serverName, char * port, int debugFlag);

   ssize_t sendHeader(int socketNum, uint8_t flag);
   Header createHeader(uint8_t flag, uint16_t length);
   ssize_t safeSend(int socket, const void *buffer, size_t length, int flags);
   
   int max(int x, int y);
#endif

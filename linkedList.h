#ifndef LINKEDLIST_H
   #define LINKEDLIST_H

   #include <stdint.h>
   
   typedef struct node {
      struct node *next;
      char *handle;
      uint32_t socket;
   } Node;
   
   struct node *head;
   
   struct node *add(char *handle, uint32_t socket);
   int del(uint32_t socket);
   int delByHandle(char *handle);
   struct node *getHead();
      
   struct node *find(char *handle);
   int freeAll();
   
#endif
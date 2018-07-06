#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "linkedList.h"

#define SUCCESS 0
#define FAILURE 1

Node *add(char *handle, uint32_t socket)
{
   Node *node = head;
   Node *newNode;
   
   if ((newNode = malloc(sizeof(Node))) == NULL)
   {
      perror("malloc");
      return NULL;
   }
   
   if ((newNode->handle = malloc(strlen(handle) + 1)) == NULL)
   {
      perror("malloc");
      return NULL;
   }
   
   newNode->handle = strcpy(newNode->handle, handle);
   newNode->socket = socket;
   newNode->next = NULL;
      
   /* If there is no entries, this one is head */
   if (node == NULL)
   {
      head = newNode;
   }
   
   /* If there are entries, put this one at the end */
   else
   {
      while (node->next != NULL)
      {
         node = node->next;
      }
      
      node->next = newNode;
   }
   
   return newNode;
}

int del(uint32_t socket)
{
   Node *node = head;
   Node *prev;
   
   /* If there are no entries, return error */
   if (node == NULL)
   {
      return FAILURE;
   }
   
   /* If the head is being deleted, assign a new head */
   if (node->socket == socket)
   {
      head = node->next;
      free(node->handle);
      free(node);
      node = NULL;
      return SUCCESS;
   }
   /* Find the entry with the correct handle */
   while(node->socket != socket)
   {
      prev = node;
      node = node->next;
      
      if (node == NULL)
      {
         return FAILURE;
      }
   }
   /* Delete the handle that matches */
   prev->next = node->next;
   free(node->handle);
   free(node);
   node = NULL;
   return SUCCESS;
}

int delByHandle(char *handle)
{
   Node *node = head;
   Node *prev;
   
   /* If there are no entries, return error */
   if (node == NULL)
   {
      return FAILURE;
   }
   
   /* If the head is being deleted, assign a new head */
   if (!strcmp(node->handle, handle))
   {
      head = node->next;
      free(node->handle);
      free(node);
      node = NULL;

      return SUCCESS;
   }
   /* Find the entry with the correct handle */
   while(strcmp(node->handle, handle))
   {
      prev = node;
      node = node->next;
      
      if (node == NULL)
      {
         return FAILURE;
      }
   }
   
   /* Delete the handle that matches */
   prev->next = node->next;
   free(node->handle);
   free(node);
   node = NULL;
   return SUCCESS;
}

Node *find(char *handle)
{
   Node* node = head;
   
   while(node != NULL)
   {
      if (!strcmp(node->handle, handle))
      {
         return node;
      }
      node = node->next;
   }
   
   return NULL;
}
  
/* Return head */  
Node *getHead()
{
   return head;
}

int freeAll()
{
   Node *node = head;
   Node *prev;
   
   /* If there are no nodes, free them all */
   if (node == NULL)
   {
      return SUCCESS;
   }
   
   /* Free all nodes */
   while (node != NULL)
   {
      prev = node;
      node = node->next;
      free(prev);
   }
   
   /* There is now no head */
   head = NULL;
   
   return SUCCESS;
}
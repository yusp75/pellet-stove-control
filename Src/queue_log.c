/*
* File: queue_log.c
*/

#include "queue_log.h"


static uint8_t log_count = 0;
extern osPoolId pool_LogHandle;


Node *rear, *front;

void InitLogQueue()
{
  rear = front = NULL;
  log_count = 0;
}


void delQueue()
{
  Node *var=rear;
  if(var == rear)
  {
    rear = rear->next;
    //free(var);
    osPoolFree(pool_LogHandle, var);
    log_count--;
  }

}

void push(uint8_t idx, uint8_t min, uint8_t hour)
{
  Node *temp;
  //temp=(Node *)malloc(sizeof(Node));
  
  temp = (Node *)osPoolCAlloc(pool_LogHandle);
  if (temp == NULL) {
    delQueue();
    temp = (Node *)osPoolCAlloc(pool_LogHandle);
  }
  
  if (temp != NULL) {
    temp->idx = idx;
    temp->min = min;
    temp->hour = hour;
    
    if (front == NULL)
    {
      front = temp;
      front->next = NULL;
      rear = front;
    }
    else
    {
      front->next = temp;
      front = temp;
      front->next = NULL;
    }
    
    log_count++;
    //printf("log_count:%d\r\n",log_count);
  }
}


Node* element(uint8_t pos)
{  
  if (pos <= log_count){
    
    Node *p_head = rear;
    uint8_t i = 0;
    
    while(p_head!=NULL) {
      if(i==pos){
        break;
      }
      p_head = p_head->next;
      i++;
    }
    return p_head;
  }
  else {
    return NULL;
  }
}


uint8_t getQueueSize()
{
  return log_count;
  
}

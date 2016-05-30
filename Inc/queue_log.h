/*
* File: queue_log.h

*/

#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

#include<stdio.h>
#include<stdlib.h>
#include "stdint.h"
#include "cmsis_os.h"

#define LOG_MAX 12
#define LOG_MAX_SCROLL_DOWN 4

struct node
{
  uint8_t idx;
  uint8_t min;
  uint8_t hour;
  struct node* next;
};

typedef struct node Node;
 

void InitLogQueue();
void delQueue();
void push(uint8_t idx, uint8_t min, uint8_t hour);
uint8_t getQueueSize();

Node* element(uint8_t idx);

#endif // End of queue header
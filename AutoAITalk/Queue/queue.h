#ifndef __QUEUE_H__
#define __QUEUE_H__


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "n32h47x_48x.h"
 
#define MAX_DATA_SIZE 250  // 定义数组最大长度
 
typedef struct {
    uint8_t data[MAX_DATA_SIZE];  // 数据数组
    size_t length;                // 实际数据长度
} AudioPacket;
 
typedef struct QueueNode {
    struct QueueNode* next;
    AudioPacket packet;  // 使用AudioPacket代替QDataType
} QNode;
 
typedef struct Queue
{
	QNode* head;
	QNode* tail;
	int size;
}Queue;
 
// 初始化队列 
void QueueInit(Queue* pq);
// 队尾入队列 
void QueuePush(Queue* pq, const uint8_t* data, size_t length);
// 队头出队列 
void QueuePop(Queue* pq);
// 获取队列头部元素 
AudioPacket* QueueFront(Queue* pq);
// 获取队列队尾元素 
AudioPacket* QueueBack(Queue* pq);
// 获取队列中有效元素个数 
int QueueSize(Queue* pq);
// 检测队列是否为空，如果为空返回非零结果，如果非空返回0 
int QueueEmpty(Queue* pq);
// 销毁队列 
void QueueDestroy(Queue* pq);

#ifdef __cplusplus
}
#endif

#endif
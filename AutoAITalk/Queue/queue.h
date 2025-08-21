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
 
#define MAX_DATA_SIZE 250  // ����������󳤶�
 
typedef struct {
    uint8_t data[MAX_DATA_SIZE];  // ��������
    size_t length;                // ʵ�����ݳ���
} AudioPacket;
 
typedef struct QueueNode {
    struct QueueNode* next;
    AudioPacket packet;  // ʹ��AudioPacket����QDataType
} QNode;
 
typedef struct Queue
{
	QNode* head;
	QNode* tail;
	int size;
}Queue;
 
// ��ʼ������ 
void QueueInit(Queue* pq);
// ��β����� 
void QueuePush(Queue* pq, const uint8_t* data, size_t length);
// ��ͷ������ 
void QueuePop(Queue* pq);
// ��ȡ����ͷ��Ԫ�� 
AudioPacket* QueueFront(Queue* pq);
// ��ȡ���ж�βԪ�� 
AudioPacket* QueueBack(Queue* pq);
// ��ȡ��������ЧԪ�ظ��� 
int QueueSize(Queue* pq);
// �������Ƿ�Ϊ�գ����Ϊ�շ��ط�����������ǿշ���0 
int QueueEmpty(Queue* pq);
// ���ٶ��� 
void QueueDestroy(Queue* pq);

#ifdef __cplusplus
}
#endif

#endif
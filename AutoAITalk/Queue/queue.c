#include "queue.h"
#include "main.h"

/**
 * @brief  ��ʼ������
 * @param  pq: ָ�����ʼ�����е�ָ��
 * @note   �ú��������е�ͷָ�롢βָ���ÿգ�Ԫ�ظ������㣬��ʾ����һ���ն��С�
 *         ������ʹ�ö���ǰ���ô˺������г�ʼ����
 */
void QueueInit(Queue* pq)
{
    pq->head = pq->tail = NULL;  // ��ʼ״̬������Ϊ�գ�ͷβָ���ָ�� NULL
    pq->size = 0;                // ��ǰԪ������Ϊ 0
}

// ��β�����
void QueuePush(Queue* pq, const uint8_t* data, size_t length) {
    QNode* newnode = (QNode*)malloc(sizeof(QNode));
    if (newnode == NULL) {
        perror("malloc fail");
        return;
    }
    
    // �������ݵ��½ڵ�
    memcpy(newnode->packet.data, data, length);
    newnode->packet.length = length;
    newnode->next = NULL;
    
    if (pq->tail == NULL) {
        pq->head = pq->tail = newnode;
    } else {
        pq->tail->next = newnode;
        pq->tail = newnode;
    }
    pq->size++;
}

// ��ͷ������
void QueuePop(Queue* q) {
    if (!q->head) return;
    
    QNode* temp = q->head;
    q->head = q->head->next;
    if (!q->head) q->tail = NULL;
    free(temp);
    q->size--;
}

// ��ȡ����ͷ��Ԫ��
AudioPacket* QueueFront(Queue* pq) {
    return &(pq->head->packet);
}

// ��ȡ���ж�βԪ��
AudioPacket* QueueBack(Queue* pq) {
    return &(pq->tail->packet);
}

// ��ȡ��������ЧԪ�ظ���
int QueueSize(Queue* pq) {
    return pq->size;
}

// �������Ƿ�Ϊ��
int QueueEmpty(Queue* pq) {
    return pq->size == 0;
}

// ���ٶ���
void QueueDestroy(Queue* pq) {
    QNode* cur = pq->head;
    while (cur) {
        QNode* next = cur->next;
        free(cur);
        cur = next;
    }
    pq->head = pq->tail = NULL;
    pq->size = 0;
}
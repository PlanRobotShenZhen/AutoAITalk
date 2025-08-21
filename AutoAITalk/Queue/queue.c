#include "queue.h"
#include "main.h"

/**
 * @brief  初始化队列
 * @param  pq: 指向待初始化队列的指针
 * @note   该函数将队列的头指针、尾指针置空，元素个数清零，表示创建一个空队列。
 *         必须在使用队列前调用此函数进行初始化。
 */
void QueueInit(Queue* pq)
{
    pq->head = pq->tail = NULL;  // 初始状态：队列为空，头尾指针均指向 NULL
    pq->size = 0;                // 当前元素数量为 0
}

// 队尾入队列
void QueuePush(Queue* pq, const uint8_t* data, size_t length) {
    QNode* newnode = (QNode*)malloc(sizeof(QNode));
    if (newnode == NULL) {
        perror("malloc fail");
        return;
    }
    
    // 复制数据到新节点
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

// 队头出队列
void QueuePop(Queue* q) {
    if (!q->head) return;
    
    QNode* temp = q->head;
    q->head = q->head->next;
    if (!q->head) q->tail = NULL;
    free(temp);
    q->size--;
}

// 获取队列头部元素
AudioPacket* QueueFront(Queue* pq) {
    return &(pq->head->packet);
}

// 获取队列队尾元素
AudioPacket* QueueBack(Queue* pq) {
    return &(pq->tail->packet);
}

// 获取队列中有效元素个数
int QueueSize(Queue* pq) {
    return pq->size;
}

// 检测队列是否为空
int QueueEmpty(Queue* pq) {
    return pq->size == 0;
}

// 销毁队列
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
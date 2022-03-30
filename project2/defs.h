#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
typedef enum State
{
    READY,
    RUNNING,
    USING_DEVICE_1,
    USING_DEVICE_2,
    TERMINATED
} State;

typedef enum SchedulingAlgorithm
{
    FCFS,
    SJF,
    RR
} SchedulingAlgorithm;

typedef struct PCB
{
    int pid;
    pthread_t thread_id;
    State state;
    int next_burst_length;
    int previous_burst_length;
    int remaining_burst_length;
    int number_of_cpu_bursts;
    int time_spent_in_ready_queue;
    int time_spent_in_cpu;
    int arrival_time;
    int finish_time;
    int number_of_io_device1;
    int number_of_io_device2;
    int last_ready_queue_enterance;
} PCB;

typedef struct node
{
    PCB data;
    struct node *next;
    struct node *prev;
} node;

typedef struct queue
{
    struct node *front;
    struct node *rear;
} queue;

int isEmpty(struct queue *q)
{
    return q->front == NULL;
}

void enqueue(struct queue *q, PCB data, SchedulingAlgorithm sa)
{
    if (sa == SJF)
    {
        node *temp = q->front;
        struct node *new_node = (struct node *)malloc(sizeof(struct node));
        new_node->data = data;
        new_node->next = NULL;
        new_node->prev = NULL;
        if (isEmpty(q))
        {
            q->front = new_node;
            q->rear = new_node;
        }
        else
        {
            while (temp && temp->data.next_burst_length < data.next_burst_length)
            {
                temp = temp->next;
            }
            if (!temp)
            {
                q->rear->next = new_node;
                new_node->prev = q->rear;
                q->rear = new_node;
            }
            else if (temp == q->front)
            {
                new_node->next = q->front;
                q->front->prev = new_node;
                q->front = new_node;
            }
            else
            {
                new_node->next = temp;
                temp->prev->next = new_node;
                new_node->prev = temp->prev;
                q->rear = new_node;
            }
        }
    }
    else
    {
        node *temp = q->rear;
        struct node *new_node = (struct node *)malloc(sizeof(struct node));
        new_node->data = data;
        new_node->next = NULL;
        new_node->prev = NULL;
        if (isEmpty(q))
        {
            q->front = new_node;
            q->rear = new_node;
        }
        else
        {
            temp->next = new_node;
            new_node->prev = q->rear;
            q->rear = new_node;
        }
    }
}

node *dequeue(queue *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return NULL;
    }
    else
    {
        struct node *temp = q->front;
        q->front = q->front->next;
        return temp;
    }
}

void printQueue(struct queue *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty\n");
        return;
    }

    struct node *temp = q->front;
    while (temp != NULL)
    {
        printf("%d ", temp->data.pid);
        temp = temp->next;
    }
    printf("\n");
}

queue *createQueue()
{
    queue *q = (queue *)malloc(sizeof(queue));
    q->front = NULL;
    q->rear = NULL;
    return q;
}

void destroyQueue(queue *q)
{
    while (!isEmpty(q))
    {
        node *temp = dequeue(q);
        free(temp);
    }
    free(q);
}

node *createNode(PCB data)
{
    node *new_node = (node *)malloc(sizeof(node));
    new_node->data = data;
    new_node->next = NULL;
    new_node->prev = NULL;
    return new_node;
}

int allTerminated(queue *queue)
{
    node *temp = queue->front;
    while (temp != NULL)
    {
        if (temp->data.state != TERMINATED)
        {
            return 0;
        }
        temp = temp->next;
    }
    return 1;
}

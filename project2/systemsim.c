#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "defs.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))

char outmode, *burst_dist;
SchedulingAlgorithm scheduling_algorithm;
int q, t1, t2, burstlen, min_burst, max_burst, maxp, allp, current_count, current_time, cpu_running_thread_count, thread_count;
int number_of_threads_left;
int current_cpu_pid;
int io1_count, io2_count;
float p0, p1, p2, pg;
pthread_t *threads;
queue *ready_queue;
pthread_mutex_t ready_queue_mutex, ready_queue_io1_mutex, ready_queue_io2_mutex, pid_mutex;
pthread_cond_t ready_queue_cv, thread_cv, ready_queue_io1_cv, ready_queue_io2_cv;

void calculate_next_burst_length(PCB *pcb, SchedulingAlgorithm scheduling_algorithm)
{
    if (scheduling_algorithm == RR)
    {
        pcb->next_burst_length = min(q, pcb->remaining_burst_length);
    }
    else
    {
        if (strcmp(burst_dist, "fixed") == 0)
        {
            pcb->next_burst_length = min(pcb->remaining_burst_length, burstlen);
        }
        else if (strcmp(burst_dist, "uniform") == 0)
        {
            pcb->next_burst_length = min(pcb->remaining_burst_length, rand() % (max_burst - min_burst + 1) + min_burst);
        }
        else if (strcmp(burst_dist, "exponential") == 0)
        {
            // lamba sonra bulmatyı çöz
            double lambda = 0.5; // temp currently
            pcb->next_burst_length = min(pcb->remaining_burst_length,
                                         pcb->previous_burst_length * lambda + (1 - lambda) * burstlen);
        }
    }
}

void *thread_function(void *arg)
{
    // printf("Thread %d created at time: %d\n", ((PCB *)arg)->pid, current_time);
    queue *temp_rq = ready_queue;
    PCB *pcb = (PCB *)arg;
    pcb->state = READY;
    pcb->arrival_time = current_time;
    pcb->number_of_cpu_bursts = 0;
    pcb->thread_id = pthread_self();
    pcb->finish_time = 0;
    pcb->number_of_io_device1 = 0;
    pcb->number_of_io_device2 = 0;
    pcb->time_spent_in_cpu = 0;
    pcb->time_spent_in_ready_queue = 0;
    pcb->previous_burst_length = burstlen;
    pcb->last_ready_queue_enterance = current_time;
    calculate_next_burst_length(pcb, scheduling_algorithm);
    pthread_mutex_lock(&ready_queue_mutex);
    printf("Thread %d enqueued at time: %d\n", pcb->pid, current_time);
    enqueue(ready_queue, *pcb, scheduling_algorithm);
    --current_count;
    pthread_mutex_unlock(&ready_queue_mutex);
    pthread_cond_broadcast(&ready_queue_cv);

    while (pcb->state != TERMINATED)
    {
        pthread_mutex_lock(&ready_queue_mutex);
        while ((cpu_running_thread_count > 0 || pcb->pid != current_cpu_pid) && current_cpu_pid != -1)
        {
            pthread_cond_wait(&ready_queue_cv, &ready_queue_mutex);
        }
        if (current_cpu_pid == -1)
        {
            pthread_mutex_unlock(&ready_queue_mutex);
            pthread_cond_broadcast(&ready_queue_cv);
            continue;
        }
        pthread_mutex_unlock(&ready_queue_mutex);
        pthread_mutex_lock(&ready_queue_mutex);
        printf("Thread %d dequeued at time: %d\n", pcb->pid, current_time);
        node *temp = dequeue(ready_queue);
        PCB *temp_pcb = &(temp->data);
        cpu_running_thread_count++;
        pthread_mutex_unlock(&ready_queue_mutex);
        pthread_cond_broadcast(&ready_queue_cv);

        if (scheduling_algorithm == RR)
        {
            temp_pcb->state = RUNNING;
            temp_pcb->previous_burst_length = pcb->next_burst_length;
            temp_pcb->number_of_cpu_bursts = 0; // io giderse
            temp_pcb->time_spent_in_ready_queue = current_time - temp_pcb->last_ready_queue_enterance;
            temp_pcb->last_ready_queue_enterance = 0; // ready queue eklerken güncelle
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            temp_pcb->time_spent_in_cpu += temp_pcb->next_burst_length; // after sleep
            current_time += temp_pcb->next_burst_length;
            usleep(temp_pcb->next_burst_length * 1000);
            pthread_mutex_lock(&ready_queue_mutex);
            if (pcb->remaining_burst_length != 0)
            {
                enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
                printf("Thread %d enqueued at time: %d\n", temp_pcb->pid, current_time);
            }
            cpu_running_thread_count--;
            pthread_mutex_unlock(&ready_queue_mutex);
            pthread_cond_broadcast(&ready_queue_cv);
        }
        else
        {
            temp_pcb->state = RUNNING;
            temp_pcb->previous_burst_length = pcb->next_burst_length;
            temp_pcb->number_of_cpu_bursts = 0; // io giderse
            temp_pcb->time_spent_in_ready_queue = current_time - temp_pcb->last_ready_queue_enterance;
            temp_pcb->last_ready_queue_enterance = 0; // ready queue eklerken güncelle
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            temp_pcb->time_spent_in_cpu += temp_pcb->next_burst_length; // after sleep
            current_time += temp_pcb->next_burst_length;
            usleep(temp_pcb->next_burst_length * 1000);
            temp_pcb->remaining_burst_length = 0;
            cpu_running_thread_count--;
            pthread_cond_broadcast(&ready_queue_cv);
        } // jj
        int prob = rand() % 100;
        if (prob < p0 * 100)
        {
            pcb->state = TERMINATED;
            temp_pcb->state = TERMINATED;
            pthread_cond_broadcast(&ready_queue_cv);
        }
        else if (prob < (p0 + p1) * 100)
        {
            pthread_mutex_lock(&ready_queue_io1_mutex);
            while (io1_count > 0)
            {
                pthread_cond_wait(&ready_queue_io1_cv, &ready_queue_io1_mutex);
            }
            temp_pcb->state = USING_DEVICE_1;
            io1_count++;
            printf("Thread %d is using IO1 at time: %d\n\n", temp_pcb->pid, current_time);
            usleep(t1 * 1000);
            temp_pcb->number_of_io_device1++;
            temp_pcb->number_of_cpu_bursts++;
            current_time += t1;
            io1_count--;
            printf("Thread %d finished using IO1 at time: %d\n", temp_pcb->pid, current_time);
            pthread_mutex_unlock(&ready_queue_io1_mutex);
            pthread_cond_signal(&ready_queue_io1_cv);
            pthread_mutex_lock(&ready_queue_mutex);
            temp_pcb->state = READY;
            enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
            pthread_mutex_unlock(&ready_queue_mutex);
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            pthread_cond_broadcast(&ready_queue_cv);
        }
        else
        {
            pthread_mutex_lock(&ready_queue_io2_mutex);
            while (io2_count > 0)
            {
                pthread_cond_wait(&ready_queue_io2_cv, &ready_queue_io2_mutex);
            }
            io2_count++;
            printf("Thread %d is using IO2 at time: %d\n\n", temp_pcb->pid, current_time);
            usleep(t2 * 1000);
            temp_pcb->number_of_io_device2++;
            temp_pcb->number_of_io_device1++;
            io2_count--;
            printf("Thread %d finished using IO2 at time: %d\n", temp_pcb->pid, current_time);
            pthread_mutex_unlock(&ready_queue_io2_mutex);
            pthread_mutex_lock(&ready_queue_mutex);
            enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
            pthread_mutex_unlock(&ready_queue_mutex);
            pthread_cond_signal(&ready_queue_io2_cv);
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            pthread_cond_broadcast(&ready_queue_cv);
        }
    }
    // print threads
    --number_of_threads_left;
    printf("Thread %d is ENDED at time: %d\n", pcb->pid, current_time);
    pthread_exit(NULL);
}

void *generator_syscall(void *arg)
{
    burstlen = 30;
    burst_dist = "fixed";
    current_time = 0;
    int thread_count = 0;
    current_count = 0;
    current_cpu_pid = -1;
    if (allp < 10)
    {
        for (size_t i = 0; i < maxp; i++)
        {
            PCB *pcb = (PCB *)malloc(sizeof(PCB));
            pcb->pid = i;
            pthread_create(&threads[i], NULL, &thread_function, (void *)pcb);
            // usleep(5000);
            ++current_count;
        }
        thread_count = maxp;
        int i = thread_count;
        while (thread_count != allp)
        {
            if (current_count < maxp)
            {
                int probability = rand() % 100;
                if (probability <= pg)
                {
                    PCB *pcb = (PCB *)malloc(sizeof(PCB));
                    pcb->pid = i++;
                    pthread_create(&threads[thread_count], NULL, &thread_function, (void *)pcb);
                    ++current_count;
                    thread_count++;
                }
                usleep(5000);
                current_time += 5;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < 10; i++)
        {
            PCB *pcb = (PCB *)malloc(sizeof(PCB));
            pcb->pid = i;
            pthread_create(&threads[i], NULL, &thread_function, (void *)pcb);
            ++current_count;
        }
        thread_count = 10;
        int i = thread_count;
        while (thread_count != allp)
        {
            if (current_count < maxp)
            {
                int probability = rand() % 100;
                if (probability <= pg)
                {
                    PCB *pcb = (PCB *)malloc(sizeof(PCB));
                    pcb->pid = i++;
                    pthread_create(&threads[thread_count], NULL, &thread_function, (void *)pcb);
                    ++current_count;
                    thread_count++;
                }
                usleep(5000);
                current_time += 5;
            }
        }
    }
    return NULL;
}
/*
systemsim <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst>
<max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>
*/

void *scheduler_func(void *arg)
{
    while (1)
    {
        if (number_of_threads_left == 0)
        {
            break;
        }
        else
        {
            pthread_mutex_lock(&ready_queue_mutex);
            queue *temp_rq = ready_queue;
            while (cpu_running_thread_count > 0)
            {
                pthread_cond_wait(&ready_queue_cv, &ready_queue_mutex);
            }
            if (ready_queue->front != NULL)
            {
                // pthread_mutex_lock(&ready_queue_mutex);
                current_cpu_pid = ready_queue->front->data.pid;
                // pthread_mutex_unlock(&ready_queue_mutex);
            }
            else
            {
                current_cpu_pid = -1;
            }
            pthread_mutex_unlock(&ready_queue_mutex);
            pthread_cond_broadcast(&ready_queue_cv);
        }
    }
    return NULL;
}

int main(int argc, char *args[])
{
    pthread_mutex_init(&ready_queue_mutex, NULL);
    pthread_mutex_init(&ready_queue_io1_mutex, NULL);
    pthread_mutex_init(&ready_queue_io2_mutex, NULL);
    pthread_cond_init(&ready_queue_cv, NULL);
    pthread_cond_init(&thread_cv, NULL);
    pthread_cond_init(&ready_queue_io1_cv, NULL);
    pthread_cond_init(&thread_cv, NULL);
    scheduling_algorithm = RR;
    ready_queue = createQueue();
    p0 = 0.3;
    max_burst = 60;
    min_burst = 30;
    p1 = 0.4;
    p2 = 0.3;
    maxp = 2;
    io1_count = 0;
    io2_count = 0;
    allp = 4;
    t1 = 31;
    t2 = 20;
    number_of_threads_left = allp;
    cpu_running_thread_count = 0;
    threads = (pthread_t *)malloc(sizeof(pthread_t) * allp);
    pthread_t generator, scheduler;
    pthread_create(&generator, NULL, &generator_syscall, NULL);

    pthread_create(&scheduler, NULL, &scheduler_func, NULL); // bu yapıcak scheduling algoritmasının çalışmasını sağlayacak
    pthread_join(scheduler, NULL);
    pthread_join(generator, NULL);
    for (size_t i = 0; i < maxp; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&ready_queue_io1_mutex);
    pthread_mutex_destroy(&ready_queue_io2_mutex);
    pthread_mutex_destroy(&ready_queue_mutex);
    // pthread_mutex_destroy(&thread_cv);
    return 0;
}

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "defs.h"
#include <sys/time.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))

// TODO: pcb update
// TODO: time update
// TODO: get input from terminal
// TODO: print output to terminal
struct timeval start_time, current_time;
char outmode, *burst_dist;
SchedulingAlgorithm scheduling_algorithm;
int q, t1, t2, burstlen, min_burst, max_burst, maxp, allp, current_count, cpu_running_thread_count, thread_count;
int number_of_threads_left;
int current_cpu_pid;
int io1_count, io2_count;
float p0, p1, p2, pg;
long int start_time_val;
pthread_t *threads;
queue *ready_queue;
pthread_mutex_t ready_queue_mutex, ready_queue_io1_mutex, ready_queue_io2_mutex, time_mutex;
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
            // TODO: implement exponential distribution
            double lambda = 0.5;
            pcb->next_burst_length = min(pcb->remaining_burst_length,
                                         pcb->previous_burst_length * lambda + (1 - lambda) * burstlen);
        }
    }
}

void *thread_function(void *arg)
{
    // queue *temp_rq = ready_queue;
    PCB *pcb = (PCB *)arg;
    pcb->state = READY;
    pthread_mutex_lock(&time_mutex);
    gettimeofday(&current_time, NULL);
    pcb->arrival_time = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - ((start_time_val));
    pthread_mutex_unlock(&time_mutex);
    pcb->number_of_cpu_bursts = 0;
    pcb->thread_id = pthread_self();
    pcb->finish_time = 0;
    pcb->number_of_io_device1 = 0;
    pcb->number_of_io_device2 = 0;
    pcb->time_spent_in_cpu = 0;
    pcb->time_spent_in_ready_queue = 0;
    pcb->previous_burst_length = burstlen;
    pcb->last_ready_queue_enterance = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val);
    calculate_next_burst_length(pcb, scheduling_algorithm);
    pthread_mutex_lock(&ready_queue_mutex);
    printf("Thread %d enqueued at time: %ld\n", pcb->pid, ((current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val)));
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
        pthread_mutex_lock(&time_mutex);
        gettimeofday(&current_time, NULL);
        printf("Thread %d dequeued at time: %ld\n", pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
        pthread_mutex_unlock(&time_mutex);
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
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->time_spent_in_ready_queue = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - temp_pcb->last_ready_queue_enterance;
            pthread_mutex_unlock(&time_mutex);
            temp_pcb->last_ready_queue_enterance = 0; // ready queue eklerken güncelle
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            temp_pcb->time_spent_in_cpu += temp_pcb->next_burst_length; // after sleep
            usleep(temp_pcb->next_burst_length * 1000);
            pthread_mutex_lock(&ready_queue_mutex);
            if (pcb->remaining_burst_length != 0)
            {
                pthread_mutex_lock(&time_mutex);
                gettimeofday(&current_time, NULL);
                enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
                printf("Thread %d enqueued at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val));
                pthread_mutex_unlock(&time_mutex);
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
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->time_spent_in_ready_queue = current_time.tv_sec * 1000 + current_time.tv_usec / 1000 - temp_pcb->last_ready_queue_enterance;
            pthread_mutex_unlock(&time_mutex);
            temp_pcb->last_ready_queue_enterance = 0; // ready queue eklerken güncelle
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            temp_pcb->time_spent_in_cpu += temp_pcb->next_burst_length; // after sleep
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
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            printf("Thread %d is using IO1 at time: %ld\n\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val));
            pthread_mutex_unlock(&time_mutex);
            usleep(t1 * 1000);
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            pthread_mutex_unlock(&time_mutex);
            temp_pcb->number_of_io_device1++;
            temp_pcb->number_of_cpu_bursts++;
            io1_count--;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            printf("Thread %d finished using IO1 at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val));
            pthread_mutex_unlock(&time_mutex);
            pthread_mutex_unlock(&ready_queue_io1_mutex);
            pthread_cond_signal(&ready_queue_io1_cv);
            pthread_mutex_lock(&ready_queue_mutex);
            temp_pcb->state = READY;
            enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
            printf("Thread %d enqueued at time: %ld\n", pcb->pid, ((current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val)));
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
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            printf("Thread %d is using IO2 at time: %ld\n\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val));
            pthread_mutex_unlock(&time_mutex);
            usleep(t2 * 1000);
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            printf("Thread %d finished using IO2 at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val));
            temp_pcb->number_of_io_device2++;
            temp_pcb->number_of_cpu_bursts++;
            io2_count--;
            pthread_mutex_unlock(&time_mutex);
            pthread_mutex_unlock(&ready_queue_io2_mutex);
            pthread_mutex_lock(&ready_queue_mutex);
            enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            printf("Thread %d enqueued at time: %ld\n", pcb->pid, ((current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val)));
            pthread_mutex_unlock(&time_mutex);
            pthread_mutex_unlock(&ready_queue_mutex);
            pthread_cond_signal(&ready_queue_io2_cv);
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            pthread_cond_broadcast(&ready_queue_cv);
        }
    }
    // print threads
    --number_of_threads_left;
    pthread_mutex_lock(&time_mutex);
    gettimeofday(&current_time, NULL);
    printf("Thread %d is ENDED at time: %ld\n", pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val));
    pthread_mutex_unlock(&time_mutex);
    pthread_exit(NULL);
}

void *generator_syscall(void *arg)
{
    int thread_count = 0;
    current_count = 0;
    current_cpu_pid = -1;
    pthread_mutex_lock(&time_mutex);
    gettimeofday(&start_time, NULL);
    start_time_val = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;
    pthread_mutex_unlock(&time_mutex);
    if (allp < 10)
    {
        for (size_t i = 0; i < maxp; i++)
        {
            PCB *pcb = (PCB *)malloc(sizeof(PCB));
            pcb->pid = i;
            pthread_create(&threads[i], NULL, &thread_function, (void *)pcb);
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
            }
        }
    }
    return NULL;
}

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
            // queue *temp_rq = ready_queue;
            while (cpu_running_thread_count > 0)
            {
                pthread_cond_wait(&ready_queue_cv, &ready_queue_mutex);
            }
            if (ready_queue->front != NULL)
            {
                current_cpu_pid = ready_queue->front->data.pid;
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
    pthread_mutex_init(&time_mutex, NULL);
    pthread_cond_init(&ready_queue_cv, NULL);
    pthread_cond_init(&ready_queue_io1_cv, NULL);
    pthread_cond_init(&ready_queue_io2_cv, NULL);
    scheduling_algorithm = FCFS;
    ready_queue = createQueue();
    burst_dist = "fixed";
    burstlen = 10;
    p0 = 0.3;
    max_burst = 60;
    min_burst = 30;
    p1 = 0.4;
    p2 = 0.3;
    maxp = 2;
    io1_count = 0;
    io2_count = 0;
    allp = 4;
    t1 = 5;
    t2 = 5;
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
    pthread_mutex_destroy(&time_mutex);
    pthread_cond_destroy(&ready_queue_io1_cv);
    pthread_cond_destroy(&ready_queue_io2_cv);
    pthread_cond_destroy(&ready_queue_cv);
    pthread_cond_destroy(&thread_cv);
    free(threads);
    destroyQueue(ready_queue);
    return 0;
}

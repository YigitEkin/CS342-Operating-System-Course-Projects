#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "defs.h"
#include <sys/time.h>
#include <math.h>
#include <string.h>

#define min(x, y) (((x) < (y)) ? (x) : (y))

// TODO: exponential distribution
// TODO: RR implementation
struct timeval start_time, current_time;
char *burst_dist;
SchedulingAlgorithm scheduling_algorithm;
int q, t1, t2, burstlen, min_burst, max_burst, maxp, allp, current_count, cpu_running_thread_count, thread_count;
int number_of_threads_left;
int current_cpu_pid, outmode;
int io1_count, io2_count;
float p0, p1, p2, pg;
long int start_time_val;
pthread_t *threads;
queue *ready_queue, *print_queue;
pthread_mutex_t ready_queue_mutex, ready_queue_io1_mutex, ready_queue_io2_mutex, time_mutex;
pthread_cond_t ready_queue_cv, thread_cv, ready_queue_io1_cv, ready_queue_io2_cv;

void calculate_next_burst_length(PCB *pcb, SchedulingAlgorithm scheduling_algorithm)
{
    if (scheduling_algorithm == RR)
    {
        if (pcb->remaining_burst_length == 0)
        {
            pcb->remaining_burst_length = q;
        }

        pcb->next_burst_length = min(q, pcb->remaining_burst_length);
    }
    else
    {
        if (strcmp(burst_dist, "fixed") == 0)
        {
            pcb->next_burst_length = burstlen;
        }
        else if (strcmp(burst_dist, "uniform") == 0)
        {
            pcb->next_burst_length = rand() % (max_burst - min_burst + 1) + min_burst;
        }
        else if (strcmp(burst_dist, "exponential") == 0)
        {
            pcb->next_burst_length =
                (1.0 / (log(-1 * min_burst * burstlen) - log(-1 * max_burst * burstlen))) * (log(-1 * burstlen * min_burst) - log(-1 * burstlen * (rand() / RAND_MAX)));
        }
    }
}

void *thread_function(void *arg)
{
    queue *temp_rq = ready_queue;
    PCB *pcb = (PCB *)arg;
    if (outmode == 3)
    {
        printf("Thread %d created\n", pcb->pid);
    }

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
    pcb->previous_burst_length = 0;
    pthread_mutex_lock(&time_mutex);
    pcb->last_ready_queue_enterance = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val);
    pthread_mutex_unlock(&time_mutex);
    pcb->remaining_burst_length = 0;
    calculate_next_burst_length(pcb, scheduling_algorithm);
    pcb->remaining_burst_length = pcb->next_burst_length;
    pthread_mutex_lock(&ready_queue_mutex);
    pthread_mutex_lock(&time_mutex);
    gettimeofday(&current_time, NULL);
    if (outmode == 3)
    {
        printf("Thread %d is added to the ready queue at time: %ld\n", pcb->pid, ((current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val)));
    }
    pthread_mutex_unlock(&time_mutex);
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
        pthread_mutex_unlock(&time_mutex);
        node *temp = dequeue(ready_queue);
        pthread_mutex_lock(&time_mutex);
        gettimeofday(&current_time, NULL);
        if (outmode == 3)
        {
            printf("Thread %d is selected for CPU at time: %ld\n", pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
        }
        pthread_mutex_unlock(&time_mutex);
        PCB *temp_pcb = &(temp->data);
        cpu_running_thread_count++;
        pthread_mutex_unlock(&ready_queue_mutex);
        pthread_cond_broadcast(&ready_queue_cv);

        if (scheduling_algorithm == RR)
        {
            temp_pcb->state = RUNNING;
            temp_pcb->previous_burst_length = pcb->next_burst_length;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->time_spent_in_ready_queue += (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - temp_pcb->last_ready_queue_enterance;
            pthread_mutex_unlock(&time_mutex);
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            temp_pcb->time_spent_in_cpu += temp_pcb->next_burst_length;
            pthread_mutex_lock(&time_mutex);
            if (outmode == 3)
            {
                printf("Thread %d is running at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            if (outmode == 2)
            {
                printf("%ld %d RUNNING\n", (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val, temp_pcb->pid);
            }
            pthread_mutex_unlock(&time_mutex);
            usleep(temp_pcb->next_burst_length * 1000);
            temp_pcb->remaining_burst_length -= temp_pcb->next_burst_length;
            pthread_mutex_lock(&ready_queue_mutex);
            if (temp_pcb->remaining_burst_length != 0) // bitiş
            {
                pthread_mutex_lock(&time_mutex);
                gettimeofday(&current_time, NULL);
                temp_pcb->last_ready_queue_enterance = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - temp_pcb->last_ready_queue_enterance;
                enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
                if (outmode == 3)
                {
                    printf("Thread %d is added to the ready queue at time: %ld\n", pcb->pid, ((current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val)));
                }
                pthread_mutex_unlock(&time_mutex);
                pthread_cond_broadcast(&ready_queue_cv);
                continue;
            }
            else
            {
                if (outmode == 3)
                {
                    printf("Q expired for thread %d at: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
                }
            }
            cpu_running_thread_count--;
            pthread_mutex_unlock(&ready_queue_mutex);
            pthread_cond_broadcast(&ready_queue_cv);
        }
        else
        {
            temp_pcb->state = RUNNING;
            temp_pcb->previous_burst_length = pcb->next_burst_length;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->time_spent_in_ready_queue = current_time.tv_sec * 1000 + current_time.tv_usec / 1000 - temp_pcb->last_ready_queue_enterance;
            pthread_mutex_unlock(&time_mutex);
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            temp_pcb->time_spent_in_cpu += temp_pcb->next_burst_length;
            pthread_mutex_lock(&time_mutex);
            if (outmode == 3)
            {
                printf("Thread %d is running at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            if (outmode == 2)
            {
                printf("%ld %d RUNNING\n", (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val, temp_pcb->pid);
            }
            usleep(temp_pcb->next_burst_length * 1000);
            pthread_mutex_unlock(&time_mutex);
            temp_pcb->remaining_burst_length = 0;
            cpu_running_thread_count--;
            pthread_cond_broadcast(&ready_queue_cv);
        }
        int prob = rand() % 100;
        if (prob < p0 * 100)
        {
            pcb->state = TERMINATED;
            temp_pcb->state = TERMINATED;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->finish_time = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - (start_time_val);
            pthread_mutex_unlock(&time_mutex);
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            if (outmode == 3)
            {
                printf("Thread %d is terminated at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }

            pthread_mutex_unlock(&time_mutex);
            enqueue(print_queue, *temp_pcb, FCFS);
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
            if (outmode == 3)
            {
                printf("Thread %d is using device 1 at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            if (outmode == 2)
            {
                printf("%ld %d USING DEVICE 1\n", (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val, temp_pcb->pid);
            }
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
            if (outmode == 3)
            {
                printf("Thread %d is finished using device 1 at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            pthread_mutex_unlock(&time_mutex);
            pthread_mutex_unlock(&ready_queue_io1_mutex);
            pthread_cond_signal(&ready_queue_io1_cv);
            pthread_mutex_lock(&ready_queue_mutex);
            temp_pcb->state = READY;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->last_ready_queue_enterance = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val;
            enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
            if (outmode == 3)
            {
                printf("Thread %d is added to the ready queue at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            pthread_mutex_unlock(&time_mutex);
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
            temp_pcb->state = USING_DEVICE_2;
            io2_count++;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            if (outmode == 3)
            {
                printf("Thread %d is using device 2 at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            if (outmode == 2)
            {
                printf("%ld %d USING DEVICE 2\n", (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val, temp_pcb->pid);
            }
            pthread_mutex_unlock(&time_mutex);
            usleep(t2 * 1000);
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            pthread_mutex_unlock(&time_mutex);
            temp_pcb->number_of_io_device2++;
            temp_pcb->number_of_cpu_bursts++;
            io2_count--;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            if (outmode == 3)
            {
                printf("Thread %d is finished using device 2 at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            pthread_mutex_unlock(&time_mutex);
            pthread_mutex_unlock(&ready_queue_io2_mutex);
            pthread_cond_signal(&ready_queue_io2_cv);
            pthread_mutex_lock(&ready_queue_mutex);
            temp_pcb->state = READY;
            pthread_mutex_lock(&time_mutex);
            gettimeofday(&current_time, NULL);
            temp_pcb->last_ready_queue_enterance = (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val;
            pthread_mutex_unlock(&time_mutex);
            enqueue(ready_queue, *temp_pcb, scheduling_algorithm);
            if (outmode == 3)
            {
                printf("Thread %d is added to the ready queue at time: %ld\n", temp_pcb->pid, (current_time.tv_sec * 1000 + current_time.tv_usec / 1000) - start_time_val);
            }
            pthread_mutex_unlock(&ready_queue_mutex);
            calculate_next_burst_length(temp_pcb, scheduling_algorithm);
            pthread_cond_broadcast(&ready_queue_cv);
        }
    }
    --number_of_threads_left;
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
            queue *temp_rq = ready_queue;
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

void calculate_scheduling_algorithm(char *alg)
{
    if (strcmp(alg, "FCFS") == 0)
    {
        scheduling_algorithm = FCFS;
    }
    else if (strcmp(alg, "SJF") == 0)
    {
        scheduling_algorithm = SJF;
    }
    else
    {
        scheduling_algorithm = RR;
    }
}

/*
systemsim <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst>
<max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>
*/
int main(int argc, char *args[])
{
    if (argc != 16)
    {
        printf("Invalid number of arguments\n");
        return 0;
    }
    pthread_mutex_init(&ready_queue_mutex, NULL);
    pthread_mutex_init(&ready_queue_io1_mutex, NULL);
    pthread_mutex_init(&ready_queue_io2_mutex, NULL);
    pthread_mutex_init(&time_mutex, NULL);
    pthread_cond_init(&ready_queue_cv, NULL);
    pthread_cond_init(&ready_queue_io1_cv, NULL);
    pthread_cond_init(&ready_queue_io2_cv, NULL);
    calculate_scheduling_algorithm(args[1]);
    if (scheduling_algorithm == RR)
    {
        q = atoi(args[2]);
    }
    else
    {
        q = 0;
    }

    t1 = atoi(args[3]);
    t2 = atoi(args[4]);
    burst_dist = (char *)malloc(sizeof(char) * strlen(args[5]));
    strcpy(burst_dist, args[5]);
    burstlen = atoi(args[6]);
    min_burst = atoi(args[7]);
    max_burst = atoi(args[8]);
    p0 = atof(args[9]);
    p1 = atof(args[10]);
    p2 = atof(args[11]);
    pg = atof(args[12]);
    maxp = atoi(args[13]);
    allp = atoi(args[14]);
    outmode = atoi(args[15]);
    ready_queue = createQueue();
    print_queue = createQueue();
    number_of_threads_left = allp;
    cpu_running_thread_count = 0;
    threads = (pthread_t *)malloc(sizeof(pthread_t) * allp);
    pthread_t generator, scheduler;
    pthread_create(&generator, NULL, &generator_syscall, NULL);

    pthread_create(&scheduler, NULL, &scheduler_func, NULL);
    pthread_join(scheduler, NULL);
    pthread_join(generator, NULL);
    for (size_t i = 0; i < maxp; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("\n");
    sortQueue(print_queue);
    printQueueData(print_queue);
    pthread_mutex_destroy(&ready_queue_io1_mutex);
    pthread_mutex_destroy(&ready_queue_io2_mutex);
    pthread_mutex_destroy(&ready_queue_mutex);
    pthread_mutex_destroy(&time_mutex);
    pthread_cond_destroy(&ready_queue_io1_cv);
    pthread_cond_destroy(&ready_queue_io2_cv);
    pthread_cond_destroy(&ready_queue_cv);
    pthread_cond_destroy(&thread_cv);
    free(threads);
    free(burst_dist);
    destroyQueue(ready_queue);
    destroyQueue(print_queue);
    return 0;
}

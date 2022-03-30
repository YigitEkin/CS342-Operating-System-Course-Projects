#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
typedef struct h
{
    int interval_count;
    int interval_width;
    int interval_start;
    int *hist;
} histogram;

histogram *hist;

void *handleHistogram(void *args)
{

    FILE *file = fopen((char *)args, "r");
    if (file == 0)
    {
        printf("invalid file");
        exit(1);
    }
    int n;
    int f;

    while ((f = fscanf(file, "%d\n", &n)) != EOF)
    {
        n -= hist->interval_start;
        n = n / hist->interval_width;
        if (n >= 0 && n < hist->interval_count)
        {
            ++hist->hist[n];
        }
    }
}

int main(int argc, char *args[])
{
    // get the histogram from the client with the message queue
    hist = malloc(sizeof(histogram));
    hist->interval_count = 10;
    hist->interval_start = 1;
    hist->interval_width = 1;
    hist->hist = malloc(sizeof(int) * hist->interval_count);

    pthread_t threads[argc - 1];

    for (size_t i = 0; i < argc - 1; i++)
    {
        if (pthread_create(threads + i, NULL, &handleHistogram, args[i + 1]) != 0)
        {
            printf("cannot create thread");
        }
    }

    for (size_t i = 0; i < argc - 1; i++)
    {
        if (pthread_join(*(threads + i), NULL) != 0)
        {
            printf("cannot join thread");
        }
    }

    for (size_t i = 0; i < hist->interval_count; i++)
    {
        printf("%d ", hist->hist[i]);
    }

    // send to client with message queue

    return 0;
}
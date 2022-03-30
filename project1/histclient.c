#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
typedef struct h
{
    int interval_count;
    int interval_width;
    int interval_start;
    int *hist;
} histogram;

void printHistogram(histogram *hist)
{
    for (size_t i = 0; i < hist->interval_count; i++)
    {
        printf("[%d, %d]: %d\n", (hist->interval_start + (i * hist->interval_width), (hist->interval_start + ((i + 1) * hist->interval_width)), hist->hist[i]));
    }
}

int main(int argc, char *args[])
{
    histogram *hist = malloc(sizeof(histogram));
    hist->interval_count = atoi(args[1]);
    hist->interval_width = atoi(args[2]);
    hist->interval_start = atoi(args[3]);
    hist->hist = malloc(sizeof(int) * hist->interval_count);
    // message queue ile yolla
    // message queuedan al
    // printle
    printHistogram(hist);
    // valid mq yolla
    return 0;
}
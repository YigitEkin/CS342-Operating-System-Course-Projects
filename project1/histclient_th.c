#include <stdio.h>
#include <stdlib.h>
typedef struct h
{
    int interval_count;
    int interval_width;
    int interval_start;
    int *hist;
} histogram;

void printHistogram(histogram *hist)
{
    int count = hist->interval_count;
    int current = hist->interval_start;
    int width = hist->interval_width;
    int end = current + width;
    int index = 0;
    while (index < count)
    {
        printf("[%d, %d]: %d\n", current, end, hist->hist[count]);
        current += width;
        end += width;
        ++index;
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
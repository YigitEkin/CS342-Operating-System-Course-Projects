#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include "shareddefs.h"

void printHistogram(int *hist, int count, int width, int start) {
    int end = start + width;
    int index = 0;
    while(index < count) {
        printf("[%d, %d]: %d\n", start, end, hist[index]);
        start += width;
        end += width;
        ++index;
    }
}

int main(int argc, char* args[]) {
    int histData[3];
    histData[0] = atoi(args[1]);
    histData[1] = atoi(args[2]);
    histData[2] = atoi(args[3]);
    int * hist = malloc(sizeof(int) * histData[0]);
    struct mq_attr mqAttr;
    mqAttr.mq_maxmsg = 2;
    mqAttr.mq_msgsize = sizeof(int) * 3;

    mqd_t mq = mq_open(mq1_name, O_RDWR | O_CREAT, 0666 ,&mqAttr);
    if (mq == -1) {
        perror("cannot create message queue0");
        exit(1);
    }

    int n = mq_send(mq, (char*) histData,
                    sizeof(int) * 3,
                    0);
    if (n == -1) {
        perror("mq send failed");
        exit(1);
    }
    //mq_close(mq);
    struct mq_attr attr;
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = sizeof(int) * histData[0];
    int x;
    mqd_t mq2 = mq_open(mq2_name, O_RDWR | O_CREAT , 0666 , &attr);
    if (mq2 == -1) {
        printf("cannot create message queue 1");
        exit(1);
    }


    int res = 1;
    x = mq_receive(mq2,(char*)hist, histData[0] * sizeof(int) , 0);
    if (x == -1) {
        perror("message receive failed1");
        res = 0;
    }
    printHistogram(hist, histData[0], histData[1], histData[2]);

    struct mq_attr attr3;
    attr.mq_maxmsg = 1;
    attr.mq_msgsize = sizeof(int);
    
    mqd_t valid = mq_open(mq3_name, O_RDWR | O_CREAT, 0666, &attr3);
    mq_send(valid, (char*)&res, sizeof(int), 0);
    mq_close(mq);
    mq_close(mq2);
    mq_close(valid);
    mq_unlink(mq1_name);
    mq_unlink(mq2_name);
    mq_unlink(mq3_name);
    free(hist);
    return 0;
}

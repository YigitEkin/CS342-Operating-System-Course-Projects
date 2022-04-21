#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <pthread.h>
#include "shareddefs.h"

int *hist;
int histData[3];
void *handleHistogram(void * args) {
    FILE *file = fopen((char*)args, "r");
    int n;
    if (!file) {
        printf("invalid file");
        exit(1);
    }
    while(fscanf(file,"%d\n", &n) != EOF) {
    //printf("val= %d\n", n);
    	if(n >= histData[2] && n < (histData[1]*histData[2]*histData[0])){
        	n -= histData[2];
        	n = n/ histData[1];
        	++hist[n];
        }
    }
    fclose(file);
    return NULL;
}

int main(int argc, char* args[]) {
    struct mq_attr mqAttr;
    mqAttr.mq_maxmsg = 2;
    mqAttr.mq_msgsize = sizeof(int) * 3;
    mqd_t mq = mq_open(mq1_name, O_RDWR | O_CREAT, 0666 ,&mqAttr);
    struct mq_attr attr;
    if (mq == -1) {
        perror("cannot create msg queue 0s");
    }
    
    //printf("burda");
    int n = mq_receive(mq, (char*) histData
                       , sizeof(int) * 3 , NULL);
    //printf("data 1:%d 2:%d, 3:%d", histData[0],histData[1],histData[2]);
    if (n == -1) {
        //printf("burda3");
        perror("failed mq receive 1s");
        exit(1);
    }
    hist = (int*) malloc(sizeof(int) * histData[0]);
    pthread_t threads[argc-1];

    for (int i = 0; i < argc-2; ++i) {
        if (pthread_create(threads + i, NULL, &handleHistogram, args[i + 2]) != 0) {
            printf("cannot create thread");
        }
    }

    for (int i = 0; i < argc-2; ++i) {
        if (pthread_join(*(threads + i), NULL) != 0) {
            printf("cannot join thread");
        }
    }



    attr.mq_maxmsg = 2;
    attr.mq_msgsize = sizeof(int) * histData[0];
    mqd_t mq2 = mq_open(mq2_name, O_RDWR | O_CREAT,  0666 ,&attr);
    if (mq2 == -1) {
        perror("cannot open msq queue 2s");
        exit(1);
    }
/*
  mq_close(mq);
    mq_close(mq2);
    mq_unlink(mq1_name);
    mq_unlink(mq2_name);
    exit(1);
    */

    int x = mq_send(mq2,(char*)hist,sizeof(int) * histData[0], 0);
    if ( x == -1) {
        perror("cannot send msq 2s");
        exit(1);
    }
    mq_close(mq2);
    struct mq_attr attr1;
    attr1.mq_maxmsg = 2;
    attr1.mq_msgsize = sizeof(int);
    mqd_t valid = mq_open(mq3_name, O_RDWR | O_CREAT, 0666, &attr1);



    int* bufptr2 = (int*) malloc(attr1.mq_msgsize);
    int a = mq_receive(valid, (char*)bufptr2, attr1.mq_msgsize, NULL);
    if (a ==-1) {
        printf("cannot recevie validation signal");
    }
    if (valid == -1) {
        perror("cannot open msq 3");
        exit(1);
    }
    if(*bufptr2 == 1) {
        printf("confirmation message received\n");
    }
    mq_close(mq);
    mq_close(mq2);
    mq_close(valid);
    mq_unlink(mq1_name);
    mq_unlink(mq2_name);
    mq_unlink(mq3_name);
    free(hist);
    free(bufptr2);
}

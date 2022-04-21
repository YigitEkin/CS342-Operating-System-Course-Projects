#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include "shareddefs.h"


void handleHistogram(int * hist, char * filename, int* histData) {
    for (int i = 0; i < histData[0]; ++i) {
        hist[i] = 0;
    }
    FILE *file = fopen(filename, "r");
    int n;
    if (!file) {
        printf("invalid file %s", filename);
        exit(1);
    }
    while(fscanf(file,"%d\n", &n) != EOF) {
        if (n >= histData[2] && n < (histData[1] * histData[2]) * histData[0]) {
            n -= histData[2];
            n = n/ histData[1];
        

            ++hist[n];
        }
    }
    fclose(file);
}

int main(int argc, char* args[]) {
    int *hist, *hist2;
    char* files[argc-2];
    int histData[3];
    struct mq_attr mqAttr;
    mqAttr.mq_maxmsg = 2;
    mqAttr.mq_msgsize = sizeof(int) * 3;
    mqd_t mq = mq_open(mq1_name, O_RDWR | O_CREAT, 0666 ,&mqAttr);

    if (mq == -1) {
        perror("cannot create msg queue 0s");
    }
    
    int n = mq_receive(mq, (char*) histData
                       , sizeof(int) * 3 , NULL);
    //printf("data 1:%d 2:%d, 3:%d\n", histData[0],histData[1],histData[2]);
    if (n == -1) {
        perror("failed mq receive 1s");
        exit(1);
    }
    struct mq_attr attr;
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = (sizeof(int) * (histData[0]));
    for (int i = 0; i < argc-2; ++i) {
        files[i] = args[i + 2];
    }

    hist = (int*) malloc(sizeof(int) * histData[0]);
    hist2 = (int*) malloc(sizeof(int) * histData[0]);



    int processes[argc-2];
    mqd_t mqchild = mq_open(mqp_name, O_RDWR | O_CREAT,
                             0666, &attr);
    if (mqchild == -1) {
        perror("cannot create parent mq");
        exit(1);
    }


    for (int i = 0; i < argc - 2; ++i) {
        processes[i] = fork();

        if (processes[i] == 0) {
            handleHistogram(hist2, files[i],histData);
            mq_send(mqchild, (char*)hist2,
                    (sizeof(int) * (histData[0])), 0);
            exit(1);
        } else {
            mq_receive(mqchild, (char*) hist2,
                       (sizeof(int) * histData[0]),0);
            for (int j = 0; j < histData[0]; ++j) {
                hist[j] += hist2[j];
            }
        }
    }
//////sdfghjklşkjhgfdfghjklkjhgfdfghjkl********
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = sizeof(int) * histData[0];
    mqd_t mq2 = mq_open(mq2_name, O_RDWR | O_CREAT,  0666 ,&attr);
    if (mq2 == -1) {
        perror("cannot open msq queue 2s");
        exit(1);
    }


    int x = mq_send(mq2,(char*)hist,
                    sizeof(int) * histData[0], 0);
    if ( x == -1) {
        perror("cannot send msq 2s");
        exit(1);
    }
    struct mq_attr attr1;
    attr1.mq_maxmsg = 2;
    attr1.mq_msgsize = sizeof(int);
    mqd_t valid = mq_open(mq3_name, O_RDWR | O_CREAT, 0666, &attr1);

    int* bufptr2 = (int*) malloc(attr1.mq_msgsize);
    //printf("here");
        int a = mq_receive(valid, (char*)bufptr2, attr1.mq_msgsize, NULL);
        if (a == -1) {
            perror("cannot receive confirmation");
            exit(1);
        }
    //printf("her");
    if (valid == -1) {
        perror("cannot open msq 3");
        exit(1);
    }
    if(*bufptr2 == 1) {
        printf("confirmation message received");
    }

    mq_close(mq);
    mq_close(mq2);
    mq_close(valid);
    mq_unlink(mq1_name);
    mq_unlink(mq2_name);
    mq_unlink(mq3_name);
    mq_close(mqchild);
    mq_unlink(mqp_name);
    free(hist2);
    free(hist);
    free(bufptr2);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include "dma.h"
// include the library interface
/*
    void *p1;
    void *p2;
    void *p3;
    void *p4;
    int ret;
    ret = dma_init (20); // create a segment of 1 MB
    if (ret != 0) {
        printf ("something was wrong\n");
exit(1); }
    p1 = dma_alloc (100); // allocate space for 100 bytes
    p2 = dma_alloc (1024);
    p3 = dma_alloc (64); //always check the return value
    p4 = dma_alloc (220);
    or (int i = 0; i < 5; i++)
    {
        printf("\nPrinting page %d\n", i);
        dma_print_page(i);
    }
    dma_free (p3);
    p3 = dma_alloc (2048);
    dma_print_blocks();
    dma_free (p1);
    dma_free (p2);
    dma_free (p3);
    dma_free (p4);
*/


int main (int argc, char ** argv)
{
    /*
    void *p1;
    void *p2;
    void *p3;
    void *p4;

    int ret;
    ret = dma_init (14); // create a segment of 1 MB
    if (ret != 0) {
        printf ("something was wrong\n");
    exit(1); }

    printf("Internal Fragmentation Test Case: Without internal fragmentation\n");
    p1 = dma_alloc (8192); // allocate space for 100 bytes
    p2 = dma_alloc (4096);
    p3 = dma_alloc (768); //always check the return value
    p4 = dma_alloc (1024);
    dma_print_blocks();
    printf("%d\n", dma_give_intfrag());
    //dma_print_bitmap();
    dma_free (p1);
    dma_free (p2);
    dma_free (p3);
    dma_free (p4);
    printf("Internal Fragmentation Test Case: With internal fragmentation\n");
    p1 = dma_alloc (8182); // allocate space for 100 bytes
    p2 = dma_alloc (4086);
    p3 = dma_alloc (758); //always check the return value
    p4 = dma_alloc (1014);
    dma_print_blocks();
    printf("%d\n", dma_give_intfrag());
    dma_free (p1);
    dma_free (p2);
    dma_free (p3);
    dma_free (p4);

    printf("External Fragmentation Test Case:\n");
    p1 = dma_alloc (4096); // allocate space for 100 bytes
    p2 = dma_alloc (1024);
    p3 = dma_alloc (2048); //always check the return value
    p4 = dma_alloc (512);
    printf("Before freeing p2 of size 1024\n");
    dma_print_blocks();
    //dma_print_bitmap();
    dma_free (p2);
    printf("After freeing p2 of size 1024\n");
    dma_print_blocks();
    p2 = dma_alloc (512);
    printf("After allocating p2 of size 512\n");
    dma_print_blocks();
    void* p5;
    p5 = dma_alloc(6912); //Expected to fill the memory but not able to allocate because of external fragmentation
    printf("After trying to allocating p5 of size 6912\n");
    dma_print_blocks();
    p5 = dma_alloc(2048);
    printf("After allocating p5 of size 2048\n");
    dma_print_blocks();
    dma_free (p1);
    dma_free (p2);
    dma_free (p3);
    dma_free (p4);
    dma_free (p5);*/

    int ret;
    ret = dma_init (14); // create a segment of 1 MB
    if (ret != 0) {
        printf ("something was wrong\n");
    exit(1); }

    void *p1;
    void *p2;
    void *p3;
    void *p4;

    struct timeval start, current;
    double diff;

    printf("Allocation/Free Timing Test Case:\n");
    gettimeofday(&start, NULL);
    p1 = dma_alloc (8192);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for allocation of 8192 bytes: %f\n", diff);

    gettimeofday(&start, NULL);
    p2 = dma_alloc (4096);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for allocation of 4096 bytes: %f\n", diff);

    gettimeofday(&start, NULL);
    p3 = dma_alloc (1024);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for allocation of 1024 bytes: %f\n", diff);

    gettimeofday(&start, NULL);
    p4 = dma_alloc (512);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for allocation of 512 bytes: %f\n", diff);
    
    dma_print_blocks();

    gettimeofday(&start, NULL);
    dma_free (p1);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for freeing of 8192 bytes: %f\n", diff);

    gettimeofday(&start, NULL);
    dma_free (p2);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for freeing of 4096 bytes: %f\n", diff);

    gettimeofday(&start, NULL);
    dma_free (p3);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for freeing of 1024 bytes: %f\n", diff);

    gettimeofday(&start, NULL);
    dma_free (p4);
    gettimeofday(&current, NULL);
    diff = (current.tv_sec * 1000.0 + current.tv_usec/1000.0) - (start.tv_sec * 1000.0 + start.tv_usec/1000.0);
    printf("Time taken for freeing of 512 bytes: %f\n", diff);


    dma_print_blocks();
    
    return 0;
}
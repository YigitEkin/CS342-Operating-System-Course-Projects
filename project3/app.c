#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
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
    dma_free (p3);
    p3 = dma_alloc (2048);
    dma_print_blocks();
    dma_free (p1);
    dma_free (p2);
    dma_free (p3);
    dma_free (p4);
*/

int bin_to_hex(int bin){
    int hex = 0;
    int i = 0;
    while(bin != 0){
        hex += (bin % 10) * pow(2,i);
        bin /= 10;
        i++;
    }
    return hex;
}

int main (int argc, char ** argv)
{
    printf("%X", bin_to_hex(10001111));
    return 0;
}
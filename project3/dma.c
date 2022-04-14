#include "dma.h"
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#define WORD_SIZE 8;
#define PAGE_SIZE 4096;
#define RESERVED_AREA_SIZE 256;
#define BITMAP_RESERVED_AREA_SIZE 32;

int* segment;
size_t size, internal_fragmentation_size, bitmap_size; //bitmap size is in  bits 
pthread_mutex_t segment_mutex, internal_fragmentation_mutex;

//TODO: helper to print in hex which will be used for printing methods
//Write a method that converts a binary number to a hexadecimal number and returns it as a integer.
int bin_to_hex(char* str){
    int bin = atoi(str);
    int hex = 0;
    int i = 0;
    while(bin != 0){
        hex += (bin % 10) * pow(2,i);
        bin /= 10;
        i++;
    }
    return hex;
}

int calculate_closest_to16(int asked_amount) {
    double division = (double)(asked_amount) / 16.0;
    double multiply_constant = ceil(division);
    return (int)(multiply_constant * 16);
}


int check_free_space(int curr_index, int asked_amount){
    int i = 0;
    while (i < asked_amount/WORD_SIZE) {
        if (segment[curr_index + i] == 0) {
            return 0;
        }
        i++;
    }
    return 1;
}

size_t find_free_space(int asked_amount){
    size_t index = BITMAP_RESERVED_AREA_SIZE + bitmap_size/WORD_SIZE;
    while (index < bitmap_size) {
        if (segment[index] == 1 && segment[index + 1] == 1) {
            if (check_free_space(index, asked_amount)) {
                return index;
            }
        }
        if (segment[index] == 1 && segment[index - 1] == 0) {
            index++;
            while (segment[index] != 1) {
                index++;
            }
            continue;
        }
        index++;            
    }
    return NULL;
} 
    
int dma_init (int m) {
    //TODO: fix size
    pthread_mutex_init(&segment_mutex, NULL);
    pthread_mutex_init(&internal_fragmentation_mutex, NULL);
    segment = mmap(0, pow(2,m), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (segment == MAP_FAILED) {
        size = 0;
        return -1;
    }
    size = pow(2,m);
    bitmap_size = size / WORD_SIZE;

    //! MAP BITMAP REGION TO BITMAP  
    segment[0] = 0; segment[1] = 1; //as the bitmap is allocated, the first 2 bits will be 01
    for (size_t i = 2; i < bitmap_size; i++)
    {
        segment[i] = 0;
    }
    
    //!MAP SHARED AREA REGION TO BITMAP 
    segment[bitmap_size] = 0; segment[bitmap_size + 1] = 1;
    for (size_t i = 0; i < 30; i++)
    {
        segment[bitmap_size + 2 + i] = 0;
    }

    //! MAP ALLOCABLE AREA REGION TO BITMAP 
    for (size_t i = bitmap_size + 32; i < bitmap_size; i++)
    {
        segment[i] = 1;
    }
    
    //add bitmap, reserved area and initiliaze the pages
    internal_fragmentation_size = 0;
    return 0;
}

void *dma_alloc (int size) {
    int size_to_allocate = calculate_closest_to16(size);
    int bitmap_index = find_free_space(size_to_allocate);
    segment[bitmap_index] = 0;
    segment[bitmap_index + 1] = 1;
    for (size_t i = 2; i < size_to_allocate/WORD_SIZE; i++)
    {
        segment[bitmap_index + i] = 0;
    }
    
    //TODO: return address from allocated space
    internal_fragmentation_size += (size_to_allocate - size);
    //search for the first free page
    //update the bitmap
    //update internal fragmentation size
}
void  dma_free (void *p) {
    //search for the first freeable page
    //update the bitmap
    //update internal fragmentation size
}
void  dma_print_page(int pno){ 
    //print the page pno
    size_t page_starting_index = pno * PAGE_SIZE;

    //TODO: create 64 hexadecimal block to print
    size_t index = 0;
}
void  dma_print_bitmap(){ 
    pthread_mutex_lock(&segment_mutex);
    for (size_t i = 0; i < size; i++)
    {
        if ((i % 64 == 0) &&  (i !== 0)) {
            printf("\n");        
        } else if (i % 8 == 0 && (i !== 0)) {
            printf( " ");
        }
        printf("%d", segment[i]);
    }
    pthread_mutex_unlock(&segment_mutex);
}
void  dma_print_blocks(){ 

}

int dma_give_intfrag(){ 
    pthread_mutex_lock(&internal_fragmentation_mutex);
    int val_to_return = internal_fragmentation_size;
    pthread_mutex_unlock(&internal_fragmentation_mutex);
    return val_to_return;
}
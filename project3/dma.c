#include "dma.h"
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define WORD_SIZE 8;
#define PAGE_SIZE 4096;
#define RESERVED_AREA_SIZE 256;
#define BITMAP_RESERVED_AREA_SIZE 32;

char* segment;
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
        if (strcmp(segment[curr_index + i], "0") == 0) {
            return 0;
        }
        i++;
    }
    return 1;
}

//TODO: can be implemented more efficiently
size_t find_free_space(int asked_amount){
    size_t index = BITMAP_RESERVED_AREA_SIZE + bitmap_size/WORD_SIZE;
    while (index < bitmap_size) {
        if (strcmp(segment[index], "1") == 0 && strcmp(segment[index + 1], "1") == 0) {
            if (check_free_space(index, asked_amount)) {
                return index;
            }
        }
        if (strcmp(segment[index], "1") == 0 && strcmp(segment[index - 1], "0") == 0) {
            index++;
            while (strcmp(segment[index], "1") != 0) {
                index++;
            }
            continue;
        }
        index++;            
    }
    return NULL;
} 
    
int dma_init (int m) {
    pthread_mutex_init(&segment_mutex, NULL);
    pthread_mutex_init(&internal_fragmentation_mutex, NULL);
    segment = mmap(0, pow(2,m), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (segment == MAP_FAILED) {
        size = 0;
        return -1;
    }
    size = pow(2,m);
    bitmap_size = size / WORD_SIZE;
    size_t bitmap_mappable_size = bitmap_size / WORD_SIZE;

    // MAP BITMAP REGION TO BITMAP  
    segment[0] = "0"; segment[1] = "1"; 
    for (size_t i = 2; i < bitmap_mappable_size; i++)
    {
        segment[i] = "0";
    }
    
    // MAP SHARED AREA REGION TO BITMAP 
    segment[bitmap_mappable_size] = "0"; segment[bitmap_mappable_size + 1] = "1";
    for (size_t i = 0; i < 30; i++)
    {
        segment[bitmap_mappable_size + 2 + i] = "0";
    }

    // MAP ALLOCABLE AREA REGION TO BITMAP 
    for (size_t i = bitmap_mappable_size + 32; i < bitmap_size; i++)
    {
        segment[i] = "1";
    }
    internal_fragmentation_size = 0;
    return 0;
}

void *dma_alloc (int size) {
    int size_to_allocate = calculate_closest_to16(size);
    pthread_mutex_lock(&segment_mutex);
    int sharedmem_bitmap_index = find_free_space(size_to_allocate);
    if (sharedmem_bitmap_index == NULL) {
        pthread_mutex_unlock(&segment_mutex);
        return NULL;
    }
    segment[sharedmem_bitmap_index] = "0";
    segment[sharedmem_bitmap_index + 1] = "1";
    for (size_t i = 2; i < size_to_allocate/WORD_SIZE; i++)
    {
        segment[sharedmem_bitmap_index + i] = "0";
    }
    internal_fragmentation_size += (size_to_allocate - size);

    size_t sharedmem_bitmap_start_index = BITMAP_RESERVED_AREA_SIZE + bitmap_size/WORD_SIZE;
    size_t sharedmem_displacement = sharedmem_bitmap_index - sharedmem_bitmap_start_index;
    int* return_val =  segment + sharedmem_displacement + bitmap_size + 32;
    pthread_mutex_unlock(&segment_mutex);
    return (void*) return_val;
}

//! There is a bug when two allocated blocks are located next to each other
//! also edit internal fragmentation size
void  dma_free (void *p) {
    pthread_mutex_lock(&segment_mutex);
    size_t sharedmem_start_index = bitmap_size + 32;
    size_t sharedmem_displacement = (char*)p - (segment  + sharedmem_start_index);
    size_t sharedmem_bitmap_start_index = BITMAP_RESERVED_AREA_SIZE + bitmap_size/WORD_SIZE;
    sharedmem_displacement += sharedmem_bitmap_start_index;

    if (strcmp(segment[sharedmem_displacement], "0") == 0
     && strcmp(segment[sharedmem_displacement + 1], "1") == 0 ) {
        segment[sharedmem_displacement] = "1";
    }
    else {
        pthread_mutex_unlock(&segment_mutex);
        return NULL;
    }
    
    int i = 1;
    while (strcmp(segment[sharedmem_displacement + (2*i)], "0") == 0 &&
     sstrcmp(segment[sharedmem_displacement + (2*i) + 1], "0") == 0) {
        segment[sharedmem_displacement + (2 * i)] = "1";
        segment[sharedmem_displacement + (2 * i) + 1] = "1";
        i++;
    } //if a bitwise pair is 01 or 11, loop terminates which means that the block is freed
    pthread_mutex_unlock(&segment_mutex);
}

//! What amount of data should be printed needs to be fixed
void  dma_print_page(int pno){ 
    //print the page pno
    size_t page_starting_index = pno * PAGE_SIZE;
    size_t index = 0;
    //create a string to print
    char str[64];
    //while index < 64 append segment[index] to str
    while (index < 64 && index < size) {
        pthread_mutex_lock(&segment_mutex);
        str[index] = segment[page_starting_index + index];
        index++;
        pthread_mutex_unlock(&segment_mutex);
    }
    bin_to_hex(str);
}

void  dma_print_bitmap(){ 
    pthread_mutex_lock(&segment_mutex);
    for (size_t i = 0; i < bitmap_size; i++)
    {
        if ((i % 64 == 0) &&  (i != 0)) {
            printf("\n");        
        } else if (i % 8 == 0 && (i != 0)) {
            printf( " ");
        }
        printf("%s", segment[i]);
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
#include "dma.h"
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define WORD_SIZE 8;
#define PAGE_SIZE 4096;
#define RESERVED_AREA_SIZE 256;
#define BITMAP_RESERVED_AREA_SIZE 32;

double* segment;
size_t size, internal_fragmentation_size, bitmap_size; //bitmap size is in  bits 
pthread_mutex_t segment_mutex, internal_fragmentation_mutex;

//TODO: helper to print in hex which will be used for printing methods
//Write a method that converts a binary number to a hexadecimal number and returns it as a integer.
void bin_to_hex(int* page){
    int hex = 0;
    int index = 0;
    while (index < 4096)
    {
        hex = page[index] * pow(2, 3)+
        page[index+1] * pow(2, 2)+
        page[index+2] * pow(2, 1)+
        page[index+3] * pow(2, 0);
        index += 4;

        printf("%x", hex);
        if ((index) % 256 == 0)
        {
            printf("\n");
        }
    }
}

int calculate_closest_to16(int asked_amount) {
    double division = (double)(asked_amount) / 16.0;
    double multiply_constant = ceil(division);
    return (int)(multiply_constant * 16); 
}


int check_free_space(int curr_index, int asked_amount){
    int i = 0;
    while (i < asked_amount/8) {
        if (segment[curr_index + i] == 0) {
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
    pthread_mutex_init(&segment_mutex, NULL);
    pthread_mutex_init(&internal_fragmentation_mutex, NULL);
    segment = mmap(0, pow(2,m) * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (segment == MAP_FAILED) {
        size = 0;
        return -1;
    }
    size = pow(2,m);
    bitmap_size = size / WORD_SIZE;
    size_t bitmap_mappable_size = bitmap_size / WORD_SIZE;

    // MAP BITMAP REGION TO BITMAP  
    segment[0] = 0; segment[1] = 1; 
    for (size_t i = 2; i < bitmap_mappable_size; i++)
    {
        segment[i] = 0;
    }
    
    // MAP SHARED AREA REGION TO BITMAP 
    segment[bitmap_mappable_size] = 0; segment[bitmap_mappable_size + 1] = 1;
    for (size_t i = 0; i < 30; i++)
    {
        segment[bitmap_mappable_size + 2 + i] = 0;
    }

    // MAP ALLOCABLE AREA REGION TO BITMAP 
    for (size_t i = bitmap_mappable_size + 32; i < bitmap_size; i++)
    {
        segment[i] = 1;
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
    segment[sharedmem_bitmap_index] = 0;
    segment[sharedmem_bitmap_index + 1] = 1;
    for (size_t i = 2; i < size_to_allocate/8; i++)
    {
        segment[sharedmem_bitmap_index + i] = 0;
    }
    internal_fragmentation_size += (size_to_allocate - size);

    size_t sharedmem_bitmap_start_index = BITMAP_RESERVED_AREA_SIZE + bitmap_size/WORD_SIZE;
    size_t sharedmem_displacement = sharedmem_bitmap_index - sharedmem_bitmap_start_index;
    double* return_val =  segment + sharedmem_displacement + bitmap_size + 32;
    pthread_mutex_unlock(&segment_mutex);

    return (void*) return_val;
}

void  dma_free (void *p) {
    pthread_mutex_lock(&segment_mutex);
    size_t sharedmem_start_index = bitmap_size + 32;
    size_t sharedmem_displacement = (double*)p - (segment  + sharedmem_start_index);
    size_t sharedmem_bitmap_start_index = BITMAP_RESERVED_AREA_SIZE + bitmap_size/WORD_SIZE;
    sharedmem_displacement += sharedmem_bitmap_start_index;

    if (segment[sharedmem_displacement] == 0 && segment[sharedmem_displacement + 1] == 1) {
        segment[sharedmem_displacement] = 1;
    }
    else {
        pthread_mutex_unlock(&segment_mutex);
        return;
    }
    
    int i = 1;
    while (segment[sharedmem_displacement + (2*i)] == 0 && segment[sharedmem_displacement + (2*i) + 1] == 0 && sharedmem_displacement + (2*i) < bitmap_size) {
        segment[sharedmem_displacement + (2 * i)] = 1;
        segment[sharedmem_displacement + (2 * i) + 1] = 1;
        i++;
    } //if a bitwise pair is 01 or 11, loop terminates which means that the block is freed
    pthread_mutex_unlock(&segment_mutex);
}

//! What amount of data should be printed needs to be fixed
void  dma_print_page(int pno){ 
    //print the page pno
    size_t page_starting_index = pno * 4096;
    size_t index = 0;
    //create a string to print
    int page[4096];
    //while index < 64 append segment[index] to str
    while (index < 4096 && (page_starting_index + index) < size) {
        pthread_mutex_lock(&segment_mutex);
        //printf("%d", (int)segment[page_starting_index + index]);
        page[index] = (unsigned int)segment[page_starting_index + index];
        index++;
        pthread_mutex_unlock(&segment_mutex);
    }
    bin_to_hex(page);
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
        printf("%d", (int)segment[i]);
    }
    pthread_mutex_unlock(&segment_mutex);
    printf("\n\n");
}

void  dma_print_blocks(){ 
    size_t sharedmem_start_index = bitmap_size + 32;
    size_t sharedmem_bitmap_start_index = 32 + bitmap_size/8;
    size_t current_index = sharedmem_bitmap_start_index;
    int current_block_size = 0;

    pthread_mutex_lock(&segment_mutex);
    while (current_index < bitmap_size) {
        //01 se (11 / 01) görene kadar 2 şerli arttır
        //11 se 11 bitene kadar arttır   
        if ( (segment[current_index] == 0) &&
             (segment[current_index + 1] == 1)) {
            printf("A, %p, ", segment + sharedmem_start_index + current_index);

            current_block_size += 2;
            current_index += 2;

            while ((segment[current_index]== 0) &&
            (segment[current_index + 1]== 0) && current_index < bitmap_size) 
            {
                current_index += 2;
                current_block_size += 2;
            }

            printf("0x%x, (%d)\n", (current_block_size * 8), (current_block_size * 8));

            current_block_size = 0;
            continue;

        } else {
            printf("F, %p, ", segment + sharedmem_start_index + current_index);
            while ((segment[current_index] == 1) &&
                   (segment[current_index + 1] == 1) && current_index < bitmap_size) {
                current_block_size += 2;
                current_index += 2;
            }
            //print the free block
            printf("0x%x, (%d)\n", (current_block_size * 8), (current_block_size * 8));

            current_block_size = 0;
            continue;
        }
        
    }    
    pthread_mutex_unlock(&segment_mutex);
}



int dma_give_intfrag(){ 
    pthread_mutex_lock(&internal_fragmentation_mutex);
    int val_to_return = internal_fragmentation_size;
    pthread_mutex_unlock(&internal_fragmentation_mutex);
    return val_to_return;
}

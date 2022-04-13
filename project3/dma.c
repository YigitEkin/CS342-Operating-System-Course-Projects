#include "dma.h"
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#define WORD_SIZE 8;
#define PAGE_SIZE 4096;
#define RESERVED_AREA_SIZE 256;

int* segment;
size_t size, internal_fragmentation_size, bitmap_size; //bitmap size is in  bits 
pthread_mutex_t segment_mutex, internal_fragmentation_mutex;

//TODO: helper to print in hex which will be used for printing methods
int convert_to_hex(char* str) {
    int no = atoi(str);
    int power_of_2 = 0;
    int val_to_return = 0; 
    if (no == 0)
    {
        return 0;
    }
    
    while (no != 0) {
        val_to_return += pow;
    }
}

int calculate_closest_to16(int asked_amount) {
    double division = (double)(asked_amount) / 16.0;
    double multiply_constant = ceil(division);
    return (int)(multiply_constant * 16);
}

int dma_init (int m) {
    //TODO: fix size
    pthread_mutex_init(&segment_mutex);
    pthread_mutex_init(&internal_fragmentation_mutex);
    segment = mmap(0, pow(2,m), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (segment == MAP_FAILED) {
        size = 0;
        return -1;
    }
    size = pow(2,m);
    bitmap_size = size / WORD_SIZE;
    size_t number_of_map_bits_for_bitmap = bitmap_size / pow(2,6);

    //! MAP BITMAP REGION TO BITMAP  
    segment[0] = 0; segment[1] = 1; //as the bitmap is allocated, the first 2 bits will be 01
    for (size_t i = 2; i < number_of_map_bits_for_bitmap; i++)
    {
        segment[i] = 0;
    }
    
    //!MAP SHARED AREA REGION TO BITMAP 
    segment[number_of_map_bits_for_bitmap] = 0; segment[number_of_map_bits_for_bitmap + 1] = 1;
    for (size_t i = 0; i < 8; i++)
    {
        segment[number_of_map_bits_for_bitmap + 2 + i] = 0;
    }

    //! MAP ALLOCABLE AREA REGION TO BITMAP 
    for (size_t i = number_of_map_bits_for_bitmap + 8; i < bitmap_size; i++)
    {
        segment[i] = 1;
    }
    
    //add bitmap, reserved area and initiliaze the pages
    internal_fragmentation_size = 0;
    return 0;
}

void *dma_alloc (int size) {
    int size_to_allocate = calculate_closest_to16(size);
    for (size_t i = 0; i < bitmap_size; i++)
    {
        //TODO: search for size_to_allocate amount of consecutive 1 bits
    }
    
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
    for (size_t i = 1; i < size + 1; i++)
    {
        if (i % 65 == 0) {
            printf("\n");        
        } else if (i % 9 == 0) {
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
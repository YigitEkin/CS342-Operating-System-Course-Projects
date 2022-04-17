#ifndef DMA_H
#define DMA_H
int   dma_init (int m); //done
void *dma_alloc (int size); //done
void  dma_free (void *p); //done
void  dma_print_page(int pno);
void  dma_print_bitmap(); //done
void  dma_print_blocks(); //done
int   dma_give_intfrag(); //done
#endif
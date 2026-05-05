#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

void kheap_init();
void *kmalloc(uint32_t size);
int kfree(void *ptr);
void kheap_reg_dump();

#endif

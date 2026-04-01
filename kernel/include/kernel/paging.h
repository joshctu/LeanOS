# ifndef PAGING_H_
# define PAGING_H_

#include <stdint.h>

typedef uint32_t page_table_t[1024] __attribute__((aligned(4096)));
typedef uint32_t page_directory_t[1024] __attribute__((aligned(4096)));
void paging_init(void);
void paging_enable();
void map_higher_half_kernel();

# endif



#ifndef __SH_HEAP_H__
#define __SH_HEAP_H__

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include "sh_comm_def.h"


int sh_heap_init(char *buffer, unsigned long size);
int sh_heap_clear();
int sh_heap_attach(char *buffer, unsigned long size);
sh_ptr_common_t sh_malloc(unsigned long size, int id);
sh_ptr_common_t sh_remalloc(sh_ptr_common_t old_ptr, unsigned long size, int id);
int sh_free(sh_ptr_common_t ptr);
int sh_get_size_info(unsigned long *tot_size, unsigned long *free_size);
int sh_dump_info(char *out_file);
int sh_save_record(sh_ptr_common_t ptr, int id);
sh_ptr_common_t sh_get_record(int id);


#endif

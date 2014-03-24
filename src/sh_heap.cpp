#include "sh_heap.h"
#include "sh_list.h"

#define HEAP_INIT_CODE 0x7b9a1c3d6e2f4321ULL

#define HEAP_VERSION 1.0
#define HEAP_OFFSET 16
#define HASH_SIZE 32
#define HASH_MASK ((HASH_SIZE) - 1)

#pragma pack(1)

/////////////////////////////////////comm start/////////////////////////////////////////
typedef struct
{
	unsigned long cur_size;//size of cur fragment and flag(the low 3 bits)
	unsigned long pre_size;//size of the front fragment in physical memory. if there is no fragment in the front, then 0
	sh_list_node_t link[1];
} sh_block_t;
#define BLOCK_INFO_SIZE (sh_offsetof(sh_block_t, link))




//the block is busy
#define sh_block_flag_busy (1 << 0)
//the block is fragment
#define sh_block_flag_fragment (1 << 1)
//the block is eof
#define sh_block_flag_eof (1 << 2)

#define sh_block_set_busy_flag(ptr) (sh_set_flag((ptr)->cur_size, sh_block_flag_busy))
#define sh_block_del_busy_flag(ptr) (sh_del_flag((ptr)->cur_size, sh_block_flag_busy))
#define sh_block_is_busy(ptr) (sh_is_flag_set((ptr)->cur_size, sh_block_flag_busy))

#define sh_block_set_fragment_flag(ptr) (sh_set_flag((ptr)->cur_size, sh_block_flag_fragment))
#define sh_block_del_fragment_flag(ptr) (sh_del_flag((ptr)->cur_size, sh_block_flag_fragment))
#define sh_block_is_fragment(ptr) (sh_is_flag_set((ptr)->cur_size, sh_block_flag_fragment))

#define sh_block_set_eof_flag(ptr) (sh_set_flag((ptr)->cur_size, sh_block_flag_eof))
#define sh_block_del_eof_flag(ptr) (sh_del_flag((ptr)->cur_size, sh_block_flag_eof))
#define sh_block_is_eof(ptr) (sh_is_flag_set((ptr)->cur_size, sh_block_flag_eof))


#define sh_block_copy_flag(from, to) ((to)->cur_size = (((to)->cur_size & ~0x7) | ((from)->cur_size & 0x7)))



#define sh_block_set_cur_size(ptr, size) ((ptr)->cur_size = (((ptr)->cur_size & 0x7)|(size)))
#define sh_block_get_cur_size(ptr) ((ptr)->cur_size & (~0x7))

#define sh_block_set_pre_size(ptr, size) ((ptr)->pre_size = (size))
#define sh_block_get_pre_size(ptr) ((ptr)->pre_size)


#define sh_block_set_sof_flag(ptr) ((ptr)->pre_size = 0)
#define sh_block_is_sof(ptr) ((ptr)->pre_size == 0)

#define sh_fragment_next(ptr) ((sh_block_t*)(((char*)(ptr)) + sh_block_get_cur_size(ptr)))
#define sh_fragment_prev(ptr) ((sh_block_t*)(((char*)(ptr)) - sh_block_get_pre_size(ptr)))




inline int sh_block_reset(sh_block_t *block)//the same addr
{
	memset(block, 0, sizeof(sh_block_t));
	return 0;
}




/////////////////////////////////////comm end/////////////////////////////////////////

/////////////////////////////////////chunk start/////////////////////////////////////////

//max 1GB
#define MAX_LEVEL 30



typedef struct
{
	sh_list_head_t area[MAX_LEVEL + 1];
	unsigned long total_size;
	unsigned long free_size;
}sh_chunk_manager_t;

inline int sh_chunk_init(sh_block_t *chunk, unsigned long size)
{
	sh_block_reset(chunk);
	sh_block_set_cur_size(chunk, size);
	return 0;
}

inline int sh_size2level(unsigned long size)
{
	int l = 0;
	int r = MAX_LEVEL;
	int m;
	unsigned long sz;
	int level = -1;
	while (l <= r)
	{
		m = ((l + r) >> 1);
		sz = (1 << m);
		if (sz >= size)
		{
			level = m;
			r = m - 1;
		}
		else
		{
			l = m + 1;
		}
	}
	return level;
}

#define sh_chunk_set_level(ptr, level) ((ptr)->cur_size = (((ptr)->cur_size & 0x7)|(1 << (size))))
#define sh_chunk_get_level(chunk) sh_size2level(sh_block_get_cur_size(chunk))

/////////////////////////////////////chunk end/////////////////////////////////////////


/////////////////////////////////////fragment start/////////////////////////////////////////


#define MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER 512
#define GRADS_BETWEEN_AREA_OF_FRAGMENT_MANAGER 8

//4096
#define LEVEL_OF_THE_CHUNK_TRANSLATE_TO_FRAGMENT 12

typedef struct
{
	sh_list_head_t area[MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER / GRADS_BETWEEN_AREA_OF_FRAGMENT_MANAGER + 1];
	sh_list_head_t cache[1];//mem malloc from chunk manager but not less than MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER
	unsigned long total_size;
	unsigned long free_size;
} sh_fragment_manager_t;


inline int sh_fragment_init(sh_block_t *fragment, unsigned long size)
{
	sh_block_reset(fragment);
	sh_block_set_cur_size(fragment, size);
	sh_block_set_fragment_flag(fragment);
	return 0;
}

/////////////////////////////////////fragment end/////////////////////////////////////////


/////////////////////////////////////sh_record_node_t start/////////////////////////////////////////

typedef struct
{
	int id;
	sh_ptr_common_t ptr;
	sh_list_node_t link[1];
}sh_record_node_t;

/////////////////////////////////////sh_record_node_t end/////////////////////////////////////////

typedef struct
{
	uint64_t init_code;
	sh_chunk_manager_t chunk_manager[1];
	sh_fragment_manager_t fragment_manager[1];
	//unsigned long shm_base;
	unsigned long size;
	sh_list_head_t id2rel[HASH_SIZE][1];
}sh_heap_t;//size:


inline sh_block_t* sh_get_binary_base_block(sh_block_t *block)
{
	if (!block)
	{
		return NULL;
	}

	sh_block_t *base_block;
	unsigned long block_rel = (unsigned long)((char*)block - g_shm_base - sizeof(sh_heap_t));
	unsigned long base_block_rel = (block_rel & ~((1 << (sh_chunk_get_level(block) + 1)) - 1));

	base_block = (sh_block_t*)((char*)block + base_block_rel - block_rel);

	return base_block;
}

#pragma pack()

unsigned long g_shm_base = 0;
sh_heap_t *heap = NULL;

int sh_chunk_manager_init(sh_chunk_manager_t *chunk_manager, char *buffer, unsigned long size)
{
	sh_block_t *chunk;


	memset(chunk_manager, 0, sizeof(sh_chunk_manager_t));

	//split buffer into chunk
	for (int level = MAX_LEVEL; level >= sh_size2level(MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER); level--)
	{
		unsigned long cur_size = (1 << level);
		while (size >= cur_size)
		{
			chunk = (sh_block_t*)buffer;
			sh_chunk_init(chunk, cur_size);
			sh_list_add_node_after(chunk->link, &chunk_manager->area[level]);
			buffer += cur_size;
			size -= cur_size;

			chunk_manager->total_size += cur_size;
			chunk_manager->free_size += cur_size;
		}
	}


	return 0;
}

int sh_fragment_manager_init(sh_fragment_manager_t *fragment_manager)
{
	memset(fragment_manager, 0, sizeof(sh_fragment_manager_t));
	return 0;
}



int sh_heap_init(char *buffer, unsigned long size)
{
	if (!buffer)
	{
		return __LINE__;
	}
	if ((unsigned long)size < sizeof(sh_heap_t) + HEAP_OFFSET)
	{
		return __LINE__;
	}

	int iRet;


	heap = (sh_heap_t*)(buffer + HEAP_OFFSET);
	memset(heap, 0, sizeof(sh_heap_t));
	heap->size = size;
	//heap->shm_base = (long)buffer;


	buffer += HEAP_OFFSET;
	size -= HEAP_OFFSET;


	buffer += sizeof(sh_heap_t);
	size -= sizeof(sh_heap_t);

	if ((iRet = sh_chunk_manager_init(heap->chunk_manager, buffer, size)) != 0)
	{
		return __LINE__;
	}
	if ((iRet = sh_fragment_manager_init(heap->fragment_manager)) != 0)
	{
		return __LINE__;
	}

	heap->init_code = HEAP_INIT_CODE;

	return 0;
}

int sh_heap_clear()
{
	if (g_shm_base == 0)
	{
		return __LINE__;
	}
	return sh_heap_init((char*)g_shm_base, heap->size);
}

int sh_heap_attach(char *buffer, unsigned long size)
{
	if (!buffer)
	{
		return __LINE__;
	}
	if ((unsigned long)size < sizeof(sh_heap_t) + HEAP_OFFSET)
	{
		return __LINE__;
	}

	g_shm_base = (unsigned long)buffer;//set this first!!

	heap = (sh_heap_t*)(buffer + HEAP_OFFSET);
	if (heap->init_code != HEAP_INIT_CODE)
	{
		return sh_heap_init(buffer, size);
	}
	return 0;
}

void* sh_chunk_alloc(sh_chunk_manager_t *chunk_manager, unsigned long size)
{
	int suitable_level = sh_size2level(size);
	if (suitable_level == -1 || suitable_level > MAX_LEVEL)
	{
		printf("malloc err: too large size. line:%d", __LINE__);
		return NULL;
	}


	int ok_level;
	for (ok_level = suitable_level; ok_level <= MAX_LEVEL; ok_level++)
	{
		if (!sh_list_empty(&chunk_manager->area[ok_level]))
		{
			break;
		}
	}
	if (ok_level > MAX_LEVEL)
	{
		//printf("malloc err: no suitable mem to alloc. line:%d", __LINE__);
		return NULL;
	}


	sh_block_t *chunk;
	sh_block_t *new_chunk;
	unsigned long cur_size;
	for (int i = ok_level; i > suitable_level; i--)
	{
		if (!sh_list_empty(&chunk_manager->area[i]))
		{
			chunk = sh_container_of(sh_list_first_node(&chunk_manager->area[i]), sh_block_t, link);
			sh_list_del_node(chunk->link);

			cur_size = sh_block_get_cur_size(chunk);
			new_chunk = (sh_block_t*)((char*)chunk + cur_size / 2);

			sh_chunk_init(chunk, cur_size / 2);
			sh_chunk_init(new_chunk, cur_size / 2);

			sh_list_add_node_after(chunk->link, &chunk_manager->area[i - 1]);
			sh_list_add_node_after(new_chunk->link, &chunk_manager->area[i - 1]);
		}
	}


	chunk = sh_container_of(sh_list_first_node(&chunk_manager->area[suitable_level]), sh_block_t, link);
	sh_list_del_node(chunk->link);
	sh_block_set_busy_flag(chunk);
	chunk_manager->free_size -= sh_block_get_cur_size(chunk);

	memset(chunk->link, 0, sh_block_get_cur_size(chunk) - BLOCK_INFO_SIZE);

	return chunk->link;//just keep cur_size and pre_size
}

int sh_chunk_free(sh_chunk_manager_t *chunk_manager, void *ptr)
{
	if (ptr == sh_null)
	{
		return __LINE__;
	}
	sh_block_t *chunk = sh_container_of(ptr, sh_block_t, link);

	int level = sh_chunk_get_level(chunk);
	sh_block_del_busy_flag(chunk);
	sh_list_add_node_after(chunk->link, &chunk_manager->area[level]);
	chunk_manager->free_size += sh_block_get_cur_size(chunk);

	sh_block_t *base_chunk;
	//merge
	while (1)
	{
		if (level == MAX_LEVEL)
		{
			break;
		}

		if (sh_list_empty(&chunk_manager->area[level]))
		{
			break;
		}

		chunk = sh_container_of(sh_list_first_node(&chunk_manager->area[level]), sh_block_t, link);
		base_chunk = sh_get_binary_base_block(chunk);
		chunk = (sh_block_t*)((char*)base_chunk + (1 << level));

		if (sh_chunk_get_level(base_chunk) != level
				|| sh_chunk_get_level(chunk) != level)
		{
			break;
		}

		if (sh_block_is_busy(base_chunk) || sh_block_is_busy(chunk))
		{
			break;
		}

		//20121127 avoid to free the fragment in fragment_manager->cache
		if (sh_block_is_fragment(base_chunk) || sh_block_is_fragment(chunk))
		{
			    break;
		}

		sh_list_del_node(base_chunk->link);
		sh_list_del_node(chunk->link);

		sh_chunk_init(base_chunk, 1 << (level + 1));

		sh_list_add_node_after(base_chunk->link, &chunk_manager->area[level + 1]);

		level++;
	}
	return 0;
}

int sh_flash_fragment_cache_back(sh_fragment_manager_t *fragment_manager)
{
	sh_block_t *block;

	sh_ptr_t<sh_list_node_t> list_iter;
	sh_ptr_t<sh_list_node_t> list_iter_next;
	for_each_list_node_safe(fragment_manager->cache, list_iter, list_iter_next)
	{
		block = sh_container_of(list_iter, sh_block_t, link);
		if (sh_block_is_sof(block) && sh_block_is_eof(block))
		{
			fragment_manager->total_size -= sh_block_get_cur_size(block);
			heap->chunk_manager->total_size += sh_block_get_cur_size(block);
			fragment_manager->free_size -= sh_block_get_cur_size(block);

			sh_list_del_node(block->link);
			sh_block_del_fragment_flag(block);
			sh_block_del_eof_flag(block);

			sh_chunk_free(heap->chunk_manager, block->link);
		}
	}
	return 0;
}

sh_block_t* sh_fragment_try_merge(sh_block_t *fragment)
{
	sh_block_t *prev_fragment, *next_fragment;

	while (1)
	{
		if (sh_block_is_sof(fragment))
		{
			break;
		}
		prev_fragment = sh_fragment_prev(fragment);
		if (sh_block_is_busy(prev_fragment))
		{
			break;
		}
		sh_list_del_node(prev_fragment->link);
		if (sh_block_is_eof(fragment))
		{
			sh_block_set_eof_flag(prev_fragment);
		}
		else
		{
			sh_block_set_pre_size(sh_fragment_next(fragment), sh_block_get_cur_size(prev_fragment) + sh_block_get_cur_size(fragment));
		}
		sh_block_set_cur_size(prev_fragment, sh_block_get_cur_size(prev_fragment) + sh_block_get_cur_size(fragment));

		fragment = prev_fragment;
	}

	while (1)
	{
		if (sh_block_is_eof(fragment))
		{
			break;
		}
		next_fragment = sh_fragment_next(fragment);
		if (sh_block_is_busy(next_fragment))
		{
			break;
		}
		sh_list_del_node(next_fragment->link);
		if (sh_block_is_eof(next_fragment))
		{
			sh_block_set_eof_flag(fragment);
		}
		else
		{
			sh_block_set_pre_size(sh_fragment_next(next_fragment), sh_block_get_cur_size(fragment) + sh_block_get_cur_size(next_fragment));
		}
		sh_block_set_cur_size(fragment, sh_block_get_cur_size(fragment) + sh_block_get_cur_size(next_fragment));

	}

	return fragment;
}

int sh_fragment_insert(sh_fragment_manager_t *fragment_manager, sh_block_t *fragment)
{
	if (sh_block_get_cur_size(fragment) <= MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER)
	{
		int idx = sh_block_get_cur_size(fragment) / 8;
		sh_list_add_node_after(fragment->link, &fragment_manager->area[idx]);
	}
	else
	{
		if (sh_list_empty(fragment_manager->cache))
		{
			sh_list_add_node_after(fragment->link, fragment_manager->cache);
		}
		else
		{
			sh_block_t *fragment_iter;
			sh_ptr_t<sh_list_node_t> list_iter;
			sh_ptr_t<sh_list_node_t> list_iter_next;
			for_each_list_node_safe(fragment_manager->cache, list_iter, list_iter_next)
			{
				fragment_iter = sh_container_of(list_iter, sh_block_t, link);
				if (sh_block_get_cur_size(fragment_iter) <= sh_block_get_cur_size(fragment))
				{
					sh_list_add_node_before(fragment->link, fragment_iter->link);
					break;
				}
				if (list_iter_next == sh_null)//insert at tail
				{
					sh_list_add_node_after(fragment->link, fragment_iter->link);
					break;
				}
			}
		}
	}
	return 0;
}

int sh_fragment_free(sh_fragment_manager_t *fragment_manager, void *ptr)
{
	sh_block_t *fragment = sh_container_of(ptr, sh_block_t, link);
	if (!sh_block_is_fragment(fragment))
	{
		return __LINE__;
	}
	sh_block_del_busy_flag(fragment);
	fragment_manager->free_size += sh_block_get_cur_size(fragment);
	fragment = sh_fragment_try_merge(fragment);
	sh_fragment_insert(fragment_manager, fragment);

	return 0;
}

sh_block_t* sh_fragment_try_split(sh_block_t *fragment, unsigned long size)
{
	sh_block_t *high_fragment;

	if ((unsigned long)size < sizeof(sh_block_t) || (unsigned long)sh_block_get_cur_size(fragment) < (unsigned long)size + sizeof(sh_block_t))
	{
		return NULL;
	}

	high_fragment = (sh_block_t*)((char*)fragment + size);
	sh_block_set_cur_size(high_fragment, sh_block_get_cur_size(fragment) - size);
	sh_block_set_pre_size(high_fragment, size);
	sh_block_set_cur_size(fragment, size);
	sh_block_copy_flag(fragment, high_fragment);
	sh_block_del_eof_flag(fragment);

	if (!sh_block_is_eof(high_fragment))
	{
		sh_block_t *next_fragment = (sh_block_t*)((char*)high_fragment + sh_block_get_cur_size(high_fragment));
		sh_block_set_pre_size(next_fragment, sh_block_get_cur_size(high_fragment));
	}

	return high_fragment;
}

void* sh_fragment_alloc(sh_fragment_manager_t *fragment_manager, unsigned long size)
{
	int iRet;
	void *ptr;
	sh_block_t *fragment = NULL;

	for (int i = size / 8; i <= MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER / GRADS_BETWEEN_AREA_OF_FRAGMENT_MANAGER; i++)
	{
		if (!sh_list_empty(&fragment_manager->area[i]))
		{
			fragment = sh_container_of(sh_list_first_node(&fragment_manager->area[i]), sh_block_t, link);
			sh_list_del_node(fragment->link);
			break;
		}
	}

	if (!fragment)
	{
		if (!sh_list_empty(fragment_manager->cache))
		{
			fragment = sh_container_of(sh_list_first_node(fragment_manager->cache), sh_block_t, link);
			if ((unsigned long)sh_block_get_cur_size(fragment) < (unsigned long)size)
			{
				fragment = NULL;
			}
			else
			{
				sh_list_del_node(fragment->link);
			}
		}
	}

	if (!fragment)
	{
		ptr = sh_chunk_alloc(heap->chunk_manager, (1 << LEVEL_OF_THE_CHUNK_TRANSLATE_TO_FRAGMENT));
		if (ptr == NULL)
		{
			return NULL;
		}

		fragment = sh_container_of(ptr, sh_block_t, link);

		heap->chunk_manager->total_size -= sh_block_get_cur_size(fragment);
		fragment_manager->total_size += sh_block_get_cur_size(fragment);
		fragment_manager->free_size += sh_block_get_cur_size(fragment);

		sh_fragment_init(fragment, sh_block_get_cur_size(fragment));
		sh_block_set_sof_flag(fragment);
		sh_block_set_eof_flag(fragment);
	}


	if (!fragment)
	{
		return NULL;
	}

	sh_block_t *rest_fragment = sh_fragment_try_split(fragment, size);
	if (rest_fragment)
	{
		if ((iRet = sh_fragment_insert(fragment_manager, rest_fragment)) != 0)
		{
			printf("sh_fragment_insert err. ret:%d", iRet);
			return NULL;
		}
	}

	sh_block_set_busy_flag(fragment);
	fragment_manager->free_size -= sh_block_get_cur_size(fragment);

	memset(fragment->link, 0, sh_block_get_cur_size(fragment) - BLOCK_INFO_SIZE);

	return fragment->link;
}

sh_ptr_common_t sh_malloc(unsigned long size, int id)
{
	if (heap == sh_null)
	{
		return NULL;
	}
	if (size == 0)
	{
		return NULL;
	}

	//fix size
	{
		/*
		   the minimum alloc or free unit is a block, the usr area is begin from sh_block_t.link,
		   so the usr area must not less than sizeof(sh_block_t.link).
		 */
		if ((unsigned long)size < sizeof(sh_list_node_t))
		{
			size = sizeof(sh_list_node_t);
		}
		sh_align_ceil(size, 8);
		size += BLOCK_INFO_SIZE;
	}


	void *ptr = NULL;
	if (size >= 512)
	{
		ptr = sh_chunk_alloc(heap->chunk_manager, size);
		if (ptr == NULL)
		{
			sh_flash_fragment_cache_back(heap->fragment_manager);
			ptr = sh_chunk_alloc(heap->chunk_manager, size);
		}
	}
	else
	{
		ptr = sh_fragment_alloc(heap->fragment_manager, size);
	}


	if (ptr && id)
	{
		if (sh_save_record(ptr, id))
		{
			return NULL;
		}
	}

	return ptr;

}

int sh_check_ptr(sh_ptr_common_t ptr)
{
	if (heap == sh_null || ptr == sh_null)
	{
		return __LINE__;
	}
	if ((unsigned long)(char*)ptr >= g_shm_base + HEAP_OFFSET + sizeof(sh_heap_t)
			&& (unsigned long)(char*)ptr < g_shm_base + HEAP_OFFSET + sizeof(sh_heap_t) + heap->chunk_manager->total_size + heap->fragment_manager->total_size)
	{
		return 0;
	}
	return __LINE__;
}

int sh_free(sh_ptr_common_t ptr)
{
	if (heap == sh_null || ptr == sh_null)
	{
		return __LINE__;
	}
	if (sh_check_ptr(ptr) != 0)
	{
		return __LINE__;
	}
	sh_block_t *block = sh_container_of(ptr, sh_block_t, link);
	if (sh_block_is_fragment(block))
	{
		return sh_fragment_free(heap->fragment_manager, ptr);
	}
	else
	{
		return sh_chunk_free(heap->chunk_manager, ptr);
	}
	return 0;
}


sh_ptr_common_t sh_remalloc(sh_ptr_common_t old_ptr, unsigned long size, int id)
{
	if (heap == sh_null)
	{
		return NULL;
	}
	if (size == 0)
	{
		return NULL;
	}

	if (old_ptr == sh_null)
	{
		return sh_malloc(size, id);
	}

	size += BLOCK_INFO_SIZE;

	sh_block_t *old_chunk;
	old_chunk = sh_container_of(old_ptr, sh_block_t, link);

	sh_ptr_common_t new_ptr;
	if ((new_ptr = sh_malloc(size, id)) == NULL)
	{
		return NULL;
	}

	memcpy((char*)new_ptr, (char*)old_ptr, sh_block_get_cur_size(old_chunk) < size ? sh_block_get_cur_size(old_chunk) : size);

	sh_free(old_ptr);

	if (new_ptr != sh_null && id)
	{
		if (sh_save_record(new_ptr, id))
		{
			return NULL;
		}
	}

	return new_ptr;

}

int sh_get_size_info(unsigned long *tot_size, unsigned long *free_size)
{
	if (heap == NULL)
	{
		return __LINE__;
	}
	if (tot_size)
	{
		*tot_size = heap->chunk_manager->total_size + heap->fragment_manager->total_size;
	}
	if (free_size)
	{
		*free_size = heap->chunk_manager->free_size + heap->fragment_manager->free_size;
	}

	return 0;
}

int sh_dump_info(char *out_file)
{
	FILE *fp = fopen(out_file, "w");
	if (fp == NULL)
	{
		return __LINE__;
	}

	fprintf(fp, "version: \t%.2lf\n", HEAP_VERSION);
	fprintf(fp, "\n");

	fprintf(fp, "--------------------------chunk info----------------------------\n");
	fprintf(fp, "total_size: \t%lu\n", heap->chunk_manager->total_size);
	fprintf(fp, "free_size: \t%lu\n", heap->chunk_manager->free_size);
	fprintf(fp, "\n");
	fprintf(fp, "level \tsize \tcount\n");
	for (int i = 0; i <= MAX_LEVEL; i++)
	{
		fprintf(fp, "%d \t%llu \t%d\n", i, 1LL << i, sh_list_len(&heap->chunk_manager->area[i]));
	}


	fprintf(fp, "\n");
	fprintf(fp, "\n");
	fprintf(fp, "--------------------------fragment info----------------------------\n");
	fprintf(fp, "total_size: \t%lu\n", heap->fragment_manager->total_size);
	fprintf(fp, "free_size: \t%lu\n", heap->fragment_manager->free_size);
	fprintf(fp, "\n");
	fprintf(fp, "size \tcount\n");
	for (int i = 0; i <= MAX_MALLOC_SIZE_FROM_FRAGMENT_MANAGER / GRADS_BETWEEN_AREA_OF_FRAGMENT_MANAGER; i++)
	{
		fprintf(fp, "%d \t%d\n", i * GRADS_BETWEEN_AREA_OF_FRAGMENT_MANAGER, sh_list_len(&heap->fragment_manager->area[i]));
	}
	fprintf(fp, "\n");
	fprintf(fp, "cache zone:\n");
	fprintf(fp, "No. \tsize\n");

	int id = 0;
	sh_block_t *block;
	sh_ptr_t<sh_list_node_t> list_iter;
	for_each_list_node(heap->fragment_manager->cache, list_iter)
	{
		block = sh_container_of(list_iter, sh_block_t, link);
		fprintf(fp, "%d \t%lu\n", id, sh_block_get_cur_size(block));
		id++;
	}
	fprintf(fp, "\n");
	fprintf(fp, "\n");

	fclose(fp);


	return 0;
}


inline int sh_save_record(sh_ptr_common_t ptr, int id)
{
	sh_ptr_t<sh_list_node_t> list_iter;
	long hash_key;

	hash_key = id & HASH_MASK;

	sh_ptr_t<sh_record_node_t> record_ptr;
	for_each_list_node(heap->id2rel[hash_key], list_iter)
	{
		record_ptr = sh_container_of(list_iter, sh_record_node_t, link);
		if (record_ptr->id == id)
		{
			record_ptr->ptr = ptr;
			return 0;
		}
	}
	record_ptr = sh_malloc(sizeof(sh_record_node_t), 0);
	record_ptr->id = id;
	record_ptr->ptr = ptr;
	sh_list_add_node_after(record_ptr->link, heap->id2rel[hash_key]);
	return 0;
}

sh_ptr_common_t sh_get_record(int id)
{
	sh_ptr_t<sh_list_node_t> list_iter;
	long hash_key;

	hash_key = id & HASH_MASK;

	sh_ptr_t<sh_record_node_t> record_ptr;
	for_each_list_node(heap->id2rel[hash_key], list_iter)
	{
		record_ptr = sh_container_of(list_iter, sh_record_node_t, link);
		if (record_ptr->id == id)
		{
			return record_ptr->ptr;
		}
	}
	return sh_null;
}

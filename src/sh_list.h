

#ifndef __SH_LIST_H__
#define __SH_LIST_H__

#include <string.h>
#include <stdio.h>
#include "sh_comm_def.h"







#pragma pack(1)


struct sh_list_node_t
{
	sh_ptr_t<sh_list_node_t> prev;
	sh_ptr_t<sh_list_node_t> next;
};
typedef sh_list_node_t sh_list_head_t;
#define sh_list_node_reset(node) ((node)->prev = sh_null, (node)->next = sh_null)



#pragma pack()




#define sh_list_no_more_next(node) (((node)->next) == sh_null)
#define sh_list_no_more_prev(node) (((node)->prev) == sh_null)
#define sh_list_first_node(head) ((head)->next)
#define sh_list_next_node(cur) ((cur)->next)
#define sh_list_empty(head) (sh_list_no_more_next(head))


#define for_each_list_node(head, iter)\
	for ((iter) = sh_list_first_node(head); (iter) != sh_null; \
			(sh_list_no_more_next(iter) ? (iter) = sh_null : (iter) = sh_list_next_node(iter)))

#define for_each_list_node_safe(head, iter, next_iter)\
	for ((iter) = sh_list_first_node(head); \
			((iter) != sh_null && \
			 (((next_iter) = sh_list_no_more_next(iter) ? sh_null : (char*)sh_list_next_node(iter)), 1) \
			); \
			(iter) = (next_iter))



inline int sh_list_add_node_after(sh_ptr_t<sh_list_node_t> n, sh_ptr_t<sh_list_node_t> cur)
{
	n->next = cur->next;
	n->prev = cur;
	if (!sh_list_no_more_next(cur))
	{
		cur->next->prev = n;
	}
	cur->next = n;

	return 0;
}

inline int sh_list_add_node_before(sh_ptr_t<sh_list_node_t> n, sh_ptr_t<sh_list_node_t> cur)
{
	n->next = cur;
	n->prev = cur->prev;
	if (!sh_list_no_more_prev(cur))
	{
		cur->prev->next = n;
	}
	cur->prev = n;

	return 0;
}

inline int sh_list_del_node(sh_ptr_t<sh_list_node_t> n)
{
	if (!sh_list_no_more_next(n))
	{
		n->next->prev = n->prev;
	}
	if (!sh_list_no_more_prev(n))
	{
		n->prev->next = n->next;
	}
	sh_list_node_reset(n);

	return 0;
}

inline int sh_list_len(sh_ptr_t<sh_list_head_t> head)
{
	if (sh_list_empty(head))
	{
		return 0;
	}
	int len = 0;
	
	sh_ptr_t<sh_list_node_t> iter;
	for_each_list_node(head, iter)
	{
		len++;
	}
	return len;
}





#endif

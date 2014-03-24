#ifndef __SH_HASH_H__
#define __SH_HASH_H__

#include <stdlib.h>
#include <stdint.h>
#include "sh_comm_def.h"

struct sh_hash_node_t
{
	uint64_t ddwKey;
	uint32_t dwValueLen;
	sh_ptr_common_t pValue;
	sh_ptr_t<sh_hash_node_t> pNext;
}; 


struct sh_hash_t
{
	private:
		uint32_t m_dwHashSize;
		sh_ptr_t<sh_ptr_t<sh_hash_node_t> > m_buckets;
	public:
		int init(int hash_size);
		int find(uint64_t ddwKey, char **ppValue, uint32_t &dwValueLen);
		int del(uint64_t ddwKey);
		int insert(uint64_t ddwKey, char *pValue, uint32_t dwValueLen);
};


#endif

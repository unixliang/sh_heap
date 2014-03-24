
#include "sh_heap.h"
#include "sh_hash.h"

void free_hash_node(sh_ptr_t<sh_hash_node_t> pNode)
{
	sh_free(pNode->pValue);
	sh_free(pNode);
}

int sh_hash_t::init(int hash_size)
{
	m_dwHashSize = hash_size;
	m_buckets = sh_malloc(m_dwHashSize * sizeof(sh_ptr_t<sh_hash_node_t>), 0);
	if (m_buckets == sh_null)
	{
		return -__LINE__;
	}
	return 0;
}

int sh_hash_t::find(uint64_t ddwKey, char **ppValue, uint32_t &dwValueLen)
{
	if (ppValue == NULL)
	{
		return -__LINE__;
	}

	*ppValue = NULL;
	dwValueLen = 0;

	uint32_t dwMod = ddwKey % m_dwHashSize;
	for (sh_ptr_t<sh_hash_node_t> pNode = m_buckets[dwMod];
			pNode != sh_null; pNode = pNode->pNext)
	{
		if (pNode->ddwKey == ddwKey)
		{
			*ppValue = (char*)pNode->pValue;
			dwValueLen = pNode->dwValueLen;
			break;
		}
	}
	return 0;
}

int sh_hash_t::del(uint64_t ddwKey)
{
	sh_ptr_t<sh_hash_node_t> pPreNode = sh_null;
	uint32_t dwMod = ddwKey % m_dwHashSize;
	for (sh_ptr_t<sh_hash_node_t> pNode = m_buckets[dwMod];
			pNode != sh_null; pNode = pNode->pNext)
	{
		if (pNode->ddwKey == ddwKey)
		{
			if (pPreNode == sh_null)
			{
				m_buckets[dwMod] = sh_null;
			}
			else
			{
				pPreNode->pNext = pNode->pNext;
			}
			free_hash_node(pNode);
			break;
		}
		pPreNode = pNode;
	}

	return 0;
}

int sh_hash_t::insert(uint64_t ddwKey, char *pValue, uint32_t dwValueLen)
{
	sh_ptr_t<sh_hash_node_t> pNode = sh_malloc(sizeof(sh_hash_node_t), 0);

	pNode->ddwKey = ddwKey;
	pNode->pValue = sh_malloc(dwValueLen, 0);
	memcpy((char*)pNode->pValue, pValue, dwValueLen);
	pNode->dwValueLen = dwValueLen;

	uint32_t dwMod = ddwKey % m_dwHashSize;

	pNode->pNext = m_buckets[dwMod];
	m_buckets[dwMod] = pNode;

	return 0;
}

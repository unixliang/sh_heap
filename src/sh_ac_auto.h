#ifndef __SH_AC_AUTO_H__
#define __SH_AC_AUTO_H__


#include <stdlib.h>
#include <stdint.h>
#include "sh_comm_def.h"

#define IN
#define OUT


#define MAX_NODE_NUM	10000000
#define MAX_MOD_NUM		1100000
#define MAX_WORD_NUM	1000000

typedef struct
{
	uint32_t dwWordId;
	uint16_t wStart;
	uint16_t wLen;
}MatchedInfo;

struct ModIdNode
{
	uint32_t dwModId;
	sh_ptr_t<ModIdNode> pNext;
};

struct AcNode
{
	sh_ptr_t<AcNode> pChild[256];
	sh_ptr_t<AcNode> pFail;
	sh_ptr_t<ModIdNode> pModIdNode;
	uint32_t dwNodeId;
	uint8_t bDanger;
	uint8_t bCh;
};

struct AcAuto
{
	private:
		AcNode m_stRoot;

		uint32_t m_dwNodeCnt;
		uint32_t m_dwModCnt;
		uint32_t m_dwWordCnt;

		sh_ptr_t<uint32_t> m_adwWordId;
		sh_ptr_t<uint32_t> m_adwModLen;

	public:
		int init();
		int insert(IN const uint16_t wWordLen, IN const char *pWord, OUT uint32_t &dwWordId);
		int makefail();
		int match(IN const uint16_t wTxtLen, IN const char *pTxt, IN const uint32_t nMaxMatchedInfo,
				OUT MatchedInfo astMatchedInfo[], OUT uint32_t &nMatchedInfo);
		uint32_t get_node_cnt();
		uint32_t get_word_cnt();
};




#endif

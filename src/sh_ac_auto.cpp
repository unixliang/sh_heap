#include "sh_ac_auto.h"
#include "sh_heap.h"

#define PER_GROW_STEP 2000

struct QueueNode
{
	sh_ptr_t<AcNode> pstAcNode;
	sh_ptr_t<QueueNode> pNext;
};

int AcAuto::init()
{
	memset(&m_stRoot, 0, sizeof(AcNode));
	m_stRoot.dwNodeId = 1;

	m_dwNodeCnt = 1;
	m_dwModCnt = 0;
	m_dwWordCnt = 0;

	return 0;
}

int AcAuto::insert(IN const uint16_t wWordLen, IN const char *pWord, OUT uint32_t &dwWordId)
{
	if (m_dwWordCnt >= MAX_WORD_NUM)
	{
		return -__LINE__;
	}

	dwWordId = 0;

	int iLastModEndPos = 0;

	uint32_t dwOldModCnt = m_dwModCnt;

	sh_ptr_t<AcNode> pFrom = &m_stRoot;
	sh_ptr_t<AcNode> pTo = sh_null;
	sh_ptr_t<ModIdNode> pModIdNode;
	for (int i = 0; i < wWordLen; i++)
	{
		if (pWord[i] == '*')
		{
			if (pTo != sh_null)
			{
				pModIdNode = sh_malloc(sizeof(ModIdNode), 0);
				if (pModIdNode == sh_null)
				{
					return __LINE__;
				}
				if (m_dwModCnt % PER_GROW_STEP == 0)
				{
					m_adwWordId = sh_remalloc(m_adwWordId, (m_dwModCnt + PER_GROW_STEP) * sizeof(uint32_t), 0);
					memset((char*)m_adwWordId + m_dwModCnt * sizeof(uint32_t), 0, PER_GROW_STEP * sizeof(uint32_t));

					m_adwModLen = sh_remalloc(m_adwModLen, (m_dwModCnt + PER_GROW_STEP) * sizeof(uint32_t), 0);
					memset((char*)m_adwModLen + m_dwModCnt * sizeof(uint32_t), 0, PER_GROW_STEP * sizeof(uint32_t));
				}
				if (m_dwModCnt + 1 >= MAX_MOD_NUM)
				{
					return -__LINE__;
				}
				pModIdNode->dwModId = ++m_dwModCnt;
				pModIdNode->pNext = pTo->pModIdNode;
				pTo->pModIdNode = pModIdNode;

				m_adwModLen[pModIdNode->dwModId] = i - iLastModEndPos;
				iLastModEndPos = i + 1;

				pTo->bDanger = 1;
			}
			pFrom = &m_stRoot;
			pTo = sh_null;
			continue;
		}
		pTo = pFrom->pChild[(uint8_t)(pWord[i])];
		if (pTo == sh_null)
		{
			pTo = sh_malloc(sizeof(AcNode), 0);
			if (pTo == sh_null)
			{
				return __LINE__;
			}
			memset((char*)pTo, 0, sizeof(AcNode));

			pFrom->pChild[(uint8_t)(pWord[i])] = pTo;
			pTo->pFail = sh_null;
			pTo->dwNodeId = ++m_dwNodeCnt;
			pTo->bDanger = 0;
			pTo->bCh = (uint8_t)(pWord[i]);
		}
		pFrom = pTo;
	}

	if (pTo != sh_null)
	{
		pModIdNode = sh_malloc(sizeof(ModIdNode), 0);
		if (pModIdNode == sh_null)
		{
			return __LINE__;
		}
		if (m_dwModCnt % PER_GROW_STEP == 0)
		{
			m_adwWordId = sh_remalloc(m_adwWordId, (m_dwModCnt + PER_GROW_STEP) * sizeof(uint32_t), 0);
			memset((char*)m_adwWordId + m_dwModCnt * sizeof(uint32_t), 0, PER_GROW_STEP * sizeof(uint32_t));

			m_adwModLen = sh_remalloc(m_adwModLen, (m_dwModCnt + PER_GROW_STEP) * sizeof(uint32_t), 0);
			memset((char*)m_adwModLen + m_dwModCnt * sizeof(uint32_t), 0, PER_GROW_STEP * sizeof(uint32_t));
		}
		if (m_dwModCnt + 1 >= MAX_MOD_NUM)
		{
			return -__LINE__;
		}
		pModIdNode->dwModId = ++m_dwModCnt;
		pModIdNode->pNext = pTo->pModIdNode;
		pTo->pModIdNode = pModIdNode;

		m_adwModLen[pModIdNode->dwModId] = (int)wWordLen - iLastModEndPos;
		iLastModEndPos = wWordLen + 1;

		pTo->bDanger = 1;
	}

	if (m_dwModCnt > dwOldModCnt)
	{
		m_adwWordId[m_dwModCnt] = ++m_dwWordCnt;
		dwWordId = m_dwWordCnt;
	}

	return 0;
}

int AcAuto::makefail()
{

	m_stRoot.pFail = sh_null;

	sh_ptr_t<QueueNode> pstQueueHead;
	sh_ptr_t<QueueNode> pstQueueTail;
	sh_ptr_t<QueueNode> pstCurQueueNode;
	sh_ptr_t<QueueNode> pstNewQueueNode;

	sh_ptr_t<AcNode> pstCurAcNode;
	sh_ptr_t<AcNode> pstChildAcNode;
	sh_ptr_t<AcNode> pstFailAcNode;
	sh_ptr_t<AcNode> pstFailChildAcNode;

	pstQueueTail = pstQueueHead = sh_null;
#define QUEUE_EMPTY (pstQueueTail == sh_null || pstQueueHead == sh_null)

	pstNewQueueNode = sh_malloc(sizeof(QueueNode), 0);
	if (pstNewQueueNode == sh_null)
	{
		return __LINE__;
	}
	pstNewQueueNode->pstAcNode = &m_stRoot;
	pstNewQueueNode->pNext = sh_null;
	if (QUEUE_EMPTY)
	{
		pstQueueHead = pstQueueTail = pstNewQueueNode;
	}
	else
	{
		pstQueueTail->pNext = pstNewQueueNode;
		pstQueueTail = pstNewQueueNode;
	}

	while (!QUEUE_EMPTY)
	{
		pstCurQueueNode = pstQueueHead;
		pstQueueHead = pstQueueHead->pNext;

		pstCurAcNode = pstCurQueueNode->pstAcNode;
		if (sh_free(pstCurQueueNode))
		{
			return __LINE__;
		}

		for (int i = 0; i < 256; i++)
		{
			pstChildAcNode = pstCurAcNode->pChild[i];
			if (pstChildAcNode != sh_null)
			{
				pstNewQueueNode = sh_malloc(sizeof(QueueNode), 0);
				if (pstNewQueueNode == sh_null)
				{
					return __LINE__;
				}
				pstNewQueueNode->pstAcNode = pstChildAcNode;
				pstNewQueueNode->pNext = sh_null;
				if (QUEUE_EMPTY)
				{
					pstQueueHead = pstQueueTail = pstNewQueueNode;
				}
				else
				{
					pstQueueTail->pNext = pstNewQueueNode;
					pstQueueTail = pstNewQueueNode;
				}


				pstFailAcNode = pstCurAcNode->pFail;
				while (pstFailAcNode != sh_null)
				{
					pstFailChildAcNode = pstFailAcNode->pChild[i];
					if (pstFailChildAcNode != sh_null)
					{
						pstChildAcNode->pFail = pstFailChildAcNode;
						pstChildAcNode->bDanger |= pstFailChildAcNode->bDanger;
						break;
					}
					pstFailAcNode = pstFailAcNode->pFail;
				}
				if (pstFailAcNode == sh_null)
				{
					pstChildAcNode->pFail = &m_stRoot;
				}
			}
		}
	}

	return 0;
}


int AcAuto::match(IN const uint16_t wTxtLen, IN const char *pTxt, IN const uint32_t nMaxMatchedInfo,
		OUT MatchedInfo astMatchedInfo[], OUT uint32_t &nMatchedInfo)
{
	nMatchedInfo = 0;

	//for muti process
	static uint32_t dwLoopId = 0;
	static uint32_t adwAcNodeIsPassed[MAX_NODE_NUM] = {0};
	static uint32_t adwModIsMatched[MAX_MOD_NUM] = {0};
	static uint16_t awWordMatchedStartPos[MAX_MOD_NUM];

	dwLoopId++;
	if (dwLoopId == 0)
	{
		memset(adwAcNodeIsPassed, 0, sizeof(adwAcNodeIsPassed));
		memset(adwModIsMatched, 0, sizeof(adwModIsMatched));
	}


	uint32_t dwWordId;
	uint32_t dwPreModId;
	sh_ptr_t<AcNode> pstCurAcNode = &m_stRoot;
	sh_ptr_t<AcNode> pstChildAcNode = sh_null;
	sh_ptr_t<AcNode> pstTmpAcNode = sh_null;
	sh_ptr_t<ModIdNode> pModIdNode;
	for (int i = 0; i < wTxtLen; i++)
	{
		while (pstCurAcNode != sh_null)
		{
			pstChildAcNode = pstCurAcNode->pChild[(uint8_t)(pTxt[i])];
			if (pstChildAcNode != sh_null)
			{
				break;
			}
			else
			{
				pstCurAcNode = pstCurAcNode->pFail;
			}
		}
		if (pstCurAcNode == sh_null)
		{
			pstCurAcNode = &m_stRoot;
			continue;
		}


		pstTmpAcNode = pstChildAcNode;
		while (pstTmpAcNode->bDanger)
		{
			if (adwAcNodeIsPassed[pstTmpAcNode->dwNodeId] == dwLoopId)
			{
				break;
			}
			adwAcNodeIsPassed[pstTmpAcNode->dwNodeId] = dwLoopId;

			pModIdNode = pstTmpAcNode->pModIdNode;
			while (pModIdNode != sh_null)
			{
				dwPreModId = pModIdNode->dwModId - 1;
				if (dwPreModId == 0 || m_adwWordId[dwPreModId] || adwModIsMatched[dwPreModId] == dwLoopId)
				{
					//match new mod!
					adwModIsMatched[pModIdNode->dwModId] = dwLoopId;
					awWordMatchedStartPos[pModIdNode->dwModId] = (dwPreModId && !m_adwWordId[dwPreModId] && adwModIsMatched[dwPreModId] == dwLoopId) ? 
						awWordMatchedStartPos[dwPreModId] : (uint32_t)(i + 1 - m_adwModLen[pModIdNode->dwModId]);
					if (m_adwWordId[pModIdNode->dwModId])//match new word!
					{
						dwWordId = m_adwWordId[pModIdNode->dwModId];
						if (nMatchedInfo < nMaxMatchedInfo)
						{
							astMatchedInfo[nMatchedInfo].dwWordId = dwWordId;
							astMatchedInfo[nMatchedInfo].wStart = awWordMatchedStartPos[pModIdNode->dwModId];
							astMatchedInfo[nMatchedInfo].wLen = (uint16_t)(i + 1 - awWordMatchedStartPos[pModIdNode->dwModId]);
							nMatchedInfo++;
						}
					}
				}

				pModIdNode = pModIdNode->pNext;
			}
			pstTmpAcNode = pstTmpAcNode->pFail;
		}

		adwAcNodeIsPassed[pstChildAcNode->dwNodeId] = dwLoopId;
		pstCurAcNode = pstChildAcNode;
	}
	return 0;
}

uint32_t AcAuto::get_node_cnt()
{
	return m_dwNodeCnt;
}

uint32_t AcAuto::get_word_cnt()
{
	return m_dwWordCnt;
}


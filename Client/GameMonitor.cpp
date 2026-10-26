// GameMonitor.cpp: implementation of the CGameMonitor class.
//
//////////////////////////////////////////////////////////////////////

#include "GameMonitor.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameMonitor::CGameMonitor()
{
 int i;

	for (i = 0 ; i < DEF_MAXBADWORD; i++)
		m_pWordList[i] = NULL;
}

CGameMonitor::~CGameMonitor()
{
 int i;

	for (i = 0 ; i < DEF_MAXBADWORD; i++)
	if (m_pWordList[i] != NULL) delete m_pWordList[i];
}

int CGameMonitor::iReadBadWordFileList(char *pFn)
{
 std::vector<std::string> words;
 int  iIndex;

	// pFn is contents\badword.json (format "bad-words"): plain JSON, no XOR obfuscation.
	// Returns the number of words loaded (at most DEF_MAXBADWORD), or -1 when the file is
	// missing or invalid.
	if (ClientData::ReadStringList(pFn, "bad-words", "words", words) == false) return -1;

	for (iIndex = 0; iIndex < (int)words.size() && iIndex < DEF_MAXBADWORD; iIndex++)
		m_pWordList[iIndex] = new class CMsg(NULL, (char *)words[iIndex].c_str(), NULL);

	return iIndex;
}

BOOL CGameMonitor::bCheckBadWord(char *pWord)
{
 int i;
 char cBuffer[500];


	ZeroMemory(cBuffer, sizeof(cBuffer));
	strcpy(cBuffer, pWord);
	i = 0;
	while ((m_pWordList[i] != NULL) && (strlen(m_pWordList[i]->m_pMsg) != 0)) {
		if (memcmp(cBuffer, m_pWordList[i]->m_pMsg, strlen(m_pWordList[i]->m_pMsg)) == 0) {
			return TRUE;
		}
		i++;
	}

	return FALSE;
}

#include "CRemoveList.h"

/*
 * FUNCTION: AddRemoveList
 *-----------------------------------------------------------------------------
 * DESCRIPTION
 *   Client를 삭제한다.
 *-----------------------------------------------------------------------------
 * PARAMETERS
 *   ulKey : [IN] 삭제할 Client의 고유키값
 *-----------------------------------------------------------------------------
 * SEE
 *   none
 *-----------------------------------------------------------------------------
 * RETURN VALUE
 *   none
 *-----------------------------------------------------------------------------
 * SIDE EFFECTS
 *   none
 *-----------------------------------------------------------------------------
 * Author
 *   Bard
 *-----------------------------------------------------------------------------
 * History
 *   2008. 06. ?? : Created
 */
void CRemoveList::AddRemoveList(unsigned long ulKey)
{
	Lock();
	m_RemoveList.push_back(ulKey);
	Unlock();
}


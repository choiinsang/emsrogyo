#ifndef __CREMOVE_LIST__
#define __CREMOVE_LIST__

#include <vector>
using namespace std;

#include "LockObject.h"

template <typename T>
class CRemoveList : public LockObject
{
public:
	vector< T > m_RemoveList;

	CRemoveList()	{ };
	virtual ~CRemoveList()	{ };

	void AddRemoveList(T);
};

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

template <typename T>
void CRemoveList<T>::AddRemoveList(T key)
{
	Lock();
	m_RemoveList.push_back(key);
	Unlock();
}

#endif

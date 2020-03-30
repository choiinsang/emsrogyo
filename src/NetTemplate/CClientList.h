/*===========================================================================================================
 * Module
 *   CClientList.h
 *-----------------------------------------------------------------------------------------------------------
 * Description
 * 
 *-----------------------------------------------------------------------------------------------------------
 * Author
 *   조진영
 *-----------------------------------------------------------------------------------------------------------
 * Version
 *   0.9.0.1
 *-----------------------------------------------------------------------------------------------------------
 * History
 *   2008. 09. 05 : Created (0.9.0.0)                                                               - 조진영
 *   2009. 01. 13 : ClientList Map List를 구하기 위한 함수 추가 (0.9.0.1)                           - 조진영
===========================================================================================================*/

#pragma once

#include <boost/shared_ptr.hpp>
#include "LockObject.h"

#ifdef __USETR1LIB__
#include <tr1/unordered_map>
#define unordered_map std::tr1::unordered_map
#else
#include <boost/unordered_map.hpp>
#define unordered_map boost::unordered_map
#endif

template <typename C>
class CClientList : public LockObject
{
    typedef typename unordered_map <unsigned long, boost::shared_ptr<C> > mapClient_l;

    typedef typename mapClient_l::iterator itmapClient_l;

// Construction
public:
    CClientList() { };
    virtual ~CClientList() { };

// Implementation
private:
    mapClient_l m_mapClientList;

public:
    bool Insert(boost::shared_ptr<C> pClient);
    bool Erase(unsigned long l_ulKey);
    void Clear();
    boost::shared_ptr<C> GetClient(unsigned long l_ulKey);
    itmapClient_l Begin() { return m_mapClientList.begin(); }
    itmapClient_l End()   { return m_mapClientList.end();   }
};

template <typename C>
bool CClientList<C>::Insert(boost::shared_ptr< C > l_pClient)
{
    bool bResult(false);
    Lock();
    m_mapClientList.insert( typename unordered_map <unsigned int, shared_ptr< C > >::
      value_type(l_pClient->m_socket, l_pClient));
    Unlock();
    return bResult;
}

template <typename C>
bool CClientList<C>::Erase(unsigned long l_ulKey)
{
    bool bResult(false);
    Lock();
    bResult = m_mapClientList.erase(l_ulKey) > 0 ? true:false;;
    Unlock();
    return bResult;
}

template <typename C>
void CClientList<C>::Clear()
{
    m_mapClientList.clear();
}

template <typename C>
boost::shared_ptr<C> CClientList<C>::GetClient(unsigned long l_ulKey)
{
    itmapClient_l itmapClientList;
    boost::shared_ptr<C> pClient;

    Lock();
    itmapClientList = m_mapClientList.find(l_ulKey);
    if (itmapClientList != m_mapClientList.end())
        pClient = itmapClientList->second;
    Unlock();

    return pClient;
}

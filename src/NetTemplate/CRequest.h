#ifndef __CREQUEST__
#define __CREQUEST__

#include <string.h>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

using namespace boost;

template <typename C>
class CRequest
{
    // Construction
    public:
        CRequest() { };
        virtual ~CRequest() { };
private:
        boost::shared_array<char> m_pPacket;
        boost::shared_ptr<C> m_pClient;
        unsigned long m_ulSize;

public:
		void setPacket(char *pSrc, unsigned long ulSize)
		{
			m_pPacket = (boost::shared_array<char>)(new char[ulSize]);
			memcpy(m_pPacket.get(), pSrc, ulSize);
			
			m_ulSize = ulSize; //add
		};

		unsigned long getPacketSize() //add
		{
			return m_ulSize;
		}

    boost::shared_array<char> getPacket()  
		{
			return m_pPacket; 
		};

    void setClient(boost::shared_ptr<C>pClient) 
		{ 
			m_pClient = pClient; 
		};

    boost::shared_ptr<C> getClient() 
		{
			return m_pClient; 
		};
};

#endif


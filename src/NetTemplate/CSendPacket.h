#ifndef __CSEND_PACKET__
#define __CSEND_PACKET__

#include <string>
using namespace std;

#include <boost/shared_ptr.hpp>
using namespace boost;

class CSendPacket
{
public:
	string m_IP;
	int m_Port;
	shared_ptr<char> m_data;
	int m_nSize;

public:
	CSendPacket(string IP, int Port, char *data, int nSize);
};

#endif

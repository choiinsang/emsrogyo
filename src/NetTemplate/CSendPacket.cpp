#include "CSendPacket.h"

CSendPacket::CSendPacket(string IP, int Port, char *data, int nSize)
: m_IP(IP), m_Port(Port), m_nSize(nSize)
{
	m_data = shared_ptr<char>(new char[m_nSize]);
	memcpy(m_data.get(), data, m_nSize);
}

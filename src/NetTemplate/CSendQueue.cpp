#include "CSendQueue.h"

CSendQueue::CSendQueue()
{
}

CSendQueue::~CSendQueue()
{
}

void CSendQueue::AddQueue(string IP, int Port, char *data, int nSize)
{
	Lock();
	m_queue.push(shared_ptr<CSendPacket>(new CSendPacket(IP, Port, data, nSize)));
	Unlock();
}

void CSendQueue::AddQueue(shared_ptr<CSendPacket> pPacket)
{
	Lock();
	m_queue.push(pPacket);
	Unlock();
}

int CSendQueue::GetSize()
{
	Lock();
	int size = m_queue.size();
	Unlock();
	return size;
}

shared_ptr<CSendPacket> CSendQueue::Pop()
{
	shared_ptr<CSendPacket> p;
	p.reset();
	Lock();
	if( !m_queue.empty() )
	{
		p = m_queue.front();
		m_queue.pop();
	}
	Unlock();
	return p;
}

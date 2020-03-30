#ifndef __CONSISTENT_HASH__
#define __CONSISTENT_HASH__

#include <map>
#include <algorithm>
using namespace std;

template < typename T >
class greater_first
{
private:
	T _theVal;
public:
	greater_first(T x_theVal):_theVal(x_theVal){};
	template < typename Tpair>
	bool operator() ( Tpair const & _sel ) const
	{
		return ( _sel.first > _theVal );
	};
};

template < typename T >
class lesser_first
{
private:
	T _theVal;
public:
	lesser_first(T x_theVal):_theVal(x_theVal){};
	template < typename Tpair>
	bool operator() ( Tpair const & _sel ) const
	{
		return ( _sel.first < _theVal );
	};
};

template < typename T >
class greater_equal_first
{
private:
	T _theVal;
public:
	greater_equal_first(T x_theVal):_theVal(x_theVal){};
	template < typename Tpair>
	bool operator() ( Tpair const & _sel ) const
	{
		return ( _sel.first >= _theVal );
	};
};

template < typename T >
class lesser_equal_first
{
private:
	T _theVal;
public:
	lesser_equal_first(T x_theVal):_theVal(x_theVal){};
	template < typename Tpair>
	bool operator() ( Tpair const & _sel ) const
	{
		return ( _sel.first <= _theVal );
	};
};

// N stands for node, K stands for key

template <typename N, typename K> 
class ConsistentHash
{
private:
	map<unsigned long, N> m_Continumm;

public:
	ConsistentHash();
	virtual ~ConsistentHash();

	virtual unsigned long GetHash(K key);
	bool AddNode(unsigned long position, N node);
	bool RemoveNode(unsigned long position);
	N GetNode(K key);
	
};

template <typename N, typename K> 
ConsistentHash<N, K>::ConsistentHash()
{
	m_Continumm.clear();
}

template <typename N, typename K> 
ConsistentHash<N, K>::~ConsistentHash()
{
}

template <typename N, typename K> 
unsigned long ConsistentHash<N, K>::GetHash(K key)
{
	return 0;
}

template <typename N, typename K> 
bool ConsistentHash<N, K>::AddNode(unsigned long position, N node)
{
	m_Continumm[position] = node;
	return true;
}

template <typename N, typename K> 
bool ConsistentHash<N, K>::RemoveNode(unsigned long position)
{
	m_Continumm.erase(position);
	return true;
}

template <typename N, typename K> 
N ConsistentHash<N, K>::GetNode(K key)
{
	typename map<unsigned long, N>::iterator I;

	I = find_if(m_Continumm.begin(), m_Continumm.end(), 
		greater_equal_first<unsigned long>(GetHash(key)));

	if( m_Continumm.end() != I )
		return (*I).second;
	else
		throw "Error on find_if";
}

#endif


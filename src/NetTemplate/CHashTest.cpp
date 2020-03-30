#include "ConsistentHash.h"

#include <math.h>

#include <string>
#include <iostream>
using namespace std;

#include <boost/shared_ptr.hpp>
#include <boost/functional/hash.hpp>
using namespace boost;

class Node
{
private:
	string m_IP;
	int m_Port;
		
public:
	Node(string IP, int Port) : m_IP(IP), m_Port(Port) { };

	string GetIP() { return m_IP; };
	int GetPort() { return m_Port; };
};

class MyHash : public ConsistentHash< shared_ptr<Node>, string >
{
public:
	MyHash() { };
	~MyHash() { };
	unsigned long GetHash(string key)
	{
		boost::hash<std::string> str_hash;
		return str_hash(key);
	} 
};

int main()
{
	shared_ptr<MyHash> pHash = shared_ptr<MyHash>(new MyHash());

	pHash->AddNode((unsigned long)(pow(2.0, 32.0)-1), shared_ptr<Node>(new Node("222.231.60.234", 4096)));
	pHash->AddNode((unsigned long)(pow(2.0, 31.0)-1), shared_ptr<Node>(new Node("222.231.60.233", 5096)));
	pHash->AddNode((unsigned long)(pow(2.0, 30.0)-1), shared_ptr<Node>(new Node("222.231.60.65", 4097)));

	try
	{
		shared_ptr<Node> pNode;
		pNode = pHash->GetNode((string)("ngn@gabia.com"));
		cout << "Server for ngn@gabia.com" << endl;
		cout << "IP : " << pNode->GetIP() << " Port : " << pNode->GetPort() << endl;
		pNode = pHash->GetNode((string)("jdw@gabia.com"));
		cout << "Server for jdw@gabia.com" << endl;
		cout << "IP : " << pNode->GetIP() << " Port : " << pNode->GetPort() << endl;
	}
	catch(const char *err)
	{
		cout << err << endl;
	}

	return 0;
}

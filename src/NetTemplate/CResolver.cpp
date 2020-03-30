#include "CResolver.h"

#include <boost/asio.hpp>
using namespace boost;
using namespace boost::asio::ip;

string CResolver::resolve(string hostname)
{
	asio::io_service io;
	tcp::resolver resolver(io);
	tcp::resolver::query query(tcp::v4(), hostname, "");
	boost::system::error_code error;

	tcp::resolver::iterator I = resolver.resolve(query, error);
	if( 0 != error ) throw ("error : " + error.message()).c_str();
	
	tcp::endpoint e = (*I);
	return e.address().to_string(); 
}

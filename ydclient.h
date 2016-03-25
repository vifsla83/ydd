#ifndef YDCLIENT_H
#define YDCLIENT_H

#include "ydremote.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>

namespace ydd
{
    class YdClient
    {
	public: 
	    YdClient(std::string& request, boost::asio::io_service& ios, bool useSandbox);
	    void handleConnect(const boost::system::error_code& error);
	    void handleHandshake(const boost::system::error_code& error);
	    void handleWrite(const boost::system::error_code& error);
	    void handleRead(const boost::system::error_code& error);
	private:
	    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
	    boost::asio::ip::tcp::resolver::iterator& hostIt_;
	    std::string request_;
	    bool useSandbox_;
	    const std::string& httpHeader_;
	    boost::asio::streambuf http_request_;
	    boost::asio::streambuf http_response_;
    };
}

#endif /* YDCLIENT_H */

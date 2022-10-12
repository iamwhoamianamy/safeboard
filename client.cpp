#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace asio = boost::asio;        // from <boost/asio.hpp>
using tcp = asio::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

// Performs an HTTP GET and prints the response
int main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if (argc < 2)
        {
            std::cout << "Usage: " << argv[0] << " /directory" << std::endl;
            return 1;
        }

        auto const host = "127.0.0.1";
        auto const port = std::atoll(argv[2]);
        auto const target = argv[1];
        int version = 11;

        // The io_context is required for all I/O
        asio::io_context ioc;

        // These objects perform our I/O
        tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);
        beast::tcp_stream stream(ioc);

        // Make the connection on the IP address we get from a lookup
        stream.connect(endpoint);

        // Set up an HTTP GET request message
        http::request<http::string_body> req { http::verb::get, target, version };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        std::cout << res << std::endl;

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error { ec };
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
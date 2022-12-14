#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <filesystem>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// Enum for suspicious file types 
enum class SusType
{
    None,
    Js,
    Unix,
    MacOS,
    Error
};

// Examples of suspicious lines
const std::string jsSus = "<script>evil_script()</script>";
const std::string unixSus = "rm -rf ~/Documents";
const std::string macOSSus = "system(\"launchctl load /Library/LaunchAgents/com.malware.agent\")";

struct ScanResults
{
    size_t jsFiles = 0;
    size_t unixFiles = 0;
    size_t macOSFiles = 0; 
    size_t errors = 0;
    size_t proccessed = 0;

    void add(SusType type)
    {
        switch (type)
        {
            case SusType::Js: jsFiles++; break;
            case SusType::Unix: unixFiles++; break;
            case SusType::MacOS: macOSFiles++; break;
            case SusType::Error: errors++; break;
            default: break;
        }

        proccessed++;
    }
};

SusType checkForSuspicion(const std::filesystem::path& path)
{
    std::ifstream fin;
    fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        fin.open(path, std::ios::in);

        std::string line;

        while (std::getline(fin, line))
        {
            if (path.string().find(".js") != std::string::npos)
            {
                if (line.find(jsSus) != std::string::npos)
                {
                    return SusType::Js;
                }
            }
            else
            {
                if (line.find(unixSus) != std::string::npos)
                {
                    return SusType::Unix;
                }
                else if (line.find(macOSSus) != std::string::npos)
                {
                    return SusType::MacOS;
                }
            }
        }        
    }
    catch(const std::exception& e)
    {
        // Could not open a file
        if(!fin.eof())
        {
            return SusType::Error;
        }
    }

    return SusType::None;
}

template <class Body, class Allocator, class Send>
void handleRequest(
    http::request<Body, http::basic_fields<Allocator>> &&req,
    Send &&send)
{
    // Returns a bad request response
    auto const badRequest = [&req](beast::string_view message)
    {
        http::response<http::string_body> res(http::status::bad_request, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(message);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const notFound = [&req](beast::string_view target)
    {
        http::response<http::string_body> res(http::status::not_found, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The folder '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const serverError = [&req](beast::string_view message)
    {
        http::response<http::string_body> res(http::status::internal_server_error, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(message) + "'";
        res.prepare_payload();
        return res;
    };

    // Returns a correct response
    auto const correctResponse = [&req](const ScanResults& scanResults, double time)
    {
        http::response<http::empty_body> res(http::status::ok, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::string_view("Processed"), std::to_string(scanResults.proccessed));
        res.set(boost::beast::string_view("Js"), std::to_string(scanResults.jsFiles));
        res.set(boost::beast::string_view("Unix"), std::to_string(scanResults.unixFiles));
        res.set(boost::beast::string_view("MacOS"), std::to_string(scanResults.macOSFiles));
        res.set(boost::beast::string_view("Errors"), std::to_string(scanResults.errors));
        res.set(boost::beast::string_view("Time"), std::to_string(time));
        res.keep_alive(req.keep_alive());
        return res;
    };

    // Make sure a request can be handled
    if (req.method() != http::verb::get)
    {
        return send(badRequest("Unknown HTTP-method"));
    }

    // Request path must be absolute and not contain "..".
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
    {
        return send(badRequest("Illegal request target"));
    }

    std::string path(req.target());
    
    // Make sure directory exists
    if(!std::filesystem::exists(path))
    {
        return send(notFound(path));
    }

    // Performs scanning of requested directory
    ScanResults scanResults;        
    auto start = std::chrono::steady_clock::now();

    for (const auto& file : std::filesystem::directory_iterator(path))
    {
        scanResults.add(checkForSuspicion(file.path()));
    }

    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() / 1000.0;

    // Responds to request
    return send(correctResponse(scanResults, time));
}

void fail(beast::error_code ec, char const *what)
{
    std::cout << "Fail on " << what << ": " << ec.message() << "\n";
}

// Performs io operations on request and response
class Session : public std::enable_shared_from_this<Session>
{
private:
    beast::tcp_stream _stream;
    beast::flat_buffer _buffer;
    http::request<http::string_body> _request;
    std::shared_ptr<void> _result;

public:
    Session(tcp::socket &&socket): _stream(std::move(socket)) { }

    // Starts asynchronous operation
    void run()
    {
        net::dispatch(
            _stream.get_executor(),
            beast::bind_front_handler(
                &Session::read,
                shared_from_this()));
    }

    void read()
    {
        _request = {};
        http::async_read(
            _stream, _buffer, _request,
            beast::bind_front_handler(
                &Session::onRead,
                shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytes_transferred)
    {
        // Generic lambda for extending a lifetime of a message
        auto sendLambda = [this]<bool isRequest, class Body, class Fields>(
            http::message<isRequest, Body, Fields> &&message)
        {
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(message));
            _result = sp;

            http::async_write(
                _stream,
                *sp,
                beast::bind_front_handler(
                    &Session::onWrite,
                    shared_from_this(),
                    sp->need_eof()));
        };

        boost::ignore_unused(bytes_transferred);

        // Client closed the connection
        if (ec == http::error::end_of_stream)
        {
            return closeSession();
        }

        // Error on reading
        if (ec)
        {
            return fail(ec, "read");
        }

        // Sends the response
        handleRequest(std::move(_request), sendLambda);
    }

    void onWrite(bool close, beast::error_code ec, size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            return fail(ec, "write");
        }

        if (close)
        {
            return closeSession();
        }

        // Delete response
        _result = nullptr;

        // Reads another request
        read();
    }

    void closeSession()
    {
        beast::error_code ec;
        _stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    }
};

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context &_ioc;
    tcp::acceptor _acceptor;

public:
    Listener(net::io_context &ioc, tcp::endpoint endpoint) :
        _ioc(ioc), _acceptor(net::make_strand(ioc))
    {
        try
        {
            // Standart acceptor opening procedure
            _acceptor.open(endpoint.protocol());
            _acceptor.set_option(net::socket_base::reuse_address(true));
            _acceptor.bind(endpoint);
            _acceptor.listen(net::socket_base::max_listen_connections);
        }
        catch(const boost::system::system_error& e)
        {
            fail(e.code(), "listen");
        }
    }

    // Starts accepting incoming connections
    void run()
    {
        accept();
    }

private:
    // Starts accepting incoming connections
    void accept()
    {
        _acceptor.async_accept(
            net::make_strand(_ioc),
            beast::bind_front_handler(
                &Listener::onAccept,
                shared_from_this()));
    }

    void onAccept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            fail(ec, "accept");
            return;
        }
        else
        {
            // Starting running a session
            std::make_shared<Session>(std::move(socket))->run();
        }

        // Repeat accepting of different connection
        accept();
    }
};

int main(int argc, char *argv[])
{
    // Setting up localhost server
    const auto address = net::ip::make_address("127.0.0.1");
    const u_short port = 8080;
    const size_t threads = 100;

    // Launching a listener
    net::io_context ioc(threads);
    std::make_shared<Listener>(ioc, tcp::endpoint(address, port))->run();

    // Activating io context
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
    {
        v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
    }
    ioc.run();

    return 0;
}
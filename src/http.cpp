

/**
 * @file http.cpp
 * 
 * @author Jorge Omar Medra (https://github.com/jorgemedra)
 * @brief 
 * @version 0.1
 * @date 2021-09-26
 * 
 * @copyright Copyright (c) 2021
 * 
 * 
 * 
 * URL to test the GET: http://worldclockapi.com/
 */
#include "ntwrk.h"
#include <iostream>
#include <memory>
#include <string_view>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>


namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace net = boost::asio;  // from <boost/asio.hpp>
namespace ssl = net::ssl;
using tcp = net::ip::tcp;     // from <boost/asio/ip/tcp.hpp>

using shrd_ptr_req = std::shared_ptr<boost::beast::http::request<boost::beast::http::dynamic_body>>;
using shrd_ptr_res = std::shared_ptr<boost::beast::http::response<boost::beast::http::dynamic_body>>;

using namespace jomt;

shrd_ptr_res HTTPClient::do_request(http::verb method,
                                    const std::string &host,
                                    const std::string &port,
                                    const std::string &resource,
                                    const std::string &params,
                                    const std::vector<std::pair<http::field, std::string>> &std_headers,
                                    const std::vector<std::pair<std::string, std::string>> &custom_headers,
                                    std::shared_ptr<std::vector<char>> payload,
                                    std::stringstream &log,
                                    beast::error_code &error)
{
    beast::flat_buffer buffer;
    //std::shared_ptr<http::response<http::dynamic_body>> res(new http::response<http::dynamic_body>());
    shrd_ptr_res res(new http::response<http::dynamic_body>());
    
    try
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);
        
        http::request<http::string_body> req{method, resource, 11}; //11 = 1.1
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        for(auto h : std_headers)
            req.set(h.first, h.second);

        for (auto h : custom_headers)
            req.set(h.first, h.second);

        log << "\n[REQUEST] To[" << host << ":" << port << "]\n"
            << req << "\n"
            << "............................................." << std::endl;

        http::write(stream, req);
        http::read(stream, buffer, *res);

        //stream.socket().shutdown(tcp::socket::shutdown_both, error);
        /*
        if (error && error != beast::errc::not_connected)
        {
            return res;
        }
        */
        if (error && error != beast::errc::not_connected)
            throw beast::system_error{error};
    }
    catch (std::exception const &e)
    {
        log << "Error: " << e.what() << std::endl;
    }

    log << "\n[RESPONSE] From [" << host << ":" << port << "]\n"
        << *res << "\n"
        << "............................................." << std::endl;

    return res;
}

shrd_ptr_res HTTPClient::do_request_ssl(http::verb method,
                                        const std::string &host,
                                        const std::string &port,
                                        const std::string &resource,
                                        const std::string &params,
                                        const std::vector<std::pair<http::field, std::string>> &std_headers,
                                        const std::vector<std::pair<std::string, std::string>> &custom_headers,
                                        std::shared_ptr<std::vector<char>> payload,
                                        std::stringstream &log,
                                        beast::error_code &error)
{
    beast::flat_buffer buffer;
    //std::shared_ptr<http::response<http::dynamic_body>> res(new http::response<http::dynamic_body>());
    shrd_ptr_res res(new http::response<http::dynamic_body>());

    try
    {
        ssl::context ctx(ssl::context::tlsv13_client);
        //ctx.set_verify_mode(ssl::verify_peer);
        ctx.set_verify_mode(ssl::verify_none);

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        //log << "1. Set SNI Hostname to [" << host << "]\n";
            // Set SNI Hostname (many hosts need this to handshake successfully)
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            error = ec;
            throw beast::system_error{ec};
        }

        //log << "2. Resolve host [" << host << "] and port [" << port << "].\n";
        auto const results = resolver.resolve(host, port);
        
        //log << "3. Connectiong stream.\n";
        beast::get_lowest_layer(stream).connect(results);
        //log << "4. Performing Handshake.\n";
        stream.handshake(ssl::stream_base::client);

        //log << "5. Preparing Request.\n";
        //http::request<http::string_body> req{method, resource, 11}; //11 = 1.1
        http::request<http::string_body> req{method, resource, 20}; //11 = 1.1

        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        for (auto h : std_headers)
            req.set(h.first, h.second);

        for (auto h : custom_headers)
            req.set(h.first, h.second);

        log << "\n[REQUEST] To[" << host << ":" << port << "]\n"
            << req << "\n"
            << "............................................." << std::endl;

        //log << "6. Performing Request.\n";
        http::write(stream, req, error);

        if (error && error != beast::errc::not_connected)
            throw beast::system_error{error};

        //log << "7. Reading Response.\n";
        size_t clenght = 0;
        clenght = http::read(stream, buffer, *res, error);
        
        if (error && error != beast::errc::not_connected)
            throw beast::system_error{error};

        //log << "\t7.1 Bytes Readed:[" << clenght  << "].\n";

        log << "\n[RESPONSE] From [" << host << ":" << port << "]\n"
            << *res << "\n"
            << "............................................." << std::endl;

        //log << "8. Shutdowing streaming..\n";
        
        stream.shutdown(error);
    }
    catch (std::exception const &e)
    {
        log << "\nError: " << e.what() << std::endl;
    }

    return res;
}

std::shared_ptr<response> HTTPClient::do_get(const std::string &host,
                                             const std::string &port,
                                             const std::string &resource,
                                             const std::string &params,
                                             std::stringstream &log)
{
        // authorization:
        //
        std::vector<std::pair<http::field, std::string>>
            std_headers{
                {http::field::host, host},
                {http::field::authorization, "Basic am9yZ2VtZWRyYToyZXdyZXR3ZTU0"},
                {http::field::user_agent, BOOST_BEAST_VERSION_STRING},
                {http::field::accept, "*.*"}};

        //std::vector<std::pair<std::string, std::string>> custom_headers{{"MyHeader", "X-Values"}, {"MyHeader_2", "Y-Values"}};
        std::vector<std::pair<std::string, std::string>> custom_headers{};
        std::shared_ptr<std::vector<char>> payload(new std::vector<char>(0));

        beast::error_code error;
        auto resp = do_request_ssl(http::verb::get, host, port, resource, params,
                                   std_headers, custom_headers, payload, log, error);

        std::shared_ptr<response> _resp(new response());

        _resp->status = resp->result_int();
        auto hdr = resp->base().begin();
        while (hdr != resp->base().end())
        {
            std::stringstream key;
            std::stringstream value;
            key << hdr->name();
            value << hdr->value();
            _resp->header.insert(std::pair<std::string, std::string>(key.str(), value.str()));
            hdr++;
    }


    if (resp->has_content_length())
    {
        size_t s_data = resp->body().size();        
        auto it = resp->body().data().begin();
        auto end = resp->body().data().end();

        size_t body_sz = resp->body().size();
        size_t body_rd = 0;
        
        while (body_rd < body_sz)
        {
            size_t temp_size = (*it).size();
            char *data = (char *)(*it).data();
            for (size_t i = 0; i < temp_size; i++)
                _resp->body.push_back(data[i]);
            body_rd += temp_size;
            it++;
        }
    }
    return _resp;
}
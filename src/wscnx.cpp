#include <iostream>
#include <memory>
#include <vector>
#include <mutex>
#include <string_view>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "ws.hpp"

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
namespace net  = boost::asio;
namespace ip = boost::asio::ip;
namespace beast = boost::beast;
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace boost::beast;
using namespace boost::beast::websocket;
using namespace jomt;


wscnx::wscnx(boost::asio::ip::tcp::socket &&sck,
               std::shared_ptr<wsserver> server) :  m_server{server}, 
                                                    m_ws(new t_ws_tcp(std::move(sck)))
{
    is_ssl = false;
    connection::type(jomt::socket_type::WEBSOCKET);
    read_metainfo(m_ws->next_layer().socket());
    connection::id(m_cnx_info.port);
}

wscnx::wscnx(boost::asio::ip::tcp::socket &&sck, boost::asio::ssl::context &cnx,
        std::shared_ptr<wsserver> server) : m_server{server},
                                            m_wss(new t_ws_ssl(std::move(sck), cnx)) 
{
    is_ssl = true;
    connection::type(jomt::socket_type::WEBSOCKET);    
    read_metainfo(m_wss->next_layer().next_layer().socket());
    connection::id(m_cnx_info.port);
}

wscnx::~wscnx()
{
    //std::cout << "[wscnx::[" << m_id << "]::~wscnx\n";
}

void wscnx::run()
{
    if(is_ssl)
        handshake_ssl();
    else
        accept_tcp();
}

void wscnx::handshake_ssl()
{
    //std::cout << "wscnx::[" << m_id << "]:: SSL async_handshake\n";
    m_wss->next_layer().async_handshake(
        ssl::stream_base::server,
        [self{shared_from_this()}](const boost::system::error_code &ec)
        {
            // std::cout << "wscnx::[" << self->m_id << "]::async_handshake: (" << ec.value() << ") " << ec.message() << "\n";
            if (ec)
            {
                self->close(beast::websocket::close_code::protocol_error, ec);
                return;
            }
            self->accept_ssl();
        });
}

void wscnx::accept_tcp()
{   
    m_ws->set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    m_ws->set_option(websocket::stream_base::decorator(
        [](websocket::response_type &res)
        {
            res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
        }));

    //std::cout << "wscnx::[" << m_id << "]:: TCP async_handshake\n";
    m_ws->async_accept(
        [self{shared_from_this()}](const boost::system::error_code &ec)
        {
            if (ec)
            {
                self->close(beast::websocket::close_code::protocol_error, ec);
                return;
            }
            // self->read_tcp();
            self->read();
        });
}

void wscnx::accept_ssl()
{
    m_wss->set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    m_wss->set_option(websocket::stream_base::decorator(
        [](websocket::response_type &res)
        {
            res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
        }));

    //std::cout << "wscnx::[" << m_id << "]:: SSL async_handshake\n";
    m_wss->async_accept(
        [self{shared_from_this()}](const boost::system::error_code &ec)
        {
            if (ec)
            {
                self->close(beast::websocket::close_code::protocol_error, ec);
                return;
            }
            // self->read_ssl();
            self->read();
        });
}

void wscnx::close(beast::websocket::close_code reason, const boost::system::error_code &ec)
{
    // std::cout << "\nwscnx::close\n";
    if (is_ssl && m_wss->is_open())
                     m_wss->close(beast::websocket::normal);
    else if (!is_ssl && m_ws->is_open())
        m_wss->close(beast::websocket::normal);
    
    if (!m_server.expired())
        m_server.lock()->cnx_closed(m_cnx_info, ec);
}

void wscnx::read()
{
    if (!is_ssl && !m_ws->is_open())
        return;
    else if (is_ssl  && !m_wss->is_open())
        return;

    auto f_read = [self{shared_from_this()}](const boost::system::error_code &ec, std::size_t bytes_transferred)
                 {
                    boost::ignore_unused(bytes_transferred);

                    // This indicates that the session was closed
                    if (ec == websocket::error::closed)
                    {
                        self->close(beast::websocket::close_code::normal, ec);
                        return;
                    }

                    if (ec)
                    {
                        self->close(beast::websocket::close_code::abnormal, ec);
                        return;
                    }

                    std::string data = beast::buffers_to_string(self->m_rx_buffer.data());
                    self->m_rx_buffer.consume(bytes_transferred);

                    if (!self->m_server.expired())
                    {
                        std::string_view vdata(data.c_str());
                        self->m_server.lock()->on_data_rx(self->m_cnx_info.id, vdata, self->cnx_info());
                    }
                    self->read(); 
                };//lambda

    if (!is_ssl)
        m_ws->async_read(m_rx_buffer, f_read);
    else
        m_wss->async_read(m_rx_buffer, f_read);
}

void wscnx::write(std::string_view data, bool close_on_write)
{
    std::unique_lock<std::mutex> u_lock(m_mtx_write);
    if ( (!is_ssl && !m_ws->is_open()) || (is_ssl && !m_wss->is_open()))
        return;

    boost::system::error_code ec;
    size_t bytes_transferred{0};
    if (is_ssl)
        bytes_transferred = m_wss->write(net::buffer(data), ec);
    else
        bytes_transferred = m_ws->write(net::buffer(data), ec);
    
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == websocket::error::closed)
    {
        // std::cout << "[wscnx::[" << self->m_id << "]::on wirite] Error: " << ec.message() << "\n";
        close(beast::websocket::close_code::normal, ec);
        return;
    }

    if (ec)
    {
        // std::cout << "[wscnx::[" << self->m_id << "]::on wirite] Error READING: " << ec.message() << "\n";
        close(beast::websocket::close_code::abnormal, ec);
        return;
    }

    if (close_on_write)
        close(beast::websocket::close_code::normal, ec);
}

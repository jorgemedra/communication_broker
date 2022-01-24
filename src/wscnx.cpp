#include <iostream>
#include <memory>
#include <vector>
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
    read_metainfo(m_wss->next_layer().next_layer().socket());
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
    read_metainfo(m_ws->next_layer().socket());

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
            self->read_tcp();
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
            self->read_ssl();
        });
}

void wscnx::close(beast::websocket::close_code reason, const boost::system::error_code &ec)
{
    
    if (is_ssl && m_wss->is_open())
        m_wss->close(beast::websocket::normal);
    if (!is_ssl && m_ws->is_open())
        m_wss->close(beast::websocket::normal);
    else 
        if (!m_server.expired())
            m_server.lock()->cnx_closed(m_cnx_info.id, ec);
}

void wscnx::read_tcp()
{
    if (!m_ws->is_open()) return;
    // Read a message into our buffer
    m_ws->async_read(m_rx_buffer, 
                    [self{shared_from_this()}](const boost::system::error_code &ec, std::size_t bytes_transferred)
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
                        
                        if(!self->m_server.expired())
                        {
                            std::string_view vdata(data.c_str());
                            self->m_server.lock()->on_data_rx(self->m_cnx_info.id, vdata, self);
                        }
                        self->read_tcp(); 
                    });//lambda
}

void wscnx::read_ssl()
{
    if(!m_wss->is_open()) return;
    // Read a message into our buffer
    m_wss->async_read(m_rx_buffer, 
                    [self{shared_from_this()}](const boost::system::error_code &ec, std::size_t bytes_transferred)
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
                        
                        if(!self->m_server.expired())
                        {
                            std::string_view vdata(data.c_str());
                            self->m_server.lock()->on_data_rx(self->m_cnx_info.id, vdata, self);
                        }
                        self->read_ssl(); 
                    });//lambda
}

void wscnx::write(std::string_view data, bool close_on_write)
{
    //std::cout << "[wscnx::[" << m_id << "]::wirite]\n";

    if(is_ssl)
        write_ssl(data, close_on_write);
    else
        write_tcp(data, close_on_write);
}

void wscnx::write_tcp(std::string_view data, bool close_on_write)
{
    if (!m_ws->is_open()) return;
    m_ws->async_write(net::buffer(data),
                      [self{shared_from_this()}, close_on_write](const boost::system::error_code &ec, std::size_t bytes_transferred)
                      {
                          boost::ignore_unused(bytes_transferred);

                          // This indicates that the session was closed
                          if (ec == websocket::error::closed)
                          {
                              // std::cout << "[wscnx::[" << self->m_id << "]::on wirite] Error: " << ec.message() << "\n";
                              self->close(beast::websocket::close_code::normal, ec);
                              return;
                          }

                          if (ec)
                          {
                              // std::cout << "[wscnx::[" << self->m_id << "]::on wirite] Error READING: " << ec.message() << "\n";
                              self->close(beast::websocket::close_code::abnormal, ec);
                              return;
                          }

                          if (close_on_write)
                              self->close(beast::websocket::close_code::normal, ec);
                      });
    // std::cout << "[wscnx::[" << self->m_id << "]::on wirite send [" << bytes_transferred << "] bytes .\n"; });
}

void wscnx::write_ssl(std::string_view data, bool close_on_write)
{
    if(!m_wss->is_open()) return;
    m_wss->async_write(net::buffer(data),
                      [self{shared_from_this()}, close_on_write](const boost::system::error_code &ec, std::size_t bytes_transferred)
                      {
                          boost::ignore_unused(bytes_transferred);

                          // This indicates that the session was closed
                          if (ec == websocket::error::closed)
                          {
                              // std::cout << "[wscnx::[" << self->m_id << "]::on wirite] Error: " << ec.message() << "\n";
                              self->close(beast::websocket::close_code::normal, ec);
                              return;
                          }

                          if (ec)
                          {
                              // std::cout << "[wscnx::[" << self->m_id << "]::on wirite] Error READING: " << ec.message() << "\n";
                              self->close(beast::websocket::close_code::abnormal, ec);
                              return;
                          }

                          if (close_on_write) 
                            self->close(beast::websocket::close_code::normal, ec);
                      });
    // std::cout << "[wscnx::[" << self->m_id << "]::on wirite send [" << bytes_transferred << "] bytes .\n"; });
}
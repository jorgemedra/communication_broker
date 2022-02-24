#include <iostream>
#include <memory>
#include <vector>
#include <mutex>
#include <string_view>
#include <vector>
// #include <boost/beast.hpp>
// #include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "tcp.hpp"

#include <bitset>

namespace asio  = boost::asio;
namespace ip    = boost::asio::ip;
namespace net   = boost::asio;
namespace ssl   = boost::asio::ssl;

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using t_sck = boost::asio::ip::tcp::socket;;
using t_sck_ssl = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

using namespace jomt;

tcpcnx::tcpcnx(t_sck &&sck,
               boost::asio::io_context &cnx, 
               boost::asio::ssl::context &sslcnx,
               std::shared_ptr<tcpserver> server, 
               bool use_ssl) :  m_server{server}, 
                                b_w_header{true},
                                m_payload_size{0}, 
                                m_sck(std::move(sck)),
                                m_ssck(cnx, sslcnx)
{

    is_ssl = use_ssl;
    connection::type(jomt::socket_type::SOCKET_IP);
    read_metainfo(m_sck);
    connection::id(m_cnx_info.port);

    if (is_ssl)
    {
        t_sck_ssl tmp_ssl(std::move(m_sck), sslcnx);
        m_ssck = std::move(tmp_ssl);
        read_metainfo(m_ssck.next_layer());
    }
    
}
            

tcpcnx::~tcpcnx()
{
    // std::cout << "[wscnx::[" << m_id << "]::~wscnx\n";
}

void tcpcnx::run()
{
    if (is_ssl)
        handshake_ssl();
    else
        read();
}

void tcpcnx::handshake_ssl()
{
    std::cout << "tcpcnx::[" << m_cnx_info.id << "]:: SSL async_handshake\n";
    
    m_ssck.async_handshake( ssl::stream_base::server,
        [self{shared_from_this()}](const boost::system::error_code &ec)
        {
            if (ec)
            {
                std::cout << "tcpcnx::[" << self->m_cnx_info.id << "]::async_handshake: Error: " << ec.message() << " END\n";
                self->close(ec);
            }
            else
            {
                std::cout << "tcpcnx::[" << self->m_cnx_info.id << "]::async_handshake  END\n";
                self->read();
            }
        });
}

void tcpcnx::close(const boost::system::error_code &ec)
{
    std::cout << "tcpcnx::[" << m_cnx_info.id << "]::Closing\n";
    if (is_ssl && m_ssck.next_layer().is_open())
    {
        std::cout << "tcpcnx::[" << m_cnx_info.id << "]::Closing ssl socket\n";
        m_ssck.next_layer().close();
    }
    else if (!is_ssl && m_sck.is_open())
    {
        std::cout << "tcpcnx::[" << m_cnx_info.id << "]::Closing socket\n";
        m_sck.close();
    }
    
    if (!m_server.expired())
        m_server.lock()->cnx_closed(m_cnx_info, ec);
}

void tcpcnx::read()
{
    if (is_ssl && !m_ssck.next_layer().is_open())
        return;
    else if (!is_ssl && !m_sck.is_open())
        return;
            
    auto f_asyn_read = [self{shared_from_this()}](const boost::system::error_code &ec, size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        if (ec)
        {
            self->close(ec);
            return;
        }

        if (self->b_w_header) // Header Received
        {
            self->b_w_header = false; // Go for Payload.
            
            int32_t size;                
            bool invertBytes = (bool) self->m_rx_b[0];
            if (invertBytes)
            {
                std::swap(self->m_rx_b[1], self->m_rx_b[4]);
                std::swap(self->m_rx_b[2], self->m_rx_b[3]);
            }
            std::memcpy(&size, &(self->m_rx_b[1]), 4);
            self->m_payload_size = size;

            // std::cout << "tcpcnx::[" << self->m_cnx_info.id << "]:: reading Payload Size:" << size << "\n";
        }
        else // Payload received
        {
            self->b_w_header = true;
            if (!self->m_server.expired())
            {
                std::string_view vdata(&(self->m_rx_b[0]), self->m_payload_size);
                std::cout << "tcpcnx::[" << self->m_cnx_info.id << "]:: reading data:" << vdata << "\n";

                self->m_server.lock()->on_data_rx(self->m_cnx_info.id, vdata, self->cnx_info());
            }
        }
        self->read();
    };


    // // Read a message into our buffer
    if(b_w_header)
        m_payload_size = tcpcnx::HEADER_SIZE;
    
    m_rx_b.resize(m_payload_size);

    if (!is_ssl)
        m_sck.async_read_some(asio::buffer(&m_rx_b[0], m_payload_size), f_asyn_read);
     else
        m_ssck.async_read_some(asio::buffer(&m_rx_b[0], m_payload_size), f_asyn_read);
}

void tcpcnx::write(std::string_view data, bool close_on_write)
{
    std::unique_lock<std::mutex> u_lock(m_mtx_write);
    
    if (is_ssl && !m_ssck.next_layer().is_open())
        return;
    else if (!is_ssl && !m_sck.is_open())
        return;

    boost::system::error_code ec;
    size_t bytes_transferred{0};

    if (is_ssl)
    {
        char b = 0x00;
        bytes_transferred = m_ssck.write_some(asio::buffer(&b, 1), ec);
        int32_t size = data.size();
        char *buff = static_cast<char *>(static_cast<void *>(&size));
        bytes_transferred = m_ssck.write_some(asio::buffer(buff, 4), ec);
        bytes_transferred = m_ssck.write_some(asio::buffer(data), ec);
    }
    else
    {
        char b = 0x00;
        bytes_transferred = m_sck.write_some(asio::buffer(&b,1), ec);
        int32_t size = data.size();
        char* buff = static_cast<char *>(static_cast<void *>(&size));
        bytes_transferred = m_sck.write_some(asio::buffer(buff, 4), ec);
        bytes_transferred = m_sck.write_some(asio::buffer(data), ec);
    }

    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        close(ec);
        return;
    }

    if (close_on_write)
        close(ec);
}
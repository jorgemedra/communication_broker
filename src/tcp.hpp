
#pragma once

#ifndef TCP_HPP
#define TCP_HPP

#include "ntwrk.hpp"
#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <vector>
#include <set>
#include <mutex>
#include <cassert>
#include <type_traits>
#include <unordered_map>
#include <stack>
#include <string_view>
#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>

namespace jomt
{
    class tcpcnx;
    class tcpserver;

    class tcpcnx : public std::enable_shared_from_this<tcpcnx>, public jomt::connection
    {
        static const int HEADER_SIZE = 5;
        using t_sck = boost::asio::ip::tcp::socket;
        using t_sck_ssl = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
        
        std::weak_ptr<tcpserver> m_server;
        boost::asio::streambuf m_rx_buffer;
        boost::asio::streambuf m_tx_buffer;
        std::vector<char> m_rx_b;
        std::mutex m_mtx_write;

        bool b_w_header;
        size_t m_payload_size;
        t_sck m_sck;
        t_sck_ssl m_ssck;

        void handshake_ssl();

        void read();
    public:
        tcpcnx(t_sck &&sck,
               boost::asio::io_context &cnx,
               boost::asio::ssl::context &sslcnx,
               std::shared_ptr<tcpserver> server,
               bool use_ssl);
        ~tcpcnx();

        void run();
        void close(const boost::system::error_code &ec);
        void write(std::string_view data, bool close_on_write = false);
    };

    class tcpserver : public std::enable_shared_from_this<tcpserver>, public ntwrk_basic
    {
        //TODO: Change it by a MACRO on Compiler time
        static const int MAX_CONNECTIONS_TCP = 100;

        std::shared_ptr<jomt::basic_server> m_mainsrv;
        int m_port;
        boost::asio::io_context m_ioc;
        boost::asio::ssl::context m_ssl_ioc;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::unordered_map<int, std::shared_ptr<tcpcnx>> m_cnxs;
        std::atomic<int> m_counter;
        std::mutex m_lockcnx;        
        bool m_is_ssl;

        void wait_for_connections();

        // void initIdQueue(int max_cnxs);
        std::pair<int, connection_info> regiter_tcpcnx(boost::asio::ip::tcp::socket &&socket);
        bool unregiter_tcpcnx(int id);

        tcpserver(std::shared_ptr<jomt::basic_server> server, int bind_port,
                  int max_cnxs = tcpserver::MAX_CONNECTIONS_TCP,
                  boost::asio::ssl::context::method mtd = boost::asio::ssl::context::tlsv12_server);

    public:
        static std::shared_ptr<tcpserver> create(std::shared_ptr<jomt::basic_server> server, int bind_port,
                                                 int max_cnxs = tcpserver::MAX_CONNECTIONS_TCP,
                                                 boost::asio::ssl::context::method mtd = boost::asio::ssl::context::tlsv12_server)
        {
            return std::shared_ptr<tcpserver>(new tcpserver(server, bind_port, max_cnxs, mtd));
        }

        std::shared_ptr<tcpserver> get()
        {
            return shared_from_this();
        }

        void set_ssl_options(std::string cert_path, std::string key_path, std::string dh_path);

        std::pair<bool, std::shared_ptr<tcpcnx>> fetch_cnx(int id);

        jomt::server_info info();
        void run();
        void onStart();
        void onStop();
        void onStop(const boost::system::error_code &ec);

        void cnx_closed(jomt::connection_info cnxi, const boost::system::error_code &ec);
        void write(int id, std::string_view data, bool close_it = false);
        void on_data_rx(int id, std::string_view data, connection_info cnxi);
    };

}

#endif


#pragma once
#ifndef WS_HPP
#define WS_HPP

#include "ntwrk.hpp"
#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <set>
#include <mutex>
#include <cassert>
#include <type_traits>
#include <unordered_map>
#include <stack>
#include <string_view>
#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/beast.hpp>

// Use (void) to silent unused warnings.
//#define assertm(exp, msg) assert(((void)msg, exp))

namespace jomt
{
    class wscnx;
    class wsserver;

    class wscnx : public std::enable_shared_from_this<wscnx>, public jomt::connection
    {
        using t_ws_tcp = boost::beast::websocket::stream<boost::beast::tcp_stream>;
        using t_ws_ssl = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

        std::weak_ptr<wsserver> m_server;
        boost::beast::flat_buffer m_rx_buffer;
        boost::beast::flat_buffer m_tx_buffer;

        std::unique_ptr<t_ws_tcp> m_ws;
        std::unique_ptr<t_ws_ssl> m_wss;
        std::mutex m_mtx_write;

        void handshake_ssl();

        void accept_tcp();
        void accept_ssl();

        void read();
    public:
        wscnx(boost::asio::ip::tcp::socket &&sck,
               std::shared_ptr<wsserver> server);

        wscnx(boost::asio::ip::tcp::socket &&sck, boost::asio::ssl::context &cnx,
              std::shared_ptr<wsserver> server);
        
        ~wscnx();
        void run();
        void close(boost::beast::websocket::close_code reason, const boost::system::error_code &ec);
        void write(std::string_view data, bool close_on_write=false);
    };
    
    class wsserver : public std::enable_shared_from_this<wsserver>, public ntwrk_basic
    {
        //TODO: Change it by a MACRO on Compiler time
        static const int MAX_CONNECTIONS_WS = 1024;

        std::shared_ptr<jomt::basic_server> m_mainsrv;
        int m_port;
        boost::asio::io_context m_ioc;
        boost::asio::ssl::context m_ssl_ioc;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::unordered_map<int, std::shared_ptr<wscnx>> m_cnxs;
        std::atomic<int> m_counter;
        // std::stack<int> m_stck_ids;
        std::mutex m_lockcnx;
        bool m_is_ssl;

        void wait_for_connections();

        // void initIdQueue(int max_cnxs);
        std::pair <int, connection_info> regiter_wscnx(boost::asio::ip::tcp::socket &&socket);
        bool unregiter_wscnx(int id);

        wsserver(std::shared_ptr<jomt::basic_server> server, int bind_port,
                 int max_cnxs = wsserver::MAX_CONNECTIONS_WS,
                 boost::asio::ssl::context::method mtd = boost::asio::ssl::context::tlsv12);

    public:
        static std::shared_ptr<wsserver> create(std::shared_ptr<jomt::basic_server> server, int bind_port,
                                                int max_cnxs = wsserver::MAX_CONNECTIONS_WS,
                                                boost::asio::ssl::context::method mtd = boost::asio::ssl::context::tlsv12)
        {
            return std::shared_ptr<wsserver>(new wsserver(server, bind_port, max_cnxs,mtd));
        }

        std::shared_ptr<wsserver> get()
        {
            return shared_from_this();
        }

        void set_ssl_options(std::string cert_path, std::string key_path, std::string dh_path);

        std::pair<bool, std::shared_ptr<wscnx>> fetch_cnx(int id);

        jomt::server_info info();
        void run();
        void onStart();
        void onStop();
        void onStop(const boost::system::error_code &ec);

        void cnx_closed(jomt::connection_info cnxi, const boost::system::error_code &ec);
        void write(int id, std::string_view data, bool close_it = false);
        void on_data_rx(int id, std::string_view data, connection_info cnxi);
    };


#pragma endregion

} //namespace jomt

#endif //WS_HPP
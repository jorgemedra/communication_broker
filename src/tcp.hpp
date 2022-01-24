
#pragma once

#ifndef TCP_HPP
#define TCP_HPP

namespace jomt
{
    class tcpcnx;
    class tcpserver;

    class tcpcnx : public std::enable_shared_from_this<wscnx>, public jomt::connection
    {
        using t_ws_tcp = boost::beast::websocket::stream<boost::beast::tcp_stream>;
        using t_ws_ssl = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

        std::weak_ptr<wsserver> m_server;
        boost::beast::flat_buffer m_rx_buffer;
        boost::beast::flat_buffer m_tx_buffer;

        std::unique_ptr<t_ws_tcp> m_ws;
        std::unique_ptr<t_ws_ssl> m_wss;

        void handshake_ssl();

        void accept_tcp();
        void accept_ssl();

        void read_tcp();
        void read_ssl();

        void write_tcp(std::string_view data, bool close_on_write);
        void write_ssl(std::string_view data, bool close_on_write);

    public:
        wscnx(short id, boost::asio::ip::tcp::socket &&sck,
               std::shared_ptr<wsserver> server) :  m_server{server}, 
                                                    m_ws(new t_ws_tcp(std::move(sck)))
        {
            is_ssl = false;
            connection::type(jomt::socket_type::WEBSOCKET);
            connection::id(id);
        }

        tcpcnx(short id, boost::asio::ip::tcp::socket &&sck, boost::asio::ssl::context &cnx,
               std::shared_ptr<wsserver> server) : m_server{server},
                                                   m_wss(new t_ws_ssl(std::move(sck), cnx))
        {
            is_ssl = true;
            connection::type(jomt::socket_type::WEBSOCKET);
            connection::id(id);
        }

        ~tcpcnx();

        void run();
        void close(boost::beast::websocket::close_code reason, const boost::system::error_code &ec);
        void write(std::string_view data, bool close_on_write=false);
    };

    class tcpserver : public std::enable_shared_from_this<wsserver>, public ntwrk_basic
    {
        //TODO: Change it by a MACRO on Compiler time
        static const int MAX_CONNECTIONS_TCP = 1024;

        int m_port;
        boost::asio::io_context m_ioc;
        boost::asio::ssl::context m_ssl_ioc;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::unordered_map<int, std::shared_ptr<wscnx>> m_cnxs;
        std::atomic<int> m_counter;
        std::stack<int> m_stck_ids;
        std::mutex m_lockcnx;
        bool m_is_ssl;
        void wait_for_connections();

        void initIdQueue(int max_cnxs);
        std::pair<int, std::shared_ptr<tcpcnx>> regiter_tcpcnx(boost::asio::ip::tcp::socket &&socket);
        bool unregiter_tcpcnx(int id);

    public:
        tcpserver(int bind_port,
                  int max_cnxs = tcpserver::MAX_CONNECTIONS,
                  boost::asio::ssl::context::method mtd = boost::asio::ssl::context::sslv23_server);

        void set_ssl_options(std::string cert_path, std::string key_path, std::string pem_path);

        void run(); // from ntwrk_basic
        void onStart(); // from ntwrk_basic
        void onStop();  // from ntwrk_basic
        void onStop(const boost::system::error_code &ec);

        void cnx_closed(int id, const boost::system::error_code &ec);

        void write(int id, std::string_view data, bool close_it = false);
        void write(std::shared_ptr<tcpcnx>, std::string_view data, bool close_it = false);

        virtual void on_server_start() = 0;
        virtual void on_server_stop(const boost::system::error_code &ec) = 0;
        virtual void on_new_connection(int id, std::shared_ptr<tcpcnx> cnx) = 0;
        virtual void on_connection_end(int id, const boost::system::error_code &ec) = 0;
        virtual void on_data_rx(int id, std::string_view data, std::shared_ptr<tcpcnx> cnx) = 0;
    };

}

#endif
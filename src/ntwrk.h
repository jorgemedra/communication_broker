
#pragma once
#ifndef NTWRK_H
#define NTWRK_H

#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#pragma region /*Headers for HTTP ans Websocket*/
#include <boost/beast.hpp>
#pragma endregion

namespace jomt
{
    enum socket_type:short
    {
        UKNOWN = 0,
        SOCKET_IP,
        WEBSOCKET
    };

    struct connection_info
    {
        int id{-1};
        socket_type type{socket_type::UKNOWN};
        std::string address{""};
        int port{-1};
        
        // connection_info(int c_id, 
        //                 socket_type c_type, 
        //                 std::string c_address, 
        //                 int c_port): id{c_id}, type{c_type},
        //                              address(c_address), port{c_port} {}
        // connection_info() = default;
        // ~connection_info() = default;
    };

    std::ostream& operator<<(std::ostream &out, const connection_info &cnx);

    class connection
    {    
    public:
        // int id() const { return m_id; }
        // std::string address() const {return m_address;}
        // int remote_port() const {return m_port;}
        // socket_type type() const  {return m_type;}

        const connection_info& cnx_info() { return std::ref(m_cnx_info);}

    protected:
        //int m_id{-1};
        bool is_ssl{false};
        //std::string m_address{""};
        //int m_port{-1};
        // socket_type m_type{socket_type::UKNOWN}; 
        connection_info m_cnx_info;

        //connection_info {m_id,m_type, m_address, m_port}

        void id(int s_id) { m_cnx_info.id = s_id;}
        void type(socket_type s_type) { m_cnx_info.type = s_type;}
        void read_metainfo(const boost::asio::ip::tcp::socket &sck)
        {
            m_cnx_info.address = sck.remote_endpoint().address().to_string();
            m_cnx_info.port = sck.remote_endpoint().port();
            //m_cnx_info = connection_info{m_id, m_type, m_address, m_port};
        }
        
    };

    struct response
    {
        int status{0};
        std::map<std::string,std::string> header;
        std::vector<char> body;

    };
    
    class ntwrk_basic
    {
        std::atomic<bool> _bRunning;
        std::thread *_tr;        
        void intRun();
  
    public:
        ntwrk_basic() : _bRunning{false} , _tr{nullptr} {};
        ~ntwrk_basic();
        
        void start();
        void stop();
        void join();
        bool isRunning() const;

        virtual void run() = 0;
        virtual void onStart() = 0;
        virtual void onStop() = 0;
    };

#pragma region // TCP

    class TcpCnx;
    class ScktServer;
    
    enum proto_stage{
        header = 0,
        payload
    };

    class TcpCnx
    {   
        static const size_t HDR_SIZE = 6;

        short _slot;        
        proto_stage _stage;
        std::unique_ptr<boost::asio::ip::tcp::socket> _sckt;
        std::vector<char> rx_data;
        size_t _bf_size;
    public:
        TcpCnx(boost::asio::io_context &ctx) : _slot{-1},
                                               _stage{proto_stage::header},
                                               _sckt(new boost::asio::ip::tcp::socket(ctx)),
                                               rx_data{},
                                               _bf_size{0} {}
        ~TcpCnx(){}

        short slot() const { return _slot;}
        void slot(short slt) { _slot = slt;}
        
        void reset_buffer(proto_stage stg, size_t sz_payload = 0)
        {
            _stage = stg;
            if (stg == proto_stage::header)
                _bf_size = HDR_SIZE;
            else
                _bf_size = sz_payload;

            rx_data.clear();
            rx_data.resize(_bf_size);
        }

        proto_stage stage(){return _stage;}
        //void stage(proto_stage stg) { _stage = stg;}
        boost::asio::ip::tcp::socket &
        socket() { return *_sckt; }
        std::vector<char> &rx_buffer() { return rx_data; }
        //void buffer_size(size_t sz) { _bf_size = sz; }
        size_t buffer_size() { return _bf_size; }
    };

    class ScktServer : public ntwrk_basic
    {
        int _bPort;
        boost::asio::io_context _ntwrk_context;
        boost::asio::ip::tcp::acceptor _acceptor;

        std::mutex _mtx_pool;
        std::vector <std::shared_ptr<TcpCnx>> _pool_cnxs;
        std::map<boost::asio::ip::port_type, short> _sktsUnvbl;
        std::set<short> _sktsAvbl;

        /* Machine state to Cnx/RX */
        void begin_accept();
        void accepted(std::shared_ptr<TcpCnx> cnx, const boost::system::error_code &error);

        void begin_rx_hdr(short idx_slot);
        void end_rx_hdr(const boost::system::error_code &error, // Result of operation.
                        std::size_t bytes_transferred           // Number of bytes received.
        );

        short queryFirstAvailable();
        void linkSocket(boost::asio::ip::port_type port, short idx_slot);
        void unlinkSocket(boost::asio::ip::port_type port, short idx_slot);

        void proccess_data(std::shared_ptr<TcpCnx> cnx, size_t bytes_transferred);
        void end_tx(std::shared_ptr<TcpCnx> cnx, const boost::system::error_code &ec, size_t bytes_transferred);

        public : 
        ScktServer(int bindPort);
        void onStart();
        void onStop();
        void run();

        void disconnect_cnx(boost::asio::ip::port_type port, short slot);

        void start_rx(std::shared_ptr<TcpCnx> cnx);
        void start_tx(std::shared_ptr<TcpCnx> cnx, std::string data);
        
    };

#pragma endregion

#pragma region //HTTP

    struct Methods
    {
        const std::string GET{"GET"};
        const std::string POST{"POST"};
    };

    class HTTPClient
    {

        using shrd_ptr_req = std::shared_ptr<boost::beast::http::request<boost::beast::http::dynamic_body>>;
        using shrd_ptr_res = std::shared_ptr<boost::beast::http::response<boost::beast::http::dynamic_body>>;

        shrd_ptr_res do_request(boost::beast::http::verb method,
                                const std::string &host,
                                const std::string &port,
                                const std::string &resource,
                                const std::string &params,
                                const std::vector<std::pair<boost::beast::http::field, std::string>> &std_headers,
                                const std::vector<std::pair<std::string, std::string>> &headers,
                                std::shared_ptr<std::vector<char>> payload,
                                std::stringstream &log,
                                boost::beast::error_code &error);

        shrd_ptr_res do_request_ssl(boost::beast::http::verb method,
                                    const std::string &host,
                                    const std::string &port,
                                    const std::string &resource,
                                    const std::string &params,
                                    const std::vector<std::pair<boost::beast::http::field, std::string>> &std_headers,
                                    const std::vector<std::pair<std::string, std::string>> &headers,
                                    std::shared_ptr<std::vector<char>> payload,
                                    std::stringstream &log,
                                    boost::beast::error_code &error);
        /*
        std::shared_ptr<response> do_request_ssl(boost::beast::http::verb method,
                                                 const std::string &host,
                                                 const std::string &port,
                                                 const std::string &resource,
                                                 const std::string &params,
                                                 const std::vector<std::pair<boost::beast::http::field, std::string>> &std_headers,
                                                 const std::vector<std::pair<std::string, std::string>> &headers,
                                                 std::shared_ptr<std::vector<char>> payload);
        */
    public :
        std::shared_ptr<response> do_get(const std::string &host,
                                         const std::string &port,
                                         const std::string &resource,
                                         const std::string &params,
                                         std::stringstream &log);
    };

    class HTTPServer
    {

    };

#pragma endregion

} //namespace jomt

#endif //NTWRK_H
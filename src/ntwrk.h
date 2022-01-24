
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
    };

    std::ostream& operator<<(std::ostream &out, const connection_info &cnx);

    class connection
    {    
    public:
        const connection_info& cnx_info() { return std::ref(m_cnx_info);}

    protected:
        bool is_ssl{false};
        connection_info m_cnx_info;
        void id(int s_id) { m_cnx_info.id = s_id;}
        void type(socket_type s_type) { m_cnx_info.type = s_type;}
        void read_metainfo(const boost::asio::ip::tcp::socket &sck)
        {
            m_cnx_info.address = sck.remote_endpoint().address().to_string();
            m_cnx_info.port = sck.remote_endpoint().port();
        }
        
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

} //namespace jomt

#endif //NTWRK_H
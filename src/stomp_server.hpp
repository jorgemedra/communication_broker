#ifndef IMPP_WSCNX_HPP
#define IMPP_WSCNX_HPP

#include "ws.hpp"
#include "tcp.hpp"
#include "stomp_session.hpp"
#include "stomp_protocol.hpp"
#include "stomp_subscription.hpp"
#include <memory>
#include <functional>
#include <string_view>
#include <vector>

namespace jomt::stomp
{

    //class stomp_sever : public jomt::wsserver
    class stomp_sever : public jomt::basic_server, public std::enable_shared_from_this<stomp_sever>
    {
        bool b_started;
        
        std::shared_ptr<jomt::wsserver> m_wsserver;
        std::shared_ptr<jomt::tcpserver> m_tcpserver;
        std::shared_ptr<stomp_session_manager> m_session_mng;
        std::shared_ptr<jomt::stomp::stomp_subscription_manager> m_subs_mng;
        stomp_profile_keys m_secret_keys;

        std::vector<std::string> fetch_header(std::shared_ptr<stomp_message> msg,
                                              jomt::connection_info cnx,
                                              std::list<std::string> headers_id,
                                              bool &error, bool close_on_error = false);
    
        void proccess_connect(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi);
        void proccess_message(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi);
        void proccess_disconnect(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi);
        void proccess_send_message(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi);
        void proccess_subscribe(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi);
        void proccess_unsubscribe(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi);

        void send_error_msg(jomt::connection_info cnxi, std::string message, bool close_cnx = false);
        void send_connected_msg(jomt::connection_info cnxi, std::shared_ptr<stomp_session> session);
        void send_receipt_msg(std::string receipt_id, jomt::connection_info cnxi, std::shared_ptr<stomp_session> session, bool close_it = false);
        void send_send_msg(std::shared_ptr<stomp_message> original_msg, std::string trans_id, std::shared_ptr<stomp_session> from_session, std::shared_ptr<stomp_session> to_session);
        void send_ack_msg(jomt::connection_info cnxi, std::shared_ptr<stomp_session> session, std::string trans_id, stomp_errors ec, std::string_view info);

        stomp_sever();
        void create_internal_servers(int tcp_port, int ws_port);
    public:
        static std::shared_ptr<stomp_sever> create(int tcp_port, int ws_port);
        // {
        //     std::shared_ptr<stomp_sever> ptr = std::shared_ptr<stomp_sever>(new stomp_sever());
        //     ptr->create_internal_servers(tcp_port, ws_port);
        //     return ptr;
        // }
        std::shared_ptr<stomp_sever> get();

        void set_secret_keys(std::string app_key, std::string agent_key, std::string super_agent_key,
                                std::string ervice_key, std::string adm_key);

        void set_ssl_options_on_ws(std::string cert_path, std::string key_path, std::string dh_path);
        void set_ssl_options_on_tcp(std::string cert_path, std::string key_path, std::string dh_path);

        void start();
        void stop();

        // Inherited methods from WSServer and TCPServer
        void on_server_start(server_info srvi);
        void on_server_stop(server_info srvi, const boost::system::error_code &ec);
        void on_new_connection(server_info srvi, connection_info cnxi);
        void on_connection_end(server_info srvi, connection_info cnxi, const boost::system::error_code &ec);
        void on_data_rx(server_info srvi, std::string_view data, connection_info cnxi);

        // Declaring the CallBack function that jomt::stomp::stomp_subscription_manager requieres
        void on_subscription_task_ends(std::shared_ptr<stomp_subscription_task> task,
                                       std::string info,
                                       stomp_errors ec);

        // void send_error_msg(int cnx_id, std::string message, bool close_cnx = false);
    };

}

#endif
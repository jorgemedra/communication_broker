#ifndef IMPP_WSCNX_HPP
#define IMPP_WSCNX_HPP

#include "ws.hpp"
#include "stomp_session.hpp"
#include "stomp_protocol.hpp"
#include "stomp_subscription.hpp"
#include <memory>
#include <functional>
#include <string_view>
#include <vector>

namespace jomt::stomp
{

    class stomp_sever : public jomt::wsserver
    {
        std::shared_ptr<stomp_session_manager> m_session_mng;
        std::shared_ptr<jomt::stomp::stomp_subscription_manager> m_subs_mng;
        stomp_profile_keys m_secret_keys;

        std::vector<std::string> fetch_header(std::shared_ptr<stomp_message> msg,
                                            std::shared_ptr<jomt::wscnx> cnx,
                                            std::list<std::string> headers_id,
                                            bool &error, bool close_on_error = false);

        void proccess_connect(std::shared_ptr<stomp_message> msg, std::shared_ptr<jomt::wscnx> cnx);
        void proccess_message(std::shared_ptr<stomp_message> msg, std::shared_ptr<jomt::wscnx> cnx);
        void proccess_disconnect(std::shared_ptr<stomp_message> msg, std::shared_ptr<jomt::wscnx> cnx);
        void proccess_send_message(std::shared_ptr<stomp_message> msg, std::shared_ptr<jomt::wscnx> cnx);
        void proccess_subscribe(std::shared_ptr<stomp_message> msg, std::shared_ptr<jomt::wscnx> cnx);
        void proccess_unsubscribe(std::shared_ptr<stomp_message> msg, std::shared_ptr<jomt::wscnx> cnx);

        void send_error_msg(std::shared_ptr<jomt::wscnx> cnx, std::string message, bool close_cnx=false);
        void send_connected_msg(std::shared_ptr<jomt::wscnx> cnx, std::shared_ptr<stomp_session> session);
        void send_receipt_msg(std::string receipt_id, std::shared_ptr<jomt::wscnx> cnx, std::shared_ptr<stomp_session> session, bool close_it = false);
        void send_send_msg(std::shared_ptr<stomp_message> original_msg, std::string trans_id, std::shared_ptr<stomp_session> from_session, std::shared_ptr<stomp_session> to_session);
        void send_ack_msg(int cnx, std::shared_ptr<stomp_session> session, std::string trans_id, stomp_errors ec, std::string_view info);

    public:
        stomp_sever(int bind_port) : jomt::wsserver{bind_port},
                                     m_session_mng{stomp_session_manager::instance()}
        {
            m_subs_mng = stomp_subscription_manager::instance();
            m_subs_mng->link_call_back_functions(std::bind(&stomp_sever::on_subscription_task_ends,
                                                               this,
                                                               std::placeholders::_1,
                                                               std::placeholders::_2,
                                                               std::placeholders::_3)
                                                               );
        }

        void set_secret_keys(std::string app_key, std::string agent_key, std::string super_agent_key,
                             std::string ervice_key, std::string adm_key);

        
        // Inherited methods(events)
        void on_server_start();
        void on_server_stop(const boost::system::error_code &ec);
        void on_new_connection(int id, std::shared_ptr<jomt::wscnx> cnx);
        void on_connection_end(int id, const boost::system::error_code &ec);
        void on_data_rx(int id, std::string_view data, std::shared_ptr<jomt::wscnx> cnx);

        void on_subscription_task_ends(std::shared_ptr<stomp_subscription_task> task,
                                       std::string info,
                                       stomp_errors ec);

        void send_error_msg(int cnx_id, std::string message, bool close_cnx = false);
    };

}

#endif
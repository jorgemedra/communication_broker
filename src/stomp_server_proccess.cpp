#include "ws.hpp"
#include "stomp_server.hpp"
#include "stomp_protocol.hpp"
#include "stomp_session.hpp"
#include <iostream>
#include <list>
#include <vector>
#include <memory>
#include <string_view>

namespace asio = boost::asio;
namespace ip = boost::asio::ip;
namespace beast = boost::beast;
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace jomt;
using namespace jomt::stomp;
using namespace boost::beast;
using namespace boost::beast::websocket;

using header_type = std::list<stomp_header>;

void stomp_sever::proccess_connect(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi)
{
    bool b_error{false};
    std::vector<std::string> hdrl = fetch_header(msg, cnxi,
                                               {stomp_headers::HDR_SECRET_KEY, 
                                                stomp_headers::HDR_LOGIN,
                                                stomp_headers::HDR_PASSCODE},
                                                b_error, true); 
    if (b_error) return;

    std::string secret_key = hdrl[0];
    std::string login_id = hdrl[1];
    std::string passcode = hdrl[2];

    // Valid the Application Secret Key
    if (secret_key.compare(m_secret_keys.sk_application) != 0)
    {
        std::cout << "[stomp_sever] proccess_connect [" << cnxi.id << "] Invalid Application Secret Key.\n";
        send_error_msg(cnxi, "Invalid Application Secret Key.", true);
        return;
    }

    // Detect the Profile of this agent
    stomp_profile profile = stomp_profile_keys::detect_profile(passcode, m_secret_keys);
    std::string proif_desc = stomp_profile_keys::profile_description(profile);
    if (profile == stomp_profile::empty)
    {
        std::cout << "[stomp_sever] proccess_connect [" << cnxi.id << "] Invalid Profile/Passcode.\n";
        send_error_msg(cnxi, "Invalid Profile/Passcode.", true);
        return;
    }

    auto [session, ec, pcnx, close_it] = m_session_mng->register_session(cnxi, login_id, profile);
    std::cout << "[stomp_sever] " << stomp_session::show_info(session);

    //std::cout << "[stomp_sever] EC : " << ec << "CloseIt:" << close_it << "\n " << stomp_session::show_info(session);
    if (ec != stomp_errors::OK)
    {
        std::stringstream out;
        bool b_end{false};
        if (ec == stomp_errors::NEW_CONNECTION_LOGGED)
        {
            out << "Connection disconnected due to: " << stomp::err_description(ec) << ". "
                << "This connection will be drop: " << pcnx << "\n";
        }
        else
        {
            out << "Connection rejected due to: " << stomp::err_description(ec) << ". "
                << "This connection will be drop: " << pcnx << "\n";
            b_end = true;
        }

        std::cout << "[stomp_sever] Session Register result: " << out.str();
        send_error_msg(pcnx, out.str(), close_it);
        if (b_end) return;
    }

    send_connected_msg(cnxi, session);
}

void stomp_sever::proccess_disconnect(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi)
{
    std::shared_ptr<stomp_session> session = m_session_mng->fetch_session_by_cnx(cnxi.id);

    std::cout << "stomp_sever::proccess_disconnect: " << cnxi;

    if (session->profile == stomp_profile::empty)
    {
        send_error_msg(cnxi, "This connection has no a logged session.", true);
        return;
    }

    bool b_error{false};
    std::vector<std::string> hdrl = fetch_header(msg, cnxi,
                                                 {stomp_headers::HDR_RECEIPT_ID},
                                                 b_error, true);
    if (b_error) return;

    std::string recep_id = hdrl[0];

    if (recep_id.compare("")==0)
        recep_id = "-na-";

    send_receipt_msg(recep_id, cnxi, session, true);
}

void stomp_sever::proccess_send_message(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi)
{
    bool b_error{false};
    std::vector<std::string> hdrl = fetch_header(msg, cnxi,
                                                 {stomp_headers::HDR_SESSION_ID,
                                                  stomp_headers::HDR_DEST,
                                                  stomp_headers::HDR_CONT_TYPE,
                                                  stomp_headers::HDR_CONT_LENGHT,
                                                  stomp_headers::HDR_TRANSACTION},
                                                 b_error, true);
    if (b_error) return;

    std::string session_id = hdrl[0];
    std::string dest = hdrl[1];
    std::string cont_type = hdrl[2];
    std::string cont_len = hdrl[3];
    std::string trans_id = hdrl[4];

    //auto session = m_session_mng -> fetch_session_by_cnx(cnx->id());
    auto session = m_session_mng->fetch_session_by_id(session_id);

    if (session->profile == stomp_profile::empty)
    {
        send_error_msg(cnxi, "This connection has no a logged session.", true);
        return;
    }

    std::shared_ptr<stomp_session> session_dest = m_session_mng->fetch_session_by_destination(dest);

    if (session_dest == m_session_mng->empty_session())
    {
        std::stringstream out;
        out << "The destination[" << dest << "] doesn't exist.";
        send_error_msg(cnxi, out.str());
        return;
    }

    std::cout << "\nstomp_sever::proccess_send_message:\n"
              << stomp_message::info(*msg);

    //int cnx_id = session->cnxs.front().id;

    // for (auto its = session_dest.begin(); its != session_dest.end(); its++)
    //     send_send_msg(msg, trans_id, session, *its);
    send_send_msg(msg, trans_id, session, session_dest);

    std::string info{""};
    send_ack_msg(cnxi, session, trans_id, stomp_errors::OK, info);
}

void stomp_sever::proccess_subscribe(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi)
{
    bool b_error{false};
    std::vector<std::string> hdrl = fetch_header(msg, cnxi,
                                                 {stomp_headers::HDR_SESSION_ID,
                                                  stomp_headers::HDR_DEST_TYPE,
                                                  stomp_headers::HDR_DEST,
                                                  stomp_headers::HDR_TRANSACTION},
                                                 b_error, true);
    if (b_error)
        return;

    std::string session_id = hdrl[0];
    std::string dest_type = hdrl[1];
    std::string dest = hdrl[2];
    std::string trans = hdrl[3];

    auto session = m_session_mng->fetch_session_by_id(session_id);
    if (session->profile == stomp_profile::empty)
    {
        send_error_msg(cnxi, "This connection has no logged a session.", true);
        return;
    }

    bool is_regex = dest_type.compare("1") == 0 ? true : false;
    m_subs_mng->async_init_subscription(session, dest, trans, is_regex);
}

void stomp_sever::proccess_unsubscribe(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi)
{
    bool b_error{false};
    std::vector<std::string> hdrl = fetch_header(msg, cnxi,
                                                 {stomp_headers::HDR_SESSION_ID,
                                                  stomp_headers::HDR_TRANSACTION},
                                                 b_error, true);
    if (b_error)
        return;

    std::string session_id = hdrl[0];
    std::string trans = hdrl[1];

    auto session = m_session_mng->fetch_session_by_id(session_id);
    if (session->profile == stomp_profile::empty)
    {
        send_error_msg(cnxi, "This connection has no logged a session.", true);
        return;
    }

    m_subs_mng->async_init_unsubscription(session, trans);

}

void stomp_sever::on_subscription_task_ends(std::shared_ptr<stomp_subscription_task> task,
                                            std::string info, stomp_errors ec)
{
    if(task->send_call_back)
    {
        auto cnxi = task->owner->cnxs.front();
        send_ack_msg(cnxi, task->owner, task->trans_id, ec, info);
    }
}
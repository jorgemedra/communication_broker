#include "ws.hpp"
#include "stomp_server.hpp"
#include "stomp_protocol.hpp"
#include "stomp_session.hpp"
#include <iostream>
#include <list>
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

// using header_str_type = std::list<std::pair<std::string, std::string>>;
// using header_long_type = std::list<std::pair<std::string, long>>;
 using header_type = std::list<stomp_header>;

void stomp_sever::on_server_start()
{
    std::cout << "[stomp_sever] Server has started\n";
}

void stomp_sever::set_secret_keys(  std::string app_key, std::string agent_key, std::string super_agent_key, 
                                    std::string service_key, std::string adm_key)
{
    m_secret_keys.sk_application = app_key;
    m_secret_keys.sk_agent = agent_key;
    m_secret_keys.sk_sagent = super_agent_key;
    m_secret_keys.sk_service = service_key;
    m_secret_keys.sk_admin = adm_key;
}

void stomp_sever::on_server_stop(const boost::system::error_code &ec)
{
    m_subs_mng->stop();
    if(ec)
        std::cout << "[stomp_sever] The server has been stoped with error: (" << ec.value()<< ")[" << ec.message() << "].\n";
    else
        std::cout << "[stomp_sever] The server has benn stoped.\n";
}

void stomp_sever::on_new_connection(int id, jomt::connection_info cnxi)
{
    std::cout << "[stomp_sever] on_new_connection with id [" << id << "]:\n";
    if(cnxi.type == jomt::WEBSOCKET)
    {
        auto [bexist, cnx] = wsserver::fetch_cnx(cnxi.id);
        if (bexist)
            cnx->run();
    }
    
}

void stomp_sever::on_data_rx(int id, std::string_view data, jomt::connection_info cnxi)
{
    stomp_errors ec;
    stomp_parser parser;
    std::shared_ptr<stomp_message> msg = parser.parser_message(data, ec);


    if(ec != stomp_errors::OK)
    {
        std::stringstream out;
        out << "CNX [" << id << "] INVALID MESSAGE: CAUSE: " << err_description(ec);

        std::cout << "WARN CNX [" << id << "] INVALID MESSAGE:"
                  << "\n\tError Description: " << err_description(ec)
                  << "\n\tRemote Source:[" << cnxi.address << ":" << cnxi.port << "]"
                  << "\n[---------------------- BEGIN WRONG MESSAGE ----------------------------------]\n"
                  << data
                  << "\n[---------------------- END WRONG MESSAGE ------------------------------------]\n";
        send_error_msg(cnxi, out.str(), true);
        return;
    }

    proccess_message(msg, cnxi);
}

void stomp_sever::on_connection_end(int id, const boost::system::error_code &ec)
{
    //std::cout << "[stomp_sever] On CNX Closed [" << id << "] Reason: (" << ec.value()<< ")" <<  ec.message() << ".\n";
    
    auto session = m_session_mng->fetch_session_by_cnx(id);

    std::cout << "[stomp_sever] On CNX Closed [" << id << "] Reason: (" << ec.value() << ")" << ec.message() << ".\n"
              << "from session: " << stomp_session::show_info(session);

    for (auto it = session->cnxs.begin(); it != session->cnxs.end(); it++)
    {
        if (it->id == id)
        {
            std::cout << "[stomp_sever]  On CNX Closed [" << id << "] Unliking Connection: "
                      << *it << "\n";
            session->cnxs.erase(it);
            break;
        }
    }

    if (session->cnxs.size() == 0)
    {
        // 1. Unsubscribe all the subscritptions and subscribers        
        //if (session != m_session_mng->empty_session())
        m_subs_mng->async_init_unsubscription(session, "cnx-end", false);
        
        //2. unregistrer session    
        m_session_mng->unregister_session(id);
    }
    else
        std::cout << "[stomp_sever]  On CNX Closed New Session State[" << id << "]: " << stomp_session::show_info(session);

}

std::vector<std::string> stomp_sever::fetch_header(std::shared_ptr<stomp_message> msg,
                                                   jomt::connection_info cnxi,
                                                   std::list<std::string> headers_id,
                                                   bool &b_error, bool close_on_error)
{
    std::vector<std::string> values{};
    b_error = false;

    for (auto ith = headers_id.begin(); ith != headers_id.end(); ith++)
    {
        auto it = msg->headers.find(*ith);

        if (it == msg->headers.end())
        {
            std::stringstream out;

            if (close_on_error)
                out << "BAD MESSAGE. There is no a valid [" << *ith << "] header. This sessión is going to be closed.";
            else
                out << "BAD MESSAGE. There is no a valid [" << *ith << "] header.";
            send_error_msg(cnxi, out.str(), close_on_error);
            b_error = true;
            break;
        }
        values.push_back(it->second);
    }

    return values;
}

void stomp_sever::proccess_message(std::shared_ptr<stomp_message> msg, jomt::connection_info cnxi)
{
    if (msg->command.compare(stomp_commands::CMD_CONNECT) == 0)
        proccess_connect(msg, cnxi);
    else if (msg->command.compare(stomp_commands::CMD_DISCONNECT) == 0)
        proccess_disconnect(msg, cnxi);
    else if (msg->command.compare(stomp_commands::CMD_SEND) == 0)
        proccess_send_message(msg, cnxi);
    else if (msg->command.compare(stomp_commands::CMD_SUBSCRIBE) == 0)
        proccess_subscribe(msg, cnxi);
    else if (msg->command.compare(stomp_commands::CMD_UNSUBSCRIBE) == 0)
        proccess_unsubscribe(msg, cnxi);
    else
    {
        // TODO: PRINT WARNINGS AND CLOSE THIS CONNECTION IF THE COMMAND IS NOT RECOGNIZED
        std::cout << "Unproccess Message: " << stomp_message::info(*msg);
    }
}
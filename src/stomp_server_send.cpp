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

using header_type = std::list<stomp_header>;


void stomp_sever::send_error_msg(int cnx_id, std::string message, bool close_cnx)
{
    std::stringstream out;
    std::string payload{""};
    header_type headers{{stomp_headers::HDR_ERROR_DESC, message}};
    stomp_message::gen_stomp_message(out, stomp_commands::CMD_ERROR, headers, payload);

    write(cnx_id, out.str(), close_cnx);
}

void stomp_sever::send_error_msg(std::shared_ptr<jomt::wscnx> cnx, std::string message, bool close_cnx)
{
    std::stringstream out;
    std::string payload{""};
    header_type headers{{stomp_headers::HDR_ERROR_DESC, message}};

    stomp_message::gen_stomp_message(out, stomp_commands::CMD_ERROR, headers, payload);

    write(cnx, out.str(), close_cnx);
}

void stomp_sever::send_connected_msg(std::shared_ptr<jomt::wscnx> cnx, std::shared_ptr<stomp_session> session)
{
    std::stringstream out;
    std::string payload{""};

    header_type headers{
        {stomp_headers::HDR_ACCEP_VERSION, STOMP_VERSION},
        {stomp_headers::HDR_SESSION_ID, session->session_id},
        {stomp_headers::HDR_LOGIN, session->login_id},
        {stomp_headers::HDR_PROF_CODE, (double)session->profile},
        {stomp_headers::HDR_PROF_DESC, stomp_profile_keys::profile_description(session->profile)},
        {stomp_headers::HDR_DEST, session->destiny_id},
        {stomp_headers::HDR_CNX_ID, (double)cnx->cnx_info().id }
    };

    stomp_message::gen_stomp_message(out, stomp_commands::CMD_CONNECTED, headers, payload);
    write(cnx, out.str(), false);
}

void stomp_sever::send_receipt_msg(std::string receipt_id, std::shared_ptr<jomt::wscnx> cnx, std::shared_ptr<stomp_session> session, bool close_it)
{
    std::stringstream out;
    std::string payload{""};

    header_type headers{
        {stomp_headers::HDR_SESSION_ID, session->session_id},
        {stomp_headers::HDR_LOGIN, session->login_id},
        {stomp_headers::HDR_DEST, session->destiny_id},
        {stomp_headers::HDR_CNX_ID, (double)cnx->cnx_info().id},
        {stomp_headers::HDR_RECEIPT_ID, receipt_id}};

    stomp_message::gen_stomp_message(out, stomp_commands::CMD_RECEIPT, headers, payload);
    write(cnx, out.str(), close_it);
}

void stomp_sever::send_send_msg(std::shared_ptr<stomp_message> original_msg, 
                                std::string trans_id,
                                std::shared_ptr<stomp_session> from_session, 
                                std::shared_ptr<stomp_session> to_session)
{
    std::stringstream out;
    std::string payload{""};

    header_type headers{
        {stomp_headers::HDR_TRANSACTION, trans_id},
        {stomp_headers::HDR_SESSION_ID, to_session->session_id},
        {stomp_headers::HDR_ORIGIN, from_session->login_id},
        {stomp_headers::HDR_DEST, from_session->destiny_id},
        {stomp_headers::HDR_CONT_TYPE, original_msg->headers[stomp_headers::HDR_CONT_TYPE]},
        {stomp_headers::HDR_CONT_LENGHT, original_msg->headers[stomp_headers::HDR_CONT_LENGHT]}
    };

    stomp_message::gen_stomp_message(out, stomp_commands::CMD_MESSAGE, headers, original_msg->payload);
    int cnx_id = to_session->cnxs.front().id;
    write(cnx_id, out.str(), false);

     //TODO: Send a copy to all its subscribers.
    for (auto it = to_session->subscribers.begin(); it != to_session->subscribers.end(); it++)
    {
        auto sub_ses = m_session_mng->fetch_session_by_id((*it));
        if (sub_ses->cnxs.size()>0)
            write(sub_ses->cnxs.front().id, out.str(), false);
    }
}

void stomp_sever::send_ack_msg( int cnx_id, std::shared_ptr<stomp_session> session, 
                                std::string trans_id, stomp_errors ec, std::string_view info)
{
    std::stringstream out;
    std::string payload{info};

    header_type headers{
        {stomp_headers::HDR_SESSION_ID, session->session_id},
        {stomp_headers::HDR_TRANSACTION, trans_id},
        {stomp_headers::HDR_ERROR_DESC, stomp::err_description(ec)}
    };

    stomp_message::gen_stomp_message(out, stomp_commands::CMD_ACK, headers, payload);
    write(cnx_id, out.str(), false);
}
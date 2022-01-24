
#include "stomp_protocol.hpp"
#include "jomt_util.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <list>

using namespace jomt::stomp;

bool stomp_commands::is_cmd_recognized(std::string &cmd)
{
    if(stomp_commands::CMD_CONNECT.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_CONNECTED.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_DISCONNECT.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_SEND.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_SUBSCRIBE.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_UNSUBSCRIBE.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_BEGIN.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_COMMIT.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_ABORT.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_ACK.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_NACK.compare(cmd) == 0) return true;
    else if(stomp_commands::CMD_ERROR.compare(cmd) == 0) return true;
    return false;
}

std::shared_ptr<stomp_message> stomp_parser::parser_message(std::string_view data, stomp_errors &ec)
{
    std::shared_ptr<stomp_message> msg(new stomp_message());
    size_t pos{0};

    short stage = 0; // 0=Command; 1=headers; 2=payload
    
    size_t nidx{0};
    size_t len_d{0};

    // Command
    parse_command(msg, data, pos, ec);

    if(ec == stomp_errors::OK)
        parse_headers(msg, data, pos, ec);

    if(ec == stomp_errors::OK)
    {
        if(pos < data.size() )
            msg->payload = data.substr(pos);
    }

    return msg;
}

void stomp_parser::parse_command(std::shared_ptr<stomp_message> msg, std::string_view data, size_t &pos, stomp_errors &ec)
{
    std::stringstream out;
    ec = stomp_errors::OK;
    char c = data.at(pos);

    while (pos < data.size() && c != '\n')
    {   
        if(c == '\0' || c == 0x00 || c == ' ') 
        {
            ec = stomp_errors::INVALID_FORMAT_COMMAND;
            return;
        }

        out << c;
        pos++;
        if(pos < data.size())
            c = data.at(pos);
    }

    msg->command = utils::strings::trim(out.str());

    if (!stomp_commands::is_cmd_recognized(msg->command))
        ec = stomp_errors::UNKOWN_COMMAND;
    else
        pos++;
}

void stomp_parser::parse_headers(std::shared_ptr<stomp_message> msg, std::string_view data, size_t &pos, stomp_errors &ec)
{
    std::string line;

    auto read_line = [data](size_t &pos, stomp_errors &ec){
        std::stringstream out{};
        bool kread{true};
        if(pos < data.size())
        {
            do
            {  
                char c = data.at(pos);
                
                if(c != '\n' && c != '\0')
                    out << c;
                else if(c == '\0')
                {
                    ec = stomp_errors::UNKOWN_FORMAT_HEADER;
                    kread = false;
                }
                else if(c == '\n')
                    kread = false;

                pos++;

                if(pos >= data.size())
                    kread = false;
                
            }while(kread);
        }
        return out.str();
    };
    
    bool b_kr{true};
    do
    {
        if(pos < data.size())
        {    
            line = read_line(pos, ec);
            if(line.compare("") != 0)
            {
                parse_head(msg, line, ec);
                if(ec != stomp_errors::OK)
                    return;
            }
            else
                b_kr = false;
        } 
        else
            b_kr = false;

    }while(b_kr);

}

void stomp_parser::parse_head(std::shared_ptr<stomp_message> msg, std::string data, stomp_errors &ec)
{   
    std::stringstream out;
    size_t pos{0};
    bool readKey{true};
    std::string key;
    std::string value;

    for(char c : data)
    {

        if(readKey && c == '\0')
        {
            ec = stomp_errors::UNKOWN_FORMAT_HEADER;
            return;
        }
        else if(readKey && c==':')
        {
            readKey = false;
            key = out.str();
        }
        else if(readKey)
            out << c;
        else if(!readKey)
        {
            value = data.substr(pos);
            break;
        }
        pos++;
    }

    if(pos == data.size())
        ec = stomp_errors::UNKOWN_FORMAT_HEADER;
    else
    {
        key = utils::strings::trim(key);
        value = utils::strings::trim(value);
        msg->headers[key] = value;
    }

}

std::string stomp_message::info(stomp_message &msg)
{
    std::stringstream out;
    out << "STOMP MESSAGE INFO:\n"
        << "\tCOMMAND:[" << msg.command << "]\n"
        << "\tHEADERS:\n";

    for (auto it = msg.headers.begin(); it != msg.headers.end(); it++)
        out << "\t\t[" << it->first << "]:[" << it->second << "]\n";

    out << "\tPAYLOAD:\n"
        << "::--------- INIT OF PAYLOAD ---------::\n"
        << msg.payload
        << "\n::--------- END OF PAYLOAD ---------::\n";

    return out.str();
}

void stomp_message::gen_stomp_message(std::stringstream &out,
                                        std::string command,
                                        std::list<stomp_header> &headers,
                                        std::string &payload)
{
    out.clear();
    out << command << "\n";

    for (auto it = headers.begin(); it != headers.end(); it++)
        out << *it<< "\n";

    out << "\n"
        << payload << 0x00;
}

#pragma region stomp_header

std::ostream &jomt::stomp::operator<<(std::ostream &out, const stomp_header &header)
{
    if (header.b_str)
        return out << header.m_key << ':' << header.m_str;

    return out << header.m_key << ':' << header.m_num;
}

#pragma endregion
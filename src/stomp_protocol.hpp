#ifndef STOMP_PROTOCOL_HPP
#define STOMP_PROTOCOL_HPP

#include<string>
#include <iostream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <list>
#include <typeinfo>

namespace jomt::stomp
{   
    const std::string  STOMP_VERSION{"1.2"};

    enum stomp_errors : short
    {
        OK = 0,
        INVALID_FORMAT_COMMAND,  // The STOMP Command has an invalid  format.
        UNKOWN_COMMAND,          // The STOMP Command is uknown.
        UNKOWN_FORMAT_HEADER,    // The Header has an invalid format-
        WRONG_PROFILE_REGISTERED,// The session is already logged with another profile.
        NEW_CONNECTION_LOGGED,   // A new connection was logged with the same login-id and profile.
        LOGIN_ID_ALREADY_LOGGED, // The session is already logged.
        PROF_NO_RIGHTS,          // The profile has no rigth to subscribe.
        NO_DESTINATION_SUB,      // There is no a destination tu subscribe.
        INVALID_DEST_TYPE,       // Invalid Detination Type Value.
    };

    static std::string err_description(stomp_errors ec)
    {
        switch(ec)
        {
            case stomp_errors::OK:
                return "OK";
            case stomp_errors::INVALID_FORMAT_COMMAND:
                return "The COMMAND doesn't meet the requeriments, check its format.";
            case stomp_errors::UNKOWN_COMMAND:
                return "The COMMAND is not recognized as a valid command.";
            case stomp_errors::UNKOWN_FORMAT_HEADER:
                return "The HEADER doesn't meet the requeriments, check its format.";
            case stomp_errors::WRONG_PROFILE_REGISTERED:
                return "The session is already logged with another profile.";
            case stomp_errors::NEW_CONNECTION_LOGGED:
                return "A new connection was logged with the same login-id and profile.";
            case stomp_errors::LOGIN_ID_ALREADY_LOGGED:
                return "The Login ID is already logged.";
            case stomp_errors::PROF_NO_RIGHTS:
                return "The profile has no rigth to subscribe to destinies.";
            case stomp_errors::NO_DESTINATION_SUB:
                return "There is no destiations to subscribe.";
            case stomp_errors::INVALID_DEST_TYPE:
                return "There is no destiations type on header 'destination-type'.";
        };

        return "Unkown Error.";
    }

    namespace stomp_commands
    {
        const std::string CMD_CONNECT{"CONNECT"};
        const std::string CMD_CONNECTED{"CONNECTED"};
        const std::string CMD_DISCONNECT{"DISCONNECT"};
        const std::string CMD_RECEIPT{"RECEIPT"};
        const std::string CMD_SEND{"SEND"};
        const std::string CMD_MESSAGE{"MESSAGE"};
        const std::string CMD_SUBSCRIBE{"SUBSCRIBE"};
        const std::string CMD_UNSUBSCRIBE{"UNSUBSCRIBE"};        
        const std::string CMD_BEGIN{"BEGIN"};
        const std::string CMD_COMMIT{"COMMIT"};
        const std::string CMD_ABORT{"ABORT"};
        const std::string CMD_ACK{"ACK"};
        const std::string CMD_NACK{"NACK"};
        const std::string CMD_ERROR{"ERROR"};

        static bool is_cmd_recognized(std::string& cmd);
    };

    namespace stomp_headers
    {
        const std::string HDR_ACCEP_VERSION{"accept-version"};
        const std::string HDR_ERROR_DESC{"error-desc"};
        const std::string HDR_LOGIN{"login"};
        const std::string HDR_DEST{"destination"};
        const std::string HDR_DEST_TYPE{"destination-type"};
        const std::string HDR_ORIGIN{"origin"};
        const std::string HDR_PASSCODE{"passcode"};
        const std::string HDR_SECRET_KEY{"secret-key"};
        const std::string HDR_SESSION_ID{"session-id"};
        const std::string HDR_PROF_CODE{"profile-code"};
        const std::string HDR_PROF_DESC{"profile-desc"};
        const std::string HDR_CNX_ID{"connection-id"};

        const std::string HDR_TRANSACTION{"transaction"};
        const std::string HDR_RECEIPT_ID{"receipt-id"};
        
        const std::string HDR_CONT_LENGHT{"content-length"};
        const std::string HDR_CONT_TYPE{"content-type"};         
    };


    class stomp_header
    {
        bool b_str;
        std::string m_key;
        std::string m_str;
        double m_num;

    public:
        stomp_header(std::string key, std::string value) : b_str{true}, m_key{key}, m_str{value}, m_num{0} {}
        stomp_header(std::string key, double value) : b_str{false}, m_key{key}, m_str{}, m_num{value} {}
        friend std::ostream &operator<<(std::ostream &out, const stomp_header &header);
        // std::string key(){return m_key;}
        // std::string value() { return b_str ? m_str : std::to_string(m_num);}
    };

    std::ostream& operator<<(std::ostream &out, const stomp_header& header);

    struct stomp_message
    {
        std::string command{};
        std::unordered_map<std::string,std::string> headers{};
        std::string payload{};

        static std::string info(stomp_message& msg);

        // static void create_stomp_message(std::stringstream &out,
        //                                  std::string command,
        //                                  std::list<std::pair<std::string, std::string>> &headers,
        //                                  std::string &payload);

        static void gen_stomp_message(std::stringstream &out,
                                                           std::string command,
                                                           std::list<stomp_header> &headers,
                                                           std::string &payload);
    };

    class stomp_parser
    {
        void parse_command(std::shared_ptr<stomp_message> msg, std::string_view data, size_t &pos, stomp_errors &ec);
        void parse_headers(std::shared_ptr<stomp_message> msg, std::string_view data, size_t &pos, stomp_errors &ec);
        void parse_head(std::shared_ptr<stomp_message> msg, std::string data, stomp_errors &ec);

    public:
        std::shared_ptr<stomp_message> parser_message(std::string_view data, stomp_errors &ec);
    };


} // namespace jomt::stomp

#endif
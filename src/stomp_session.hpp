#pragma once
#ifndef STOMP_SESSION_HPP
#define STOMP_SESSION_HPP

#include <iostream>
#include <unordered_map>
#include <set>
#include <memory>
#include <list>
#include <mutex>
#include <boost/system/error_code.hpp>
#include"ntwrk.hpp"

namespace jomt::stomp{

    struct stomp_message;
    enum stomp_errors:short;
    /**
     * @brief
     *
     * destination_id format:
     *      /{profile}/{unique id}
     * 
     *for example:
     
     *      /agent/{agent id}
     *      /service/{id}
     *      /admin/{entity}
     *
     * where profile can be:
     *      + empty     This profile it to specify an Empty Session. This one is not a valid destiny.
     *      + agent
     *      + service
     *      + admin
     */

    class wscnx;
    struct cnx_info;

    class stomp_error: public boost::system::error_category
    {
        std::string m_app;
        std::string m_message;
    public:
        stomp_error(std::string app, std::string message) : m_app{app}, m_message { message}{}

        const char *name() const noexcept { return m_app.c_str(); }
        std::string message(int ev) const { return m_message; }
    };

    enum stomp_profile : short
    {
        empty = 0,
        agent,
        super_agent,
        service,
        admin
    };

    struct stomp_profile_keys
    {
        std::string sk_application;
        std::string sk_agent;
        std::string sk_sagent;
        std::string sk_admin;
        std::string sk_service;

        static std::string profile_description(stomp_profile profile, bool for_dest=false)
        {
            switch (profile)
            {
            case stomp_profile::agent:
                return "agent";
            case stomp_profile::super_agent:
                return for_dest ? "agent":"super agent";
            case stomp_profile::service:
                return "service";
            case stomp_profile::admin:
                return "admin";
            case stomp_profile::empty:
                return "empty";
            }
            return "empty";
        }

        static stomp_profile detect_profile(std::string secret_key, stomp_profile_keys &keys)
        {
            if (secret_key.compare(keys.sk_agent) == 0)
                return stomp_profile::agent;
            else if (secret_key.compare(keys.sk_sagent) == 0)
                return stomp_profile::super_agent;
            else if (secret_key.compare(keys.sk_service) == 0)
                return stomp_profile::service;
            else if (secret_key.compare(keys.sk_admin) == 0)
                return stomp_profile::admin;
            return stomp_profile::empty;
        }
        
    };

    struct stomp_session
    {
        const std::string session_id;       // Unique Id of Session of stomp_server.
        const std::string login_id;  //Id of agent, which must be the login_id on CONNECT command.
        const stomp_profile profile; //profile of the
        const std::string destiny_id;
        std::list<jomt::connection_info> cnxs;
        std::set<std::string> subscribers;
        std::set<std::string> subscriptions;

        // stomp_session(int cnxid, std::string sessionid,
        //               std::string iphost, int remoteport,
        //               std::string agentid, stomp_profile prof) : cnx_id{cnxid}, session_id{sessionid},
        //                                                          ip_host{iphost}, remote_port{remoteport},
        //                                                          agent_id{agentid}, profile{prof},
        //                                                          destiny_id{"/" + stomp_profile_keys::profile_description(prof) + "/" + agentid} 
        // {}
        stomp_session(std::string agentid, stomp_profile prof);

        static std::string gen_session_id();

        static std::string show_info(std::shared_ptr<stomp_session> session);  
    };

    class stomp_session_manager
    {
        std::mutex mtx_session;
        std::mutex mtx_search;

        std::unordered_map<std::string, std::shared_ptr<stomp_session>> m_sessions;
        std::unordered_map<std::string, std::string> m_login_session;
        std::unordered_map<std::string, std::string> m_dest_session;
        std::unordered_map<int, std::string> m_cnx_session;
        
        std::shared_ptr<stomp_session> m_null_session;
        std::pair<bool, std::shared_ptr<stomp_session>> retrive_session(std::string loginid, stomp_profile prof);
        
        stomp_session_manager();

    public:

        static std::shared_ptr<stomp_session_manager> instance();

        std::shared_ptr<stomp_session> empty_session();

        std::tuple<std::shared_ptr<stomp_session> , stomp_errors, jomt::connection_info, bool> 
                        register_session(jomt::connection_info cnxinf, std::string loginid, stomp_profile prof);

        void unregister_session(int cnx_id);

        std::shared_ptr<stomp_session> fetch_session_by_id(const std::string session_id);
        
        std::shared_ptr<stomp_session> fetch_session_by_cnx(const int cnx_id);
        std::shared_ptr<stomp_session> fetch_session_by_destination(std::string dest);
        std::list<std::shared_ptr<stomp_session>> fetch_all_sessions();        
    };
}

#endif
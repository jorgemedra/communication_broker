#include "stomp_session.hpp"
#include "stomp_protocol.hpp"
#include "ws.hpp"
#include <mutex>
#include <memory>
#include <ctime>

using namespace jomt::stomp;

using ptr_session = std::shared_ptr<stomp_session>;

#pragma region stomp_session

stomp_session::stomp_session(std::string loginid, stomp_profile prof) : login_id{loginid},
                                                                        session_id{gen_session_id()},
                                                                        profile{prof},
                                                                        destiny_id{"/" + stomp_profile_keys::profile_description(prof, true) + "/" + loginid},
                                                                        cnxs{},
                                                                        subscribers{},
                                                                        subscriptions{}
{
}

std::string stomp_session::gen_session_id()
{
    static std::mutex mtx_session_id{};
    static long m_counter{0};
    static std::time_t m_second{0};

    std::unique_lock<std::mutex> u_lock(mtx_session_id);
    std::time_t c_time = std::time(nullptr);
    std::stringstream out;
    if (c_time != m_second)
    {
        m_second = c_time;
        m_counter = 0;
    }
    m_counter++;

    out << m_second << ":" << m_counter;
    return out.str();
}

std::string stomp_session::show_info(ptr_session session)
{
    std::stringstream out;
    out << "Session Info:\n"
        << "\tSession ID:[" << session->session_id << "]\n"
        << "\tLogin ID:[" << session->login_id << "]\n"
        << "\tProfile:[" << stomp_profile_keys::profile_description(session->profile) << "]\n"
        << "\tDest:[" << session->destiny_id << "]\n";
        for (auto it = session->cnxs.begin(); it != session->cnxs.end(); it++)
        {
            jomt::connection_info ci = *it;
            out << "\tCNX: " << ci << "\n";
        }
        out << "\tSubscribers:\n";
        for (auto it = session->subscribers.begin(); it != session->subscribers.end(); it++)
            out << "\t\tSession Id: [" << *it << "]\n";
        out << "\tSubscriptions:\n";
        for (auto it = session->subscriptions.begin(); it != session->subscriptions.end(); it++)
            out << "\t\tSession Id: [" << *it << "]\n";

        return out.str();
}

#pragma endregion

#pragma region stomp_session_manager

std::shared_ptr<stomp_session_manager> stomp_session_manager::instance()
{   
    using type_ins = std::shared_ptr<stomp_session_manager>;
    static std::mutex mtx_fac_session;
    static std::lock_guard<std::mutex> lock_fact(mtx_fac_session);
    static type_ins st_unqique_sm(new stomp_session_manager());
    return st_unique_sm;
}

stomp_session_manager::stomp_session_manager() : m_sessions{}, m_dest_session{},
                                                 m_login_session{}, m_cnx_session{},
                                                 m_null_session{new stomp_session("", stomp_profile::empty)}
{}

ptr_session stomp_session_manager::empty_session()
{
    return m_null_session;
}

#pragma region find

ptr_session stomp_session_manager::fetch_session_by_id(const std::string session_id)
{
    std::unique_lock<std::mutex> u_lock(mtx_session);

    std::shared_ptr<stomp_session> ret_session = m_null_session;

    auto its = m_sessions.find(session_id);
    if (its != m_sessions.end())
        ret_session = its->second;

    return ret_session;
} 

ptr_session stomp_session_manager::fetch_session_by_cnx(const int cnx_id)
{
    std::unique_lock<std::mutex> u_lock(mtx_session);

    std::shared_ptr<stomp_session> ret_session = m_null_session;

    auto itc = m_cnx_session.find(cnx_id);
    if(itc != m_cnx_session.end())
    {
        auto its = m_sessions.find(itc->second);
        if (its == m_sessions.end())
            ret_session = m_null_session;
        else
            ret_session = its->second;
    }
    
    return ret_session;
}

ptr_session stomp_session_manager::fetch_session_by_destination(std::string dest)
{
    std::unique_lock<std::mutex> u_lock(mtx_session);

    ptr_session ret_session = m_null_session;

    auto itd = m_dest_session.find(dest);
    if (itd != m_dest_session.end())
    {
        auto its = m_sessions.find(itd->second); //seconf = agent_id

        if (its != m_sessions.end())
            ret_session = its->second;
    }

    return ret_session;
}

std::list<ptr_session> stomp_session_manager::fetch_all_sessions()
{
    std::unique_lock<std::mutex> u_lock(mtx_session);
    std::list<ptr_session> sessions{};

    for (auto it = m_sessions.begin(); it != m_sessions.end(); it++)
        sessions.push_back(it->second);
    return sessions;
}

#pragma endregion


#pragma region register

std::pair<bool, ptr_session> stomp_session_manager::retrive_session(std::string loingid, stomp_profile prof)
{
    std::shared_ptr<stomp_session> ret_session;
    bool isnew{false};

    auto ita = m_login_session.find(loingid);
    if (ita != m_login_session.end())
    {
        auto its = m_sessions.find(ita->second);
        ret_session = its->second;
    }
    else
    {
        ret_session = std::make_shared<stomp_session>(loingid, prof);
        isnew = true;
    }
    //ret_session->cnxs.insert(cnxid);
    return {isnew, ret_session};
}

//
// fist:    Error Code
// second:  Band wich says if the connection MUST be closed or not.
//std::tuple<stomp_errors, int, bool> stomp_session_manager::register_session(std::shared_ptr<stomp_session> session)
// std::tuple<std::shared_ptr<stomp_session>, stomp_errors, jomt::connection_info, bool> stomp_session_manager::register_session(jomt::connection_info cnxinf,
                                                                                                                    //    std::string loginid,
                                                                                                                    //    stomp_profile prof)
/**
 * @brief 
 * 
 * @param cnxinf  The connection from which the session is working.
 * @param loginid The Login-Id of the session.
 * @param prof The profile of the session.
 * @return std::tuple<std::shared_ptr<stomp_session>, stomp_errors, jomt::connection_info, bool> 
 */
std::tuple<std::shared_ptr<stomp_session>, stomp_errors, jomt::connection_info, bool>
stomp_session_manager::register_session(jomt::connection_info cnxinf, std::string loginid, stomp_profile prof)
{
    std::unique_lock<std::mutex> u_lock(mtx_session);
    
    stomp_errors ec = stomp_errors::OK;
    jomt::connection_info cnx_to_close{cnxinf};
    bool close_session{false};

    auto [is_new, session] = retrive_session(loginid, prof);

    if (!is_new) //The session is not new, so there must be a previous session.
    {
        if (session->profile == stomp_profile::agent || session->profile == stomp_profile::super_agent)
        {
            // If the profile is Agent or SuperAgent, the previous connection must be closed.
            if (session->profile != prof)
                ec = stomp_errors::WRONG_PROFILE_REGISTERED;
            else
            {
                ec = stomp_errors::NEW_CONNECTION_LOGGED;
                cnx_to_close = session->cnxs.front(); //previous connection
                m_cnx_session[cnxinf.id] = session->session_id;
                session->cnxs.push_back(cnxinf);
            }
        }
        else //The current connection must be closed because it's not allowed replace connections of Services or Admin
            ec = stomp_errors::LOGIN_ID_ALREADY_LOGGED;

        close_session = true;
    }
    else
    {
        m_sessions[session->session_id] = session;
        m_dest_session[session->destiny_id] = session->session_id;
        m_login_session[session->login_id] = session->session_id;
        m_cnx_session[cnxinf.id] = session->session_id;
        session->cnxs.push_back(cnxinf);
    }
    
    return std::make_tuple(session, ec, cnx_to_close, close_session);
}

void stomp_session_manager::unregister_session(int cnx_id)
{
    std::unique_lock<std::mutex> u_lock(mtx_session);
    auto itc = m_cnx_session.find(cnx_id);
    if (itc != m_cnx_session.end())
    {
         auto its = m_sessions.find(itc->second); //Fetch the session
         if (its != m_sessions.end())
         {
             auto session = its->second;
            // 1. remove all the connections linked to the sesssion
            for (auto it = session->cnxs.begin(); it != session->cnxs.end(); it++)                
                m_cnx_session.erase(it->id);

            // 2. Unlink its destination link
            m_dest_session.erase(session->destiny_id);
            // 3. Unlink its login-id link
            m_login_session.erase(session->login_id);
            
            // 4. Unlink its login-id link
            m_sessions.erase(session->session_id);
            std::cout << "unregister_session: " << stomp_session::show_info(session);
        }
    } // if (itc != m_cnx_session.end())
}
// :#pragma endregion

#pragma endregion
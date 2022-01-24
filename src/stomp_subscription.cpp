#include "stomp_subscription.hpp"
#include "stomp_protocol.hpp"
#include <memory>
#include <cstdio>
#include <regex>

using namespace jomt::stomp;
using smrt_sbs_mngr = std::shared_ptr<stomp_subscription_manager> ;
using subs_task = stomp_subscription_task;
using smrt_ptr_tsk = std::shared_ptr<subs_task>;
using smrt_ptr_s = std::shared_ptr<stomp_session>;
using type_cb_task_end = std::function<void(stomp_subscription_tasks,
                                            std::shared_ptr<stomp_subscription_task>,
                                            std::string,
                                            stomp_errors)>;

smrt_sbs_mngr stomp_subscription_manager::instance()
{
    using type_ins = std::shared_ptr<stomp_subscription_manager>;
    static std::mutex mtx_fac;
    static std::lock_guard<std::mutex> lock_fact(mtx_fac);
    static type_ins st_unique(new stomp_subscription_manager());
    return st_unique;
}

stomp_subscription_manager::stomp_subscription_manager() : m_queue_eng(std::bind(&stomp_subscription_manager::on_task, this, std::placeholders::_1)),
                                                           m_session_mng{stomp_session_manager::instance()}
{
    m_queue_eng.start();
}

void stomp_subscription_manager::link_call_back_functions(type_cb_task_end m_callback_unsub)
{
    m_cb_on_task_end = m_callback_unsub;
}

void stomp_subscription_manager::stop()
{
    m_queue_eng.stop();
}

void stomp_subscription_manager::on_task(smrt_ptr_tsk task)
{
    if (task->type_task == stomp_subscription_tasks::SUBSCRIBE)
        perform_subscription(task);
    else if (task->type_task == stomp_subscription_tasks::UNSUBSCRIBE)
        perform_unsubscription(task);
    else{
        //TODO: Mandar un acuse de notificacion que diga commando no reconocido.
    }
}

void stomp_subscription_manager::async_init_subscription(std::shared_ptr<stomp_session> session_owner,
                                                    std::string destination,
                                                    std::string trans_id, bool is_regex)
{
    smrt_ptr_tsk task(new stomp_subscription_task{
        trans_id,
        stomp_subscription_tasks::SUBSCRIBE,
        is_regex ? stomp_destination_type::REGEX : stomp_destination_type::DIRECT,
        destination,
        session_owner,true});

    m_queue_eng.add_task(task);
}

void stomp_subscription_manager::perform_subscription(std::shared_ptr<stomp_subscription_task> task)
{
    std::stringstream out;
    std::list<smrt_ptr_s> se_candidates{};
    stomp::stomp_errors ec{stomp_errors::OK};

    out << "Destinations subscribed:"
        << "\n--------------------";

    if (task->owner->profile == stomp_profile::empty ||
        task->owner->profile == stomp_profile::agent ||
        task->owner->profile == stomp_profile::admin)
        ec = stomp_errors::PROF_NO_RIGHTS;
    else{ //Profile with rights
        if (task->dest_type == stomp_destination_type::DIRECT)
            se_candidates.push_back(m_session_mng->fetch_session_by_destination(task->destination));
        else if (task->dest_type == stomp_destination_type::REGEX)
            se_candidates = m_session_mng->fetch_all_sessions();
        else
            ec = stomp_errors::INVALID_DEST_TYPE;
    } //Profile with rights

    if (ec == stomp_errors::OK)
    {
        if (se_candidates.size() > 0)
        {
            std::regex pattern{task->destination};
            for (auto it = se_candidates.begin(); it != se_candidates.end(); it++)
            {
                /*
                The subscribe must be Hieger than the profile.
                Herarchies:
                    Service Profile > Super Agent Profile
                    Super Agent Profile > Agent Profile

                    task->owner = Session whicha is the subscriber
                    it = The session candidate to subscribe
                */
                if (task->owner->profile > (*it)->profile)
                {
                    bool sub_to_sess{true};
                    if (task->dest_type == stomp_destination_type::REGEX)
                    {
                        std::smatch matches;
                        sub_to_sess = std::regex_match((*it)->destiny_id, matches, pattern);
                    }

                    if (sub_to_sess)
                    {       
                        auto ses_subsr = task->owner; //Subscriber
                        auto ses_subsc = *it;  //Subscription
                        ses_subsc->subscribers.insert(ses_subsr->session_id);
                        ses_subsr->subscriptions.insert(ses_subsc->session_id);
                        out << "\n[" << ses_subsc->login_id << "]:[" << ses_subsc->session_id << "]:[" << ses_subsc->destiny_id << "]";

                        std::cout << "\n\nperform_subscription: Session Subscription update:\n\t" << stomp_session::show_info(*it);
                    }

                } // if (task->owner->profile > (*it)->profile)
            } // for (auto it = se_candidates.begin(); it != all_sess.end(); it++)
        } // if (se_candidates.size() > 0)
        else if (ec == stomp_errors::OK)
            ec = stomp_errors::NO_DESTINATION_SUB;
    }

    out << "\n--------------------";
    m_cb_on_task_end(task, out.str(), ec);
}

void stomp_subscription_manager::async_init_unsubscription(std::shared_ptr<stomp_session> session_owner,
                                                           std::string trans_id, bool send_ack_cb)
{
    smrt_ptr_tsk task(new stomp_subscription_task{
        trans_id,
        stomp_subscription_tasks::UNSUBSCRIBE,
        stomp_destination_type::DISABLE,
        "",
        session_owner, send_ack_cb});

    m_queue_eng.add_task(task);
}

void stomp_subscription_manager::perform_unsubscription(std::shared_ptr<stomp_subscription_task> task)
{
    std::stringstream out;
    //std::list<smrt_ptr_s> se_candidates{};
    stomp::stomp_errors ec{stomp_errors::OK};
    
    out << "Destinations unsubscribed:"
        << "\n--------------------";

    auto session = task->owner;
    // 1. Update all its subscribers

    for (auto it = session->subscribers.begin(); it != session->subscribers.end(); it++)
    {
        auto subscriber = m_session_mng->fetch_session_by_id(*it);
        if (subscriber != m_session_mng->empty_session())
        {
            subscriber->subscribers.erase(session->session_id);
            subscriber->subscriptions.erase(session->session_id);
            out << "\nSubscriber: [" << subscriber->login_id << "]:[" << subscriber->session_id << "]:[" << subscriber->destiny_id << "]";
        }
    }

    for (auto it = session->subscriptions.begin(); it != session->subscriptions.end(); it++)
    {
        auto subscription = m_session_mng->fetch_session_by_id(*it);
        if (subscription != m_session_mng->empty_session())
        {
            subscription->subscribers.erase(session->session_id);
            subscription->subscriptions.erase(session->session_id);
            out << "\nSubscription: [" << subscription->login_id << "]:[" << subscription->session_id << "]:[" << subscription->destiny_id << "]";
        }

    }
    session->subscribers.clear();
    session->subscriptions.clear();
    out<< "\n--------------------";
    m_cb_on_task_end(task, out.str(), ec);
}
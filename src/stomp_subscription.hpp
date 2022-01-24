//#pragma once
#ifndef STOMP_SUBS_MNGR
#define STOMP_SUBS_MNGR

#include "jomt_util.hpp"
#include "stomp_session.hpp"
//#include "stomp_server.hpp"
#include <memory>
#include <string>

namespace jomt::stomp
{
    enum stomp_subscription_tasks : short {
        UNKOWN = 0,
        SUBSCRIBE,
        UNSUBSCRIBE
    };
    enum stomp_destination_type : short
    {
        DIRECT = 0,
        REGEX,
        DISABLE = 99
    };

    struct stomp_subscription_task
    {
        std::string trans_id{};
        stomp_subscription_tasks type_task{stomp_subscription_tasks::UNKOWN};
        stomp_destination_type dest_type{stomp_destination_type::DIRECT};
        std::string destination{};
        std::shared_ptr<stomp_session> owner{};
        bool send_call_back{true}; //To send the ACK message, FALSE = Not Send ACK, for cases of disconnects.
    };

    class stomp_subscription_manager
    {
        using smrt_ptr_tsk = std::shared_ptr<stomp_subscription_task>;
        using type_cb_task_end = std::function<void(std::shared_ptr<stomp_subscription_task>, 
                                                    std::string, 
                                                    stomp_errors)>;

        jomt::utils::task_queue<smrt_ptr_tsk> m_queue_eng;
        std::shared_ptr<stomp_session_manager> m_session_mng;
        type_cb_task_end m_cb_on_task_end;

        stomp_subscription_manager();
        void perform_subscription(std::shared_ptr<stomp_subscription_task> task);
        void perform_unsubscription(std::shared_ptr<stomp_subscription_task> task);

    public:         
        static std::shared_ptr<stomp_subscription_manager> instance();
        
        void link_call_back_functions(type_cb_task_end m_callback_unsub);
        void stop();
        void on_task(smrt_ptr_tsk task);

        void async_init_subscription(std::shared_ptr<stomp_session> session_owner, std::string destination,
                                     std::string trans_id, bool is_regex = false);

        void async_init_unsubscription(std::shared_ptr<stomp_session> session_owner, 
                                       std::string trans_id, bool send_ack_cb = true);
    }; 

}//namespace jomt::stomp
#endif

#pragma once
#ifndef JOMT_UTLS_HPP
#define JOMT_UTLS_HPP

#include<iostream>
#include <string>
#include <memory>
#include <thread>
#include <future>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <list>

namespace jomt::utils
{
    struct strings
    {
        static std::string trim(std::string  data);
        static std::string ltrim(std::string data);
        static std::string rtrim(std::string data);
    };


    template<class T>
    class task_queue
    {
        std::shared_ptr<std::thread> m_thread;
        std::mutex m_qlock;
        std::mutex m_tlock;
        std::condition_variable m_cv;
        std::atomic<bool> m_running;
        std::queue <T> m_queue;
        std::function<void(T)> m_callback_funcs;

        bool is_there_available_task()
        {
            std::unique_lock<std::mutex> lck_thr(m_qlock);
            return !m_queue.empty();
        }

        T dequeue_task()
        {
            std::unique_lock<std::mutex> lck_thr(m_qlock);
            T task = m_queue.front();
            m_queue.pop();
            return task;
        }

        void run() {
            std::unique_lock<std::mutex> lck_thr(m_tlock);
            while (m_running)
            {
                m_cv.wait(lck_thr);
                while (m_running && is_there_available_task())
                {
                    T task = dequeue_task();
                    m_callback_funcs(task);
                }
            };
        }
    
    public:
        task_queue(std::function<void(T)> cb_fnc) : m_callback_funcs(cb_fnc),
                                                    m_qlock{}, m_tlock{}, m_cv{},
                                                    m_running{false}, m_queue{}{}

        ~task_queue() { if (m_running) stop(); }

        void start()
        {
            if (!m_running)
            {
                m_running = true;
                m_thread = std::make_shared<std::thread>(&task_queue::run, this);
            }
        }

        void stop()
        {
            if (m_running)
            {
                m_running = false;
                m_cv.notify_all();
                if (m_thread->joinable())
                    m_thread->join();
            }
        }

        void add_task(T task)
        {
            std::unique_lock<std::mutex> lck(m_qlock);
            m_queue.push(task);
            m_cv.notify_all(); 
        }

        void add_task(std::list<T> tasks)
        {
            std::unique_lock<std::mutex> lck(m_qlock);
            for(auto it = tasks.begin(); it != tasks.end(); it++)
                m_queue.push(*it);
            m_cv.notify_all();
        }
    };

}// namespace

#endif
#include "ntwrk.h"
#include <iostream>
#include <thread>

using namespace jomt;

ntwrk_basic::~ntwrk_basic()
{
    if (_tr)
        delete _tr;
}

bool ntwrk_basic::isRunning() const { 
    return _bRunning; 
}

void ntwrk_basic::start()
{
    if (!_bRunning && _tr == nullptr)
    {
        std::cout << "\nntwrk_basic: starting thread\n";
        _bRunning = true;
        _tr = new std::thread(&ntwrk_basic::intRun, this);
        onStart();
    }    
}

void ntwrk_basic::stop()
{
    if (_bRunning)
    {
        _bRunning = false;
        onStop();
    }
}

void ntwrk_basic::join()
{
    if (_tr)
        _tr->join();
}

void ntwrk_basic::intRun()
{
    std::cout << "ntwrk_basic::run" << std::endl;
    run();
}

std::ostream &jomt::operator<<(std::ostream &out, const jomt::connection_info &cnx)
{
    std::string type{};
    // socket_type stp = ;
    switch (cnx.type)
    {
    case socket_type::SOCKET_IP:
        type = "TCP/IP";
        break;
    case socket_type::WEBSOCKET:
        type = "WEBSOCKET";
        break;
    default:
        type = "UNKWON";
        break;
    };

    return out << "ID[" << cnx.id << "]: Type: [" << type << "] "
               << "Address: [" << cnx.address << ":" << cnx.port << "]";
}
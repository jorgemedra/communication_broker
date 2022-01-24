/**
 * @file scktserver.cpp
 * 
 *
 * @author Jorge Omar Medra (https://github.com/jorgemedra)
 * @brief 
 * @version 0.1
 * @date 2021-09-26
 * 
 * @copyright Copyright (c) 2021
 * 
 * 
 * This sample is coded according with the ASIO specification to use Async Operations on sockets.
 * 
 * Protocol:
 *  [5Chars ]
 * 
 * 
$ ulimit -a
core file size          (blocks, -c) 0
data seg size           (kbytes, -d) unlimited
file size               (blocks, -f) unlimited
max locked memory       (kbytes, -l) unlimited
max memory size         (kbytes, -m) unlimited
open files                      (-n) 256
pipe size            (512 bytes, -p) 1
stack size              (kbytes, -s) 8192
cpu time               (seconds, -t) unlimited
max user processes              (-u) 2784
virtual memory          (kbytes, -v) unlimited

ulimit -n 2048

 */
#include "ntwrk.h"
#include <iostream>
#include <iomanip>

namespace asio  = boost::asio;
namespace ip    = boost::asio::ip;

using namespace jomt;

#pragma region //ScktServer

ScktServer::ScktServer(int bindPort) : _bPort{bindPort},
                                       _ntwrk_context{},
                                       _acceptor(_ntwrk_context, ip::tcp::endpoint(ip::tcp::v4(), bindPort)),
                                       _pool_cnxs(CNX_TCP_MAX)
{
    auto hint = _sktsAvbl.begin();
    for (short i = 0; i < CNX_TCP_MAX; i++)
    {
        _pool_cnxs[i] = std::shared_ptr<TcpCnx>(new TcpCnx(_ntwrk_context));
        hint = _sktsAvbl.insert(hint, i);
    }
}

short ScktServer::queryFirstAvailable()
{
    std::unique_lock<std::mutex> lck(_mtx_pool);

    if (_sktsAvbl.empty())
        return -1;
    auto bgn = _sktsAvbl.begin();
    short idx = *bgn;    
    return idx;
}

void ScktServer::linkSocket(ip::port_type port, short idx_slot)
{
    std::unique_lock<std::mutex> lck(_mtx_pool);

    auto it = _sktsAvbl.find(idx_slot);    
    _sktsAvbl.erase(it);

    _sktsUnvbl.insert(std::pair<ip::port_type, short>(port, idx_slot));
    std::cout << "\nLinked Port to Index:  [" << port << "]<->[" << idx_slot << "]\n";    
    std::cout << "\n\tConnection Available:[" << _sktsAvbl.size() << "]" << std::endl;
    std::cout << "\n\tConnection Count:[" << _sktsUnvbl.size() << "]" << std::endl;
}

        void
        ScktServer::unlinkSocket(ip::port_type port, short idx_slot)
    {
        std::unique_lock<std::mutex> lck(_mtx_pool);

        auto it = _sktsUnvbl.find(port);
        if (it != _sktsUnvbl.end())
        {
            std::cout << "\nUnLinked Port to Index:  [" << port << "]<->[" << idx_slot << "]\n";
            _sktsAvbl.insert(idx_slot);
            _sktsUnvbl.erase(it);
        }
}

void ScktServer::onStart() 
{
    std::cout << "[onStart] Runnig ScktServer for IPV4 and on port [" << _bPort << "]\n";
}

void ScktServer::run()
{
    std::cout << " Runnig ScktServer for IPV4 and on port [" << _bPort << "]\n";
    
    boost::system::error_code error;

    try
    {
        std::cout << "\nMax Listent Connections: " << _acceptor.max_listen_connections << "\n";
        _acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
        _acceptor.listen();

        begin_accept();

        _ntwrk_context.run(error); //To run in async mode.

    }
    catch (std::exception &e)
    {
        std::cout << "EXCEPTION[run]: " << e.what() << std::endl;
        std::cout << "\tValue:" << error.value() << '\n';
        std::cout << "\tCategory:" << error.category().name() << '\n';
        std::cout << "\tMessage:" << error.message() << '\n';
    }
    std::cout << "....\n";
}

void ScktServer::onStop(){

    std::cout << "[onStop] stop\n";
    _acceptor.cancel();
    _ntwrk_context.stop();
    _acceptor.close();
    std::cout << "[onStop] Stopped\n";
}

void ScktServer::begin_accept()
{
    try
    {
        std::cout << "wait for a connection.\n";
        std::shared_ptr<TcpCnx> cnx(new TcpCnx(_ntwrk_context));
        cnx-> reset_buffer(proto_stage::header);
        _acceptor.async_accept(cnx->socket(),
                               boost::bind(&ScktServer::accepted,
                                           this,
                                           cnx,
                                           asio::placeholders::error));
    }
    catch (std::exception &e)
    {
        std::cout << "EXCEPTION [begin_accept]: " << e.what() << std::endl;
    }
}

void ScktServer::accepted(std::shared_ptr<TcpCnx> cnx, const boost::system::error_code &error)
{
    if (!error)
    {
        short idx_slot = queryFirstAvailable();
        if (idx_slot >= 0)
        {
            _pool_cnxs[idx_slot].swap(cnx);
            ip::tcp::socket &skct = _pool_cnxs[idx_slot]->socket();
            std::cout << "Accepted Connection [" << skct.local_endpoint().port()
                      << "]:[" << skct.remote_endpoint().address() << ":" << skct.remote_endpoint().port() << "] "
                      << "on slot[" << idx_slot << "]\n";

            _pool_cnxs[idx_slot]->slot(idx_slot);
            linkSocket(skct.remote_endpoint().port(), idx_slot);
            start_rx(_pool_cnxs[idx_slot]);
        }
        else
        {
            std::cout << "[accepted cnx] Rejected by reach the limit of connections available.";
            cnx->socket().close();
        }
    }
    else
    {
        std::cout << "Accepted Connection Error: " << error << "\n";

        std::cout << "\tValue:" << error.value() << '\n';
        std::cout << "\tCategory:" << error.category().name() << '\n';
        std::cout << "\tMessage:" << error.message() << '\n';
    }

    if (isRunning())
        begin_accept();
}

void ScktServer::start_rx(std::shared_ptr<TcpCnx> cnx)
{
    std::cout << "------------------------------------\n";
    std::cout << "[" << cnx->slot() << "] start_rx.\n";


    std::vector<char> &buff = cnx->rx_buffer();    
    
    asio::async_read(cnx->socket(),
                     boost::asio::buffer(buff),                     
                     [this, cnx](const boost::system::error_code &error, size_t bytes_transferred)
                     {
                        auto port = cnx->socket().remote_endpoint().port();
                        auto _slot = cnx->slot();

                        if (!error)
                        {
                            if (bytes_transferred > 0)
                                proccess_data(cnx, bytes_transferred);
                            else
                            {
                                std::cout << "[" << cnx->slot() << "]rx_handler Error: \n"
                                          << "\tMessage: It has received a message of 0 byres, it will be closed.\n";
                                disconnect_cnx(port, _slot);
                            }
                         }
                         else
                         {
                             std::cout << "[" << cnx->slot() << "]rx_handler Error: \n"
                                       << "\tValue:" << error.value() << '\n'
                                       << "\tCategory:" << error.category().name() << '\n'
                                       << "\tMessage:" << error.message() << '\n';

                             disconnect_cnx(port, _slot);
                         }
                     });
}

void ScktServer::start_tx(std::shared_ptr<TcpCnx> cnx, std::string data)
{
    std::cout << "\n[" << cnx->slot() << "]start_tx:\n";
    std::cout << "[" << cnx->slot() << "] TX Message: [" << data << "]" << std::endl;

    asio::async_write(
        cnx->socket(),
        boost::asio::buffer(data),
        asio::transfer_all(),
        boost::bind(&ScktServer::end_tx, this, cnx, asio::placeholders::error, asio::placeholders::bytes_transferred)
    );

    /*
    asio::async_write(
        cnx->socket(),
        boost::asio::buffer(data),
        asio::transfer_all(),
        [this, cnx](const boost::system::error_code &ec, size_t bytes_transferred)
        {
            if (ec)
            {
                std::cout << "[" << cnx->slot() << "]tx_handler Error: \n"
                          << "\tValue:" << ec.value() << '\n'
                          << "\tCategory:" << ec.category().name() << '\n'
                          << "\tMessage:" << ec.message() << '\n';

                auto port = cnx->socket().remote_endpoint().port();
                disconnect_cnx(port, cnx->slot());
            }
            else
            {
                std::cout << "[" << cnx->slot() << "]tx_handler lambda Finish Tx [" << bytes_transferred << "] bytes.\n" << std::endl;
                std::cout << "endl TX." << std::endl;
            }
        });
    */
}

void ScktServer::end_tx(std::shared_ptr<TcpCnx> cnx, const boost::system::error_code &ec, size_t bytes_transferred)
{
    if(ec)
    {
        std::cout << "[" << cnx->slot() << "]tx_handler Error: \n"
                  << "\tValue:" << ec.value() << '\n'
                  << "\tCategory:" << ec.category().name() << '\n'
                  << "\tMessage:" << ec.message() << '\n';

        auto port = cnx->socket().remote_endpoint().port();
        disconnect_cnx(port, cnx->slot());
    }
    else
    {
        std::cout << "[" << cnx->slot() << "]tx_handler lambda Finish Tx [" << bytes_transferred << "] bytes.\n"
                  << std::endl;
    }
}

void ScktServer::disconnect_cnx(ip::port_type port, short _slot)
{
    std::cout << "\ndisconnect_cnx(" << port  << "," << _slot << ")";
    if (_slot < _pool_cnxs.size() && _pool_cnxs[_slot]->socket().is_open())
    {
        _pool_cnxs[_slot]->socket().close();
        unlinkSocket(port, _slot);
    }
    
}

void ScktServer::proccess_data(std::shared_ptr<TcpCnx> cnx, size_t bytes_transferred)
{
    auto buff = cnx->rx_buffer();
    std::cout << "\n[" << cnx->slot() << "]rx_handler; stage[" << (cnx->stage() == 0? "HDR":"PYL") << "]:\n";
    std::string message(buff.begin(), buff.end());
    std::cout << "[" << cnx->slot() << "] RX Bytes Transfered: [" << bytes_transferred << "]\n";
    std::cout << "[" << cnx->slot() << "] RX Data: [" << message << "]\n";

    if (cnx->stage() == proto_stage::header)
    {
        size_t py_size = std::atoi(message.c_str());
        std::cout << "\n PAYLOAD = " << py_size << std::endl;
        cnx->reset_buffer(proto_stage::payload, py_size);
    }
    else
    {   
        std::stringstream out;
        std::string data("ECHO:");
        data += message;
        out << std::setw(6) << data.size() << data;
        start_tx(cnx, out.str());
        out << "Reseting the boofer to Header..." << std::endl;
        cnx->reset_buffer(proto_stage::header);
        out << "Buffer reseted" << std::endl;
    }
    std::cout << "Starting to read" << std::endl;
    start_rx(cnx);
}



#pragma endregion //ScktServer
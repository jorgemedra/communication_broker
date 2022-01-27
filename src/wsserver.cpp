#include <iostream>
#include <memory>
#include <string_view>
#include <boost/beast.hpp>
//#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>

#include <thread>
#include <mutex>
#include "ws.hpp"

namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
namespace ip = boost::asio::ip;
namespace beast = boost::beast;
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace boost::beast;
using namespace boost::beast::websocket;
using namespace jomt;

wsserver::wsserver(std::shared_ptr<jomt::basic_server> server,
                   int bind_port,
                   int max_cnxs,
                   ssl::context::method mtd) : m_mainsrv{server},
                                               m_port{bind_port},
                                               m_ioc{},
                                               m_ssl_ioc(mtd),
                                               m_acceptor(m_ioc, ip::tcp::endpoint(ip::tcp::v4(), m_port)),
                                               m_cnxs{}, m_counter{0},
                                               m_stck_ids{},
                                               m_lockcnx{},
                                               m_is_ssl{false}
{
    initIdQueue(max_cnxs);
}

void wsserver::initIdQueue(int max_cnxs)
{
    for (int id = max_cnxs; id > 0; id--)
        m_stck_ids.push(id);
}

void wsserver::set_ssl_options(std::string crt_path, std::string key_path, std::string pem_path) 
{
    //std::cout << "set_ssl_options\n";
    //std::cout << "set_ssl_options::set_options\n";
    m_ssl_ioc.set_options(  ssl::context::default_workarounds |
                            ssl::context::no_tlsv1_2 |
                            ssl::context::single_dh_use
                        );
    //std::cout << "set_ssl_options::set_password_callback\n";
    m_ssl_ioc.set_password_callback([this](std::size_t max_lenght, ssl::context::password_purpose porpouse) -> std::string
                                    { return "dummypem"; });

    try
    {
        m_ssl_ioc.use_certificate_chain_file(crt_path);
        m_ssl_ioc.use_private_key_file(key_path, ssl::context::pem);
        m_ssl_ioc.use_tmp_dh_file(pem_path);
    }
    catch (const boost::wrapexcept<boost::system::system_error> &ec)
    {
        std::cout << "ERROR::set_ssl_options:: " << ec.what() << "\n";
        m_mainsrv->on_server_stop(info(), ec.code());
        exit(1);
    }

    m_is_ssl= true;
}

std::pair<bool, std::shared_ptr<wscnx>> wsserver::fetch_cnx(int id)
{
    std::unique_lock<std::mutex> lockstck(m_lockcnx);

    auto it = m_cnxs.find(id);
    if(it == m_cnxs.end())
        return {false, {}};
    return {true,it->second};
}

jomt::server_info wsserver::info()
{
    return {m_port, jomt::WEBSOCKET};
}

void wsserver::onStart()
{
    //std::cout << "[onStart] Runnig WSServer for IPV4 and on port [" << m_port << "]\n";
    m_mainsrv->on_server_start(info());
}

void wsserver::onStop()
{
    boost::system::error_code ec;
    onStop(ec);
}

void wsserver::onStop(const boost::system::error_code &ec)
{
    m_acceptor.cancel();
    m_ioc.stop();
    m_acceptor.close();
    m_mainsrv->on_server_stop(info(), ec);
}

void wsserver::run()
{
    //std::cout << "[run] Running...\n";
    beast::error_code ec;

    
    m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    m_acceptor.listen(net::socket_base::max_listen_connections, ec);



    if (ec)
    {
        //std::cout << "[run] Failed at listening on acceptor.\n";
        onStop(ec);
        return;
    }

    //if(!m_ssl)
    wait_for_connections();
    //else
    //    wait_for_connections_ssl();
    m_ioc.run();
    //std::cout << "[run] Running end\n";
}

//This acceptor uses the first thread.
void wsserver::wait_for_connections()
{
    //std::cout << "[wait_for_connections] Waitting for a new connection.\n";
    m_acceptor.async_accept(m_ioc,
                            [self{get()}](const boost::system::error_code &ec, ip::tcp::socket socket)
                            {
                                if (ec)
                                {
                                    std::cout << "[on_accept]: " << ec.message() << "\n";
                                    self->onStop(ec);
                                }
                                else
                                {
                                    auto [id, cnxi] = self->regiter_wscnx(std::move(socket));
                                    self->m_mainsrv->on_new_connection(self->info(), id, cnxi);
                                }

                                self->wait_for_connections();
                            });
}

void wsserver::cnx_closed(int id, const boost::system::error_code &ec)
{
    //std::cout << "[cnx_closed] The connection [" << id << "].\n";
    if (unregiter_wscnx(id))
    {
        m_mainsrv->on_connection_end(info(), id, ec);
    }
    //std::cout << "[cnx_closed] END.\n";
}

std::pair<int, connection_info> wsserver::regiter_wscnx(ip::tcp::socket &&socket)
{
    std::unique_lock<std::mutex> lockstck(m_lockcnx);

    int id = m_stck_ids.top();
    m_stck_ids.pop();

    std::shared_ptr<wscnx> cnx = m_is_ssl ? std::make_shared<wscnx>( id, std::move(socket), std::ref(m_ssl_ioc), shared_from_this() ) : 
                                            std::make_shared<wscnx>( id, std::move(socket), shared_from_this() );

    std::pair<int, std::shared_ptr<wscnx>> data = std::pair<int, std::shared_ptr<wscnx>>(id, cnx);
    m_cnxs.insert(data);
    return {id, cnx->cnx_info()};
}

bool wsserver::unregiter_wscnx(int id)
{
    std::unique_lock<std::mutex> lockstck(m_lockcnx);

    auto it = m_cnxs.find(id);
    if (it != m_cnxs.end())
    {   
        m_cnxs.erase(it);
        m_stck_ids.push(id);
        return true;
    }
    return false;
}

void wsserver::write(int id, std::string_view data, bool close_it)
{
    auto it = m_cnxs.find(id);
    if(it != m_cnxs.end())
        it->second->write(data, close_it);
}

void wsserver::on_data_rx(int id, std::string_view data, connection_info cnxi)
{
    m_mainsrv->on_data_rx(info(), id, data, cnxi);
}
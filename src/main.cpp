

#include <iostream>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <list>
#include "stomp_server.hpp"

//#include "ntwrk.h"
//#include "ws.hpp"
//#include <boost/array.hpp>
//#include <boost/asio.hpp>

//using boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace ip = boost::asio::ip;
//namespace resolver = boost::asio::ip::tcp::resolver;

// void print_utc_time()
// {
//     // https: //api.github.com/users/jorgemedra/orgs

//     jomt::HTTPClient http_clt;
//     //std::string host{"worldclockapi.com"};
//     std::string host{"api.github.com"};    
//     //std::string port{"80"};
//     std::string port{"443"};
//     //std::string target{"/users/jorgemedra/orgs"};
//     std::string target{"/"};
//     std::string params{""};

//     std::stringstream log;
//     std::shared_ptr<jomt::response> resp = http_clt.do_get(host, port, target, params, log);

//     /*
//     std::cout << "<****************** LOG ******************>\n"
//               << log.str() << "\n" 
//               << ">****************** LOG ******************<" << std::endl; 
//     */
   
//     std::cout << "---------------------------------------------------------------\n";
//     std::cout << "STATUS CODE = " << resp->status << "\n"
//               << "HEADERS: \n";
//     for (auto it_h = resp->header.begin(); it_h != resp->header.end(); it_h++)
//         std::cout << "\t[" << it_h->first << "]:[" << it_h->second << "]\n";

//     std::string_view body(resp->body.data());
//     std::cout << "BODY: \n" << body << "\n";
// }



int main(int argc, char* argv[])
{
    //-DCNX_TCP_MAX = 10 - CNX_ADM = 5 - DFD_SET_SIZE = 15 std::cout << argv[1];
    static_assert(FD_SET_SIZE == (CNX_TCP_MAX + CNX_ADM), "CNX_TCP_MAX + CNX_ADM must be EQUAL than FD_SET_SIZE, fix it on Complile cvalidation.");

    std::cout << "------------------------------\n"
        << "./sample <SktSrvPort> <WSSrv>\n"
        << "\nMax Socket Connections : " << FD_SET_SIZE
        << "\nMax TCP Connections DCNX_TCP_MAX: " << CNX_TCP_MAX << std::endl;
    
    //print_utc_time(); 
    
    int tcpPort = std::stoi(argv[1]);
    int wsPort = std::stoi(argv[2]);

    /*
    jomt::ScktServer sckSrv(scktPort);
    sckSrv.start();
    */

    std::string crt{"./cert/dummy.crt"};
    std::string key{"./cert/dummy.key"};
    std::string pem{"./cert/dhdummy.pem"};

    //std::shared_ptr<jomt::stomp::stomp_sever> stompsrv(new jomt::stomp::stomp_sever(scktPort));
    auto stompsrv = jomt::stomp::stomp_sever::create(tcpPort, wsPort);
    // Set all the secret key into the stomp_server
    stompsrv->set_secret_keys("my_app_secret_key", "my_agent_secret_key",
                            "my_super_agent_secret_key", "my_service_secret_key",
                            "my_admin_secret_key");

    // This method activate the SSL Mode on the stomp server.
    stompsrv->set_ssl_options_on_ws(crt, key, pem);
    stompsrv->start();

    std::cout
        << "Press -Enter- to exit\n"
        << std::endl;

    std::string line{};
    // std::cin >> line;
    do
    {
        std::cin >> line;
    }while(line.compare("q") != 0);

    std::cout << "Stop all" << std::endl;;

    //sckSrv.stop();
    stompsrv->stop();

    std::cout << "-- END --" << std::endl;
    return 0;
}


# C++ Communication Broker

# TODO List

- [x] Connect Diagram.
- [x] Disconnect Diagram.
- [x] Subscribe Diagram.
- [x] Unsubscribe Diagram.
- [x] Send Diagram.
- [x] BUG: It may be a problem on Subscriptions and Unsubscriptions, it necessary to check the whole essenary to add and remove the subscriptons/subscrbes form ALL the sessions linked to an LoginId/Agent.
- [x] BUG: Subscription has no efect on Client on Service Profile.
- [x] BUG: Unsubscribe Exception at disconnect: "Assertion failed: (id_ != T::id), function try_lock, file /usr/local/include/boost/beast/websocket/detail/soft_mutex.hpp, line 89"
- [x] BUG: test with an empty login-id.
- [x] Upate Close connection, on Disconnect, to close connection.
- [x] BUG: Remove end lines of body
- [x] Implement a global trim function 
- [x] Add session information on Error Message
- [ ] On Connect, implement KeepAlive to all connections on API.
- [ ] Add a thread to check all the connections which have been reached the time out.
- [x] get the destination id to set into the STOMP Client API.
- [x] Check error Message.
- [x] Desgine the scheme of Publisher Suscriber and the ids to be used with regex on destiny.
- [x] Subscribe
- [x] UnSubscribe
- [x] Disconnect / Receipt
- [x] Message
- [x] Send / ACK
- [x] ACK FOR SEND
- [x] Finish JS API, Left proccess the remain messages.
- [x] Implements the TCP Server.
- [ ] BUG: SSL on TCP Server doesn't work.
- [ ] Run as a Daemon or Console mode.
- [ ] Add logs with Boost Log api... or mine (as more control as better).


# Overview

This sample shows how to work with the libs of JSON and LOGS of BOOST. The sample covers the basic apects of Boost Log Lib and how to read a JSON file and, more important, how to compile and link the sample with these libs.

> **Important** Consider that the file boostlog.h contains all the macros to simplify the use of logs methdos on trivial mode:
 ```cpp
#define LT_TRACE    BOOST_LOG_TRIVIAL(trace)
#define LT_DEBUG    BOOST_LOG_TRIVIAL(debug)
#define LT_INFO     BOOST_LOG_TRIVIAL(info)
#define LT_WARN     BOOST_LOG_TRIVIAL(warning)
#define LT_ERROR    BOOST_LOG_TRIVIAL(error)
#define LT_FATAL    BOOST_LOG_TRIVIAL(fatal)
```

## Requierements

    1. C++17 Standar.
    2. Boost V1.77. All libraries are instaled into the `/user/local/lib` and headers in `/user/local/include`.
    3. Visual Studio Code 1.55 or above.
    4. For Linux: g++ (GCC) 11.1.0
    5. For OSX (11.5.2): Apple clang version 11.0.0 (clang-1100.0.33.17)

## Build

> Due to some problems at running time, in my case *"Segmentation Fault: 11"* at reading the files, its important to appoint that the library must be compiles with only Headers, avoiding linkig the *.so/.a* lib, on OSX environments. 

```cpp
json::value parseJson(std::istream &is, json::error_code &ec)
{
    ...

    json::value vval;

    try{
        vval = p.release();
    }
    catch(const std::exception &e)
    {
        LT_FATAL << "Error in parsing JSON file: " << e.what();
    }
    LT_TRACE << "Parsing JSON ended.";

    return vval;    // At this point the Parser's Destructor raises an exception.
}
```

1. **For JSON**, it needs to add at the beging of the *main.cpp** file the include `#include <boost/json/src.hpp>`, and remove from link step the library **boost_json**.
2.  **For Logs**, it needs to add the Macro `-DBOOST_LOG_DYN_LINK` on **link step**, to link the library as a dynamic liberary.
3.  Add the next libraries to link step: `-pthread -lboost_log -lboost_thread -lboost_log_setup -lstdc++`

## Make

To get help to use the Makefile:
```shell
$make help
.............................................
Build: make
Build with debug info: make DBGFLG=-g
Clean compiled files: make clean
```

To compile and build all the project:
```shell
$make
```

To compile and activate debug info:

```shell
$make DBGFLG=-g
```

To clean all the compiled files:

```shell
$make clean
```

## Testing

To run the sample change path to `{$workspace}/bin` and execute:

``` cmd
./sample <path to boost_jsonlogs proyect>/logs/sample_%N.log
```

## References

1. Getting starter [here](https://www.boost.org/doc/libs/1_77_0/more/getting_started/unix-variants.html).
2. JSON Lib with Header-Only [here](https://www.boost.org/doc/libs/1_75_0/libs/json/doc/html/json/overview.html#json.overview.requirements).
3. LOG Lib as Dynamic lib [here](https://www.boost.org/doc/libs/1_77_0/libs/log/doc/html/index.html).


## Create a Certificate

> From: https://stackoverflow.com/questions/6452756/exception-running-boost-asio-ssl-example


My Dummy data to create a cert are:

+ Key File: dummy.key
+ Cert File: dummy.csr
+ Self Signed Cert File: dummy.crt
+ PEM File: dhdummy.pem
+ PEM Pass Phrase: dummypem
+ Country: MX
+ State (Full Name): Mexico
+ Locality Name: CDMX
+ Organization Name: JOMT
+ Organization Unit Name: JOMT
+ Common Name: MacBook-Pro-de-Jorge.local
+ Email Address: anyaddress@mail.com

openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -sha256 -days 365 -subj '/CN=localhost'

``` cmd
openssl req -x509 -newkey rsa:4096 -keyout dummy_key.pem -out dummy.pem -sha256 -days 365
openssl x509 -outform der -in dummy.pem -out dummy.crt
```


1. Generate a private key
```cmd
openssl genrsa -des3 -out dummy.key 2048
```

2. Generate Certificate signing request
```cmd
openssl req -new -key dummy.key -out dummy.csr
```

3. Sign certificate with private key
```cmd
openssl x509 -req -days 3650 -in dummy.csr -signkey dummy.key -out dummy.crt
openssl x509 -signkey dummy.key -in dummy.csr -req -days 365 -out dummy.crt
```

4. Generate dhparam file
```cmd
openssl dhparam -out dhdummy.pem 1024
```

```cpp
m_ssl_ipp.use_certificate_chain_file("dummy.crt"); 
m_ssl_ipp.use_private_key_file("dummy.key", boost::asio::ssl::context::pem);
m_ssl_ioc.use_tmp_dh_file("dhdummy.pem");
```


# Sample 

```cmd
python -m SimpleHTTPServer 7771
```
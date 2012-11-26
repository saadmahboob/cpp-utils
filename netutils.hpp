#pragma once

#include <functional>
#include <cstring>
#include <list>

#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

namespace util
{
    namespace net
    {
        class socket_exception : public std::exception {
        public:
            inline socket_exception(const std::string &_message)
                : mMessage(_message) {}
            inline const char *what() const throw() { return mMessage.c_str(); }
        private:
            std::string mMessage;
        };
        
        typedef int socket;
        typedef hostent host_entity;
        
        const socket invalid_socket = (socket)(-1);
        
        #if defined(_WIN32) || defined(_WIN64)
            typedef SOCKADDR_IN socket_address;
            typedef ADDRINFOA address_info;
        #else
            typedef sockaddr_in socket_address;
            typedef addrinfo address_info;
        #endif
        
        template<class StorageType = std::list<address_info>>
        StorageType resolve(const std::string &_hostname)
        {
            StorageType result;
            address_info *hosts;
            
            address_info hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            auto returnValue = getaddrinfo(_hostname.c_str(), nullptr, &hints, &hosts);

            if(returnValue != 0)
                throw socket_exception("getaddrinfo() call failed whilst resolving '" + _hostname + "'");

            for(address_info *addr = hosts; addr != nullptr; addr = addr->ai_next)
            {
                result.push_back(*addr);
            }
            
            freeaddrinfo(hosts);

            return result;
        }
        
        address_info resolve_to_any(const std::string &_hostname)
        {
            auto result = resolve(_hostname);
            if(result.empty())
                throw socket_exception("unable to resolve hostname '" + _hostname + "'");
            return result.front();
        }
        
        class service
        {
        public:
            inline void run() {
            }
        private:
        };
        
        class base_socket
        {
        public:
            inline base_socket(service &_service) : mService(_service) {}
        protected:
            service &mService;
        };
        
        class client : public base_socket
        {
        public:
            inline client(service &_service) : base_socket(_service), mSocket(invalid_socket) {}
            inline client(client &&_move)
                : base_socket(_move.mService), mSocket(_move.mSocket), mIP(_move.mIP) {
                _move.mSocket = invalid_socket;
            }
            inline void connect_async(const std::string &_target, int _port) {
            }
            inline bool connect(const std::string &_target, int _port) {
                if(mSocket != invalid_socket)
                    throw socket_exception("socket already connected");             
                mSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if(mSocket == invalid_socket)
                    throw socket_exception("failed to create socket");
                address_info resolved = resolve_to_any(_target);
                socket_address addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(_port);
                addr.sin_addr = ((socket_address*)resolved.ai_addr)->sin_addr;
                auto returnValue = ::connect(mSocket, (sockaddr*)&addr, sizeof(addr));
                if(returnValue != 0)
                    return false;
                mIP = _target;
                return true;
            }
            inline void write_async(const std::string &_data, std::function<void(client&,int)> _callback) {
                write_async(_data.c_str(), _data.length(), _callback);
            }
            inline void write_async(const char *_data, int _count, std::function<void(client&,int)> _callback) {
            }
            inline int write(const std::string &_data) {
                return write(_data.c_str(), _data.length());
            }
            inline int write(const char *_data, int _count) {
                return ::send(mSocket, _data, _count, 0);
            }
            inline void read_async(char *_data, int _size, std::function<void(client&,int)> _callback) {
            }
            inline int read(char *_data, int _size) {
                return ::recv(mSocket, _data, _size, 0);
            }
            inline void close() {
                ::close(mSocket);
                mSocket = invalid_socket;
            }
            inline const std::string &ip() const { return mIP; }
        private:
            socket mSocket;
            std::string mIP;
        };
        
        class server : public base_socket
        {
        public:
            inline server(service &_service, int _port) : base_socket(_service), mPort(_port), mSocket(invalid_socket) {
            }
            inline server(server &&_move) : base_socket(_move.mService) {
            }
            inline void accept_async(std::function<void(server&)> _callback) {
            }
            inline client accept() {
                return client(mService); // STUB
            }
            inline int port() const { return mPort; }
        private:
            int mPort;
            socket mSocket;
        };
    };
};


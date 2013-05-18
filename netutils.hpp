#pragma once

#include <functional>
#include <cstring>
#include <stdio.h>
#include <string>
#include <memory>
#include <list>

#include <stringutils.hpp>

#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <sys/ioctl.h>
    #include <sys/poll.h>
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
        
        inline void __throw_error_with_number(const std::string &message) {
            #if defined(_WIN32) || defined(_WIN64)
                int error = WSAGetLastError();
            #else
                int error = errno;
            #endif
            
            throw socket_exception(message + ": error code (" + util::string::from(error) + ")");
        }
        
        typedef int socket;
        typedef hostent host_entity;
        
        const socket invalid_socket = (socket)(-1);
        
        #if defined(_WIN32) || defined(_WIN64)
            typedef SOCKADDR_IN socket_address;
            typedef ADDRINFOA address_info;
            typedef WSAPOLLFD poll_descriptor;
        #else
            typedef sockaddr_in socket_address;
            typedef addrinfo address_info;
            typedef pollfd poll_descriptor;
        #endif
        
        template<class storage_type = std::list<address_info>>
        storage_type resolve(const std::string &_hostname)
        {
            storage_type result;
            address_info *hosts;
            
            address_info hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            auto returnValue = getaddrinfo(_hostname.c_str(), nullptr, &hints, &hosts);

            if(returnValue != 0)
                __throw_error_with_number("getaddrinfo() call failed whilst resolving '" + _hostname + "'");

            for(address_info *addr = hosts; addr != nullptr; addr = addr->ai_next)
            {
                result.push_back(*addr);
            }
            
            freeaddrinfo(hosts);

            return result;
        }
        
        inline address_info resolve_to_any(const std::string &_hostname)
        {
            auto result = resolve(_hostname);
            if(result.empty())
                throw socket_exception("unable to resolve hostname '" + _hostname + "'");
            return result.front();
        }
        
        inline void set_nonblocking(socket _socket, int _argument)
        {
            #if defined(_WIN32) || defined(_WIN64)
                ioctlsocket(_socket, FIONBIO, &_argument);
            #else
                ioctl(_socket, FIONBIO, &_argument);
            #endif
        }
        
        inline void make_nonblocking(socket _socket)
        {
            set_nonblocking(_socket, 1);
        }
        
        inline void make_blocking(socket _socket)
        {
            set_nonblocking(_socket, 0);
        }
        
        inline void shutdown_socket(socket _socket)
        {
            #if defined(_WIN32) || defined(_WIN64)
                ::shutdown(_socket, SD_BOTH);
            #else
                ::shutdown(_socket, SHUT_RDWR);
            #endif
        }
        
        class service;
        
        class base_socket
        {
        public:
            inline base_socket(service &_service) : mService(_service) {}
        protected:
            service &mService;
        };
        
        class socket_event_handler
        {
        public:
            typedef std::function<void()> error_fn;
            typedef std::function<void()> read_fn;
            typedef std::function<void()> write_fn;
        public:
            inline socket_event_handler()
                : mSocket(invalid_socket) {}
            inline socket_event_handler(socket _socket, read_fn _read, write_fn _write, error_fn _error)
                : mRead(_read), mSocket(_socket), mWrite(_write), mError(_error) {}
            inline socket_event_handler(const socket_event_handler &_copy)
                : mRead(_copy.mRead), mSocket(_copy.mSocket), mWrite(_copy.mWrite), mError(_copy.mError) {}
            inline bool wants_to_read() const { return !!mRead; }
            inline bool wants_to_write() const { return !!mWrite; }
            inline void on_read() { mRead(); }
            inline void on_write() { mWrite(); }
            inline void on_error() { mError(); }
            inline socket handle() const { return mSocket; }
        private:
            read_fn mRead;
            socket mSocket;
            write_fn mWrite;
            error_fn mError;
        };
        
        class service
        {
        public:
            inline void add_handler(const socket_event_handler &_handler) {
                mHandlers.push_back(_handler);
            }
            inline bool do_poll(int _timeout = 100) {
                if(mHandlers.size() < 1) return false;
                
                std::vector<socket_event_handler> handlers;
                handlers.insert(handlers.begin(), std::begin(mHandlers), std::end(mHandlers));
                mHandlers.clear();
                
                std::unique_ptr<poll_descriptor> descriptors(new poll_descriptor[handlers.size()]);
                std::unique_ptr<bool> preserve_flags(new bool[handlers.size()]);
                
                for(unsigned int i = 0; i < handlers.size(); i++)
                {
                    poll_descriptor &descriptor = descriptors.get()[i];
                    descriptor.fd = handlers[i].handle();
                    descriptor.events = 0;
                    if(handlers[i].wants_to_read())
                        descriptor.events |= POLLIN;
                    if(handlers[i].wants_to_write())
                        descriptor.events |= POLLOUT;
                    preserve_flags.get()[i] = true;
                }
                
                #if defined(_WIN32) || defined(_WIN64)
                    auto result = ::WSAPoll(descriptors.get(), handlers.size(), _timeout);
                #else
                    auto result = ::poll(descriptors.get(), handlers.size(), _timeout);
                #endif
                
                if(result < 0)
                    __throw_error_with_number("error performing socket poll");
                
                for(unsigned int i = 0; i < handlers.size() && result > 0; i++)
                {
                    poll_descriptor &descriptor = descriptors.get()[i];
                    
                    if(!(descriptor.revents & POLLERR))
                    {
                        if(descriptor.revents & POLLIN || descriptor.revents & POLLOUT)
                        {
                            result --;
                            preserve_flags.get()[i] = false;
                            if(descriptor.revents & POLLIN)
                                handlers[i].on_read();
                            if(descriptor.revents & POLLOUT)
                                handlers[i].on_write();
                        }
                    }
                    else
                    {
                        result --;
                        preserve_flags.get()[i] = false;
                        handlers[i].on_error();
                    }
                }
                
                for(unsigned int i = 0; i < handlers.size(); i++)
                {
                    if(preserve_flags.get()[i])
                    {
                        add_handler(handlers[i]);
                    }
                }
                
                return true;
            }
            inline void run(bool _abort_on_empty=false) {
                while(do_poll(1000) || !_abort_on_empty);
            }
        private:
            std::list<socket_event_handler> mHandlers;
        };
        
        class client : public base_socket
        {
        public:
            inline client(service &_service, socket _socket) : base_socket(_service), mSocket(_socket) {
                // TODO: derive IP string? o:
                mIP = "some-ip-here";
            }
            inline client(service &_service) : base_socket(_service), mSocket(invalid_socket) {}
            inline client(client &&_move)
                : base_socket(_move.mService), mSocket(_move.mSocket), mIP(_move.mIP) {
                _move.mSocket = invalid_socket;
            }
            inline ~client() {
                close();
            }
            inline void connect_async(const std::string &_target, int _port, std::function<void(client&,bool)> _callback) {
                make_nonblocking(mSocket);
                invokeConnect(_target, _port);
                mService.add_handler(socket_event_handler(mSocket, nullptr,
                    [=](){
                        _callback(*this, true);
                    },
                    [=](){
                        _callback(*this, false);
                    }
                ));
            }
            inline bool connect(const std::string &_target, int _port) {
                make_blocking(mSocket);
                auto returnValue = invokeConnect(_target, _port);
                if(returnValue != 0)
                    return false;
                mIP = _target;
                return true;
            }
            inline void write_async(const std::string &_data, std::function<void(client&,int)> _callback) {
                write_async(_data.c_str(), _data.length(), _callback);
            }
            inline void write_async(const char *_data, int _count, std::function<void(client&,int)> _callback) {
                make_nonblocking(mSocket);
                mService.add_handler(socket_event_handler(mSocket, nullptr,
                    [=](){
                        auto result = write(_data, _count);
                        _callback(*this, result);
                    },
                    [=](){
                        _callback(*this, 0);
                    }
                ));
            }
            inline int write(const std::string &_data) {
                return write(_data.c_str(), _data.length());
            }
            inline int write(const char *_data, int _count) {
                make_blocking(mSocket);
                return ::send(mSocket, _data, _count, 0);
            }
            inline void read_async(char *_data, int _size, std::function<void(client&,int)> _callback) {
                make_nonblocking(mSocket);
                mService.add_handler(socket_event_handler(mSocket,
                    [=](){
                        auto result = read(_data, _size);
                        _callback(*this, result);
                    }, nullptr,
                    [=](){
                        _callback(*this, 0);
                    }
                ));
            }
            inline int read(char *_data, int _size) {
                make_blocking(mSocket);
                return ::recv(mSocket, _data, _size, 0);
            }
            inline void close() {
                shutdown_socket(mSocket);
                ::close(mSocket);
                mSocket = invalid_socket;
            }
            inline const std::string &ip() const { return mIP; }
        private:
            inline int invokeConnect(const std::string &_target, int _port) {
                if(mSocket != invalid_socket)
                    throw socket_exception("socket already connected");
                mSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if(mSocket == invalid_socket)
                    __throw_error_with_number("failed to create socket");
                address_info resolved = resolve_to_any(_target);
                socket_address addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(_port);
                addr.sin_addr = ((socket_address*)resolved.ai_addr)->sin_addr;
                return ::connect(mSocket, (sockaddr*)&addr, sizeof(addr));
            }
        private:
            socket mSocket;
            std::string mIP;
        };

        socket_address make_address(const std::string &_hostname, int _port) {
            address_info resolved = resolve_to_any(_hostname);
            socket_address addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(_port);
            addr.sin_addr = ((socket_address*)resolved.ai_addr)->sin_addr;
            return addr;
        }

        class udp_client : public base_socket {
        public:
            inline udp_client(service &_service, socket _socket) : base_socket(_service), mSocket(_socket) {
                // TODO: derive IP string? o:
                mIP = "some-ip-here";
            }
            inline udp_client(service &_service) : base_socket(_service), mSocket(::socket(AF_INET, SOCK_DGRAM, 0)) {}
            inline udp_client(udp_client &&_move)
                : base_socket(_move.mService), mSocket(_move.mSocket), mIP(_move.mIP) {
                _move.mSocket = invalid_socket;
            }
            inline ~udp_client() {
                close();
            }
            inline void write_async(const std::string &_data, socket_address _target, std::function<void(udp_client&,int)> _callback) {
                write_async(_data.c_str(), _data.length(), _target, _callback);
            }
            inline void write_async(const char *_data, int _count, socket_address _target, std::function<void(udp_client&,int)> _callback) {
                make_nonblocking(mSocket);
                mService.add_handler(socket_event_handler(mSocket, nullptr,
                    [=](){
                        auto result = write(_data, _count, _target);
                        _callback(*this, result);
                    },
                    [=](){
                        _callback(*this, 0);
                    }
                ));
            }
            inline int write(const std::string &_data, socket_address _target) {
                return write(_data.c_str(), _data.length(), _target);
            }
            inline int write(const char *_data, int _count, socket_address _target) {
                make_blocking(mSocket);
                return ::sendto(mSocket, _data, _count, 0, (sockaddr*) &_target, sizeof(_target));
            }
            inline void read_async(char *_data, int _size, socket_address &_target, std::function<void(udp_client&,int)> _callback) {
                make_nonblocking(mSocket);
                mService.add_handler(socket_event_handler(mSocket,
                    [=, &_target](){
                        auto result = read(_data, _size, _target);
                        _callback(*this, result);
                    }, nullptr,
                    [=](){
                        _callback(*this, 0);
                    }
                ));
            }
            inline int read(char *_data, int _size, socket_address &_target) {
                make_blocking(mSocket);
                unsigned int length = sizeof(_target);
                return ::recvfrom(mSocket, _data, _size, 0, (sockaddr*) &_target, &length);
            }
            inline void close() {
                shutdown_socket(mSocket);
                ::close(mSocket);
                mSocket = invalid_socket;
            }
            inline const std::string &ip() const { return mIP; }
        protected:
            socket mSocket;
            std::string mIP;
        };
        
        class server : public base_socket
        {
        public:
            inline server(service &_service, int _port) : base_socket(_service), mPort(_port), mSocket(invalid_socket) {}
            inline server(server &&_move) : base_socket(_move.mService), mSocket(_move.mSocket) {
                _move.mSocket = invalid_socket;
            }
            inline ~server() {
                shutdown_socket(mSocket);
                ::close(mSocket);
                mSocket = invalid_socket;
            }
            inline void configure() {
                if(mSocket != invalid_socket)
                    throw socket_exception("server already configured");
                    
                mSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if(mSocket == invalid_socket)
                    __throw_error_with_number("failed to create socket"); 
                socket_address addr;
                
                addr.sin_family = AF_INET;
                addr.sin_port = htons(mPort);
                addr.sin_addr.s_addr = htonl(INADDR_ANY);
                
                auto bindResult = ::bind(mSocket, (sockaddr*)&addr, sizeof(addr));
                if(bindResult < 0)
                    __throw_error_with_number("failed to bind server");
                
                ::listen(mSocket, SOMAXCONN);
            }
            inline void accept_async(std::function<void(server&,bool)> _callback) {
                if(!_callback)
                    throw socket_exception("invalid callback passed to accept_async()");
                mService.add_handler(socket_event_handler(mSocket,
                    [=](){
                        _callback(*this, true);
                    }, nullptr,
                    [=](){
                        _callback(*this, false);
                    }
                ));
            }
            inline client accept() {
                socket_address addr;
                auto addr_size = sizeof(addr);
                socket accepted = ::accept(mSocket, (sockaddr*)&addr, &addr_size);
                if(accepted == invalid_socket)
                    __throw_error_with_number("failed to accept connection");
                return client(mService, accepted);
            }
            inline int port() const { return mPort; }
        private:
            int mPort;
            socket mSocket;
        };

        class udp_server : public udp_client {
        public:
            inline udp_server(service &_service, int _port)
                : udp_client(_service), mPort(_port) {
                mAddress.sin_family = AF_INET;
                mAddress.sin_port = htons(mPort);
                mAddress.sin_addr.s_addr = htonl(INADDR_ANY);

                auto bindResult = ::bind(mSocket, (sockaddr*)&mAddress, sizeof(mAddress));
                if(bindResult < 0)
                    __throw_error_with_number("failed to bind server");
            }
            inline socket_address address() const { return mAddress; }
        private:
            int mPort;
            socket_address mAddress;
        };
    };
};


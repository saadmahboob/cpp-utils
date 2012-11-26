#pragma once

#include <functional>

namespace util
{
    namespace net
    {
        class service
        {
        public:
        private:
        };
        
        class socket
        {
        public:
            inline socket(service &_service) : mService(_service) {}
        protected:
            service &mService;
        };
        
        class client : public socket
        {
        public:
            inline client(service &_service) : socket(_service) {}
            inline client(client &&_move) : socket(_move.mService) {
            }
            inline void connect(const std::string &_target, int _port) {
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
            }
            inline void read_async(char *_data, int _size, std::function<void(client&,int)> _callback) {
            }
            inline int read(char *_data, int _size) {
            }
            inline void close() {
            }
            inline const std::string &ip() const { return mIP; }
        private:
            std::string mIP;
        };
        
        class server : public socket
        {
        public:
            inline server(service &_service, int _port) : socket(_service), mPort(_port) {
            }
            inline server(server &&_move) : socket(_move.mService) {
            }
            inline void accept_async(std::function<void(server&)> _callback) {
            }
            inline client accept() {
            }
            inline int port() const { return mPort; }
        private:
            int mPort;
        };
    };
};


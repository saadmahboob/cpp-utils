#pragma once

#include <fstream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

namespace util
{
    namespace file
    {
        typedef std::fstream handle;
        
        inline bool exists(const std::string &_path)
        {
            std::ifstream file(_path.c_str());
            return (file.is_open());
        }

        inline bool create(const std::string &_path, bool _overwrite=false)
        {
            if(exists(_path) && !_overwrite)
                return false;
            std::ofstream file(_path.c_str());
            file.flush();
            return true;
        }
        
        inline bool make_directory(const std::string &_path)
        {
        #if defined(_WIN32) || defined(_WIN64)
            return (_mkdir(_path.c_str()) == 0);
        #else
            return (mkdir(_path.c_str(), 0777) == 0);
        #endif
        }
        
        inline bool open(handle &_stream, const std::string &_path, std::ios_base::openmode _mode)
        {
            _stream.open(_path.c_str(), _mode);
            if(!_stream.is_open())
                return false;
            return true;
        }
        
        inline bool open_readable(handle &_stream, const std::string &_path, std::ios_base::openmode _mode = std::ios_base::in)
        {
            return open(_stream, _path, _mode);
        }
        
        inline bool open_writable(handle &_stream, const std::string &_path, std::ios_base::openmode _mode = std::ios_base::out)
        {
            return open(_stream, _path, _mode);
        }
    };
};


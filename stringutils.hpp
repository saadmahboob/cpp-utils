#pragma once

#include <algorithm>
#include <vector>
#include <string>

#include "listutils.hpp"

namespace util
{
    typedef std::vector<std::string> string_vector;
    
    namespace string
    {
        template<class StorageType = string_vector>
        inline StorageType split(const std::string &_text,
                            const std::string &_seperator = " ",
                            bool _removeEmpty = true)
        {
            StorageType result;
            std::string::size_type lpos = 0, pos = 0;
            bool done = false;
            
            while(!done)
            {
                std::string token;
                pos = _text.find(_seperator, lpos);
                
                if(pos == -1)
                {
                    token = _text.substr(lpos);
                    done = true;
                }
                else
                {
                    token = _text.substr(lpos, (pos - lpos));
                    lpos = pos + _seperator.length();
                }
                
                if(!token.empty() || !_removeEmpty)
                {
                    result.push_back(token);
                }
            }
            
            return result;
        }
        
        template<class Iterable>
        inline std::string join(const Iterable &_strings, const std::string &_seperator)
        {
            return list::foldl(_strings, std::string(""),
                [&](const std::string &_acc, const std::string &_next) -> std::string {
                    if(_acc.empty())
                        return _next;
                    else
                        return _acc + _seperator + _next;
                }
            );
        }
        
        inline string_vector from_args(int _argc, char *_argv[])
        {
            return string_vector(_argv, _argv + _argc);
        }
        
        inline std::string strip(const std::string &_string)
        {
            unsigned int start = 0;
            unsigned int end = _string.length();
            
            while(start < _string.length() && isspace(_string[start]))
                start++;
            while(end > start && isspace(_string[end-1]))
                end--;
            
            return _string.substr(start, end - start);
        }
        
        template<typename T>
        inline T to(const std::string &_string)
        {
            std::stringstream convert(_string);
            T result;
            convert >> result;
            return result;
        }
        
        template<typename T>
        inline std::string from(const T &_value)
        {
            std::stringstream convert;
            convert << _value;
            return convert.str();
        }
    };
};


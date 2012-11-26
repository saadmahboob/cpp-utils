#pragma once

#include <algorithm>
#include <iterator>
#include <sstream>
#include <list>

namespace util
{
    namespace list
    {
        class empty_list_exception : std::exception {};
        class not_enough_elements_exception : std::exception {};
        
        template<class IterType>
        int length(IterType _begin, IterType _end)
        {
            int result = 0;
            std::for_each(_begin, _end,
                [&](decltype(*_begin) _) {
                    result ++;
                }
            );
            return result;
        }
        
        template<typename Iterable>
        int length(Iterable _iterable)
        {
            return length(std::begin(_iterable), std::end(_iterable));
        }
        
        template<typename Ret, class IterType, typename Fun>
        Ret foldl(IterType _begin, IterType _end, Ret _seed, Fun _fn)
        {
            typedef decltype(*_begin) ElemType;
            std::for_each(_begin, _end, [&](const ElemType &_elem) {
                _seed = _fn(_seed, _elem);
            });
            return _seed;
        }
        
        template<typename Ret, typename Iterable, typename Fun>
        Ret foldl(Iterable _iterable, Ret _seed, Fun _fn)
        {
            return foldl(std::begin(_iterable), std::end(_iterable), _seed, _fn);
        }
        
        template<class IterType, typename Fun>
        auto foldl1(IterType _begin, IterType _end, Fun _fn)
            -> decltype(_fn(*_begin,*_end))
        {
            if(_begin == _end) throw empty_list_exception();
            typedef decltype(*_begin) ElemType;
            ElemType first = *_begin++;
            if(_begin == _end) throw not_enough_elements_exception();
            auto seed = _fn(first, *_begin++);
            return foldl(_begin, _end, seed, _fn);
        }
        
        template<typename Iterable, typename Fun>
        auto foldl1(Iterable _iterable, Fun _fn)
            -> decltype(_fn(*std::begin(_iterable),*std::end(_iterable)))
        {
            return foldl1(std::begin(_iterable), std::begin(_iterable), _fn);
        }
        
        template<class IterType>
        std::string stringify(IterType _begin, IterType _end)
        {
            typedef decltype(*_begin) ElemType;
            const std::string seperator = ", ";
            std::string result;
            
            int len = length(_begin, _end);
            
            auto fn = [&](const std::string &_a, const ElemType &_b) -> std::string {
                std::stringstream convert;
                convert << _a << seperator << _b;
                return convert.str();
            };

            if(len < 1)
                result = "";
            else if(len < 2)
                result = *_begin;
            else
                result = foldl1(_begin, _end, fn);
            
            return std::string("[") + result + std::string("]");
        }
        
        template<typename Iterable>
        std::string stringify(Iterable _iterable)
        {
            return stringify(std::begin(_iterable), std::end(_iterable));
        }
        
        template<class CollectionType>
        CollectionType tail(const CollectionType &_collection)
        {
            auto start = std::begin(_collection);
            start ++;
            return CollectionType(start, std::end(_collection));
        }
    };
};


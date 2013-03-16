#pragma once
#ifndef IFACE_HPP
#define IFACE_HPP

#include "event.hpp"

namespace util {
	namespace iface {
		template<typename... T>
		void connect(event::listener<T...> &_listener, event::notifier<T...> &_source) {
			_source.listen(&_listener);
		}

		template<typename... T>
		void connect(event::listener<T...> &_listener, event::notifier_public<T...> _source) {
			_source.listen(&_listener);
		}

		template<typename ObjA, typename ObjB, typename R, typename... T>
		void connect(ObjA *_a, std::function<R(T...)> (ObjA::*_member), ObjB *_b, R (ObjB::*_method)(T...)) {
			(_a->*_member) = [&, _b, _method](T... _args) -> R {
				return (_b->*_method)(_args...);
			};
		}
	}
}

#endif

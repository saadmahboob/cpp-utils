#pragma once
#ifndef IFACEUTILS_HPP
#define IFACEUTILS_HPP

#include "metautils.hpp"
#include "eventutils.hpp"

namespace util {
	namespace iface {
		template<typename... T>
		void connect(event::listener<T...> &_listener, event::notifier<T...> &_source) {
			_listener.listen(_source.public_interface());
		}

		template<typename... T>
		void connect(event::listener<T...> &_listener, event::notifier_public<T...> *_source) {
			_listener.listen(_source);
		}

		template<typename ObjA, typename ObjB, typename R, typename... T>
		void connect(ObjA *_a, std::function<R(T...)> (ObjA::*_member), ObjB *_b, R (ObjB::*_method)(T...)) {
			(_a->*_member) = meta::wrap_method(_b, _method);
		}
	}
}

#endif

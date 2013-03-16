#pragma once
#ifndef EVENT_HPP

#include <algorithm>
#include <set>

namespace util {
	namespace event {
		template<typename... T>
		class listener {
		public:
			virtual void updated(T...) = 0;
		};

		template<typename... T>
		class lambda_listener : public listener<T...> {
		public:
			lambda_listener(std::function<void(T...)> _lambda) : mLambda(_lambda) {}
			lambda_listener() {}
			void updated(T... _arguments) override {
				mLambda(_arguments...);
			}
		private:
			std::function<void(T...)> mLambda;
		};

		// forward declaration
		template<typename... T>
		class notifier_public;

		template<typename... T>
		class notifier {
		public:
			void notify(T... _arguments)  {
				for(auto listener : mListeners) {
					listener->updated(_arguments...);
				}
			}
			bool listen(listener<T...> *_listener) {
				return (mListeners.insert(_listener).second);
			}
			bool unlisten(listener<T...> *_listener) {
				return (mListeners.erase(_listener) > 0);
			}
			notifier_public<T...> public_interface();
		private:
			std::set<listener<T...>*> mListeners;
		};

		template<typename... T>
		class notifier_public {
		public:
			notifier_public(const notifier_public &_other) : mNotifier(_other.mNotifier) {}
			notifier_public(notifier_public &&_other) : mNotifier(_other.mNotifier) {}
			notifier_public(notifier<T...> &_notifier) : mNotifier(_notifier) {}
			bool listen(listener<T...> *_listener) {
				return mNotifier.listen(_listener);
			}
			bool unlisten(listener<T...> *_listener) {
				return mNotifier.listen(_listener);
			}
		private:
			notifier<T...> &mNotifier;
		};

		template<typename... T>
		notifier_public<T...> notifier<T...>::public_interface() {
			return notifier_public<T...>(*this);
		}
	}
}

#endif

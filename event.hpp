#pragma once
#ifndef EVENT_HPP

#include <algorithm>
#include <set>

namespace util {
	namespace event {
		// forward declaration
		template<typename... T>
		class notifier_public;

		template<typename... T>
		class listener {
		public:
			listener() : mNotifier(nullptr) {}
			~listener();
			void listen(notifier_public<T...> *_notifier);
			void unlisten();
			virtual void updated(T...) = 0;
		private:
			notifier_public<T...> *mNotifier;
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

		template<typename... T>
		class notifier {
		public:
			notifier();
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
			notifier_public<T...> *public_interface();
		private:
			notifier_public<T...> mPublicInterface;
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
		listener<T...>::~listener() {
			unlisten();
		}

		template<typename... T>
		void listener<T...>::listen(notifier_public<T...> *_notifier) {
			if(_notifier != nullptr) {
				if(mNotifier != nullptr)
					unlisten();
				mNotifier = _notifier;
				mNotifier->listen(this);
			}
		}

		template<typename... T>
		void listener<T...>::unlisten() {
			if(mNotifier != nullptr) {
				mNotifier->unlisten(this);
				mNotifier = nullptr;
			}
		}

		template<typename... T>
		notifier<T...>::notifier() : mPublicInterface(*this) {}

		template<typename... T>
		notifier_public<T...> *notifier<T...>::public_interface() {
			return &mPublicInterface;
		}
	}
}

#endif

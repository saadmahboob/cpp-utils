#pragma once

namespace util
{
	namespace meta
	{
		namespace internal {
            struct null_return_t {};

            template<typename T>
            internal::null_return_t operator<<(std::ostream&,const T&);

            template<typename T>
            struct stream_writable : std::integral_constant<
                bool,
                !std::is_same<
                    decltype(std::declval<std::ostream>() << std::declval<T>()),
                    internal::null_return_t
                >::value
            > {};
        }

        template<typename T>
        struct stream_writable : public internal::stream_writable<T> {
            using internal::stream_writable<T>::value;
        };
	};
};

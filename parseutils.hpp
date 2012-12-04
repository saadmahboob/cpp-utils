#pragma once

#include <functional>
#include <stdexcept>
#include <istream>
#include <array>

#include "stringutils.hpp"

namespace util
{
	namespace parse
	{
		struct stream_position
		{
			int line_number;
			int column;
		};

		class exception : public std::runtime_error {
		public:
			exception(const stream_position &_where, const std::string &_error)
				: std::runtime_error(
					"[" + util::string::from(_where.line_number) + ":"
						+ util::string::from(_where.column) + "] " + _error),
				  mPosition(_where) {}
			inline const stream_position &position() const { return mPosition; }
			inline int linenumber() const { return mPosition.line_number; }
			inline int column() const { return mPosition.column; }
		private:
			stream_position mPosition;
		};

		class reader
		{
		public:
			inline reader(std::istream &_stream)
				: mStream(_stream), mPosition{0,1} {}
			inline bool eof() {
				return _eof();
			}
			inline char peek() { return _peek(); }
			inline char get() {
				char c = _get();
				mPosition.column ++;
				if(c == '\n') {
					mPosition.column = 1;
					mPosition.line_number ++;
				}
				return c;
			}
			inline void put(char _c) {
				mBuffer = _c + mBuffer;
				mPosition.column --;
				if(_c == '\n') {
					// no nice way to know correct column value, but
					// to be honest, 'put()'ing things back into a stream
					// is a bad idea to begin with... ;O
					mPosition.column = 1;
					mPosition.line_number --;
				}
			}
			inline void put(const std::string &_str) {
				for(auto i = _str.length() - 1; i >= 0; i--)
					put(_str[i]);
			}
			inline int column() const { return mPosition.column; }
			inline int linenumber() const { return mPosition.line_number; }
			inline const stream_position &position() const { return mPosition; }
			inline bool skip_whitespace(bool _eof_allowed=false) {
				while(!eof() && isspace(peek()))
					get();
				if(eof() && !_eof_allowed)
					error("unexpected end of stream");
				return eof();
			}
			inline bool skip_to_nextline(bool _eof_allowed=false) {
				while(!eof() && get() != '\n')
					;
				if(eof() && !_eof_allowed)
					error("unexpected end of stream");
				return eof();
			}
			inline bool skip_expected(const std::string &_expected, bool _error=true) {
				int index = 0;
				while(!eof() && peek() == _expected[index]) {
					index ++;
					get();
				}
				if(index < _expected.size() && _error)
					error("expected '" + _expected + "', got: '" + _expected.substr(0, index) + "'");
				return (index < _expected.size());
			}
		private:
			inline void error(const std::string &_message) {
				throw exception(mPosition, _message);
			}
			inline char _get() {
				char result = _peek();
				mBuffer = mBuffer.substr(1);
				return result;
			}
			inline char _peek() {
				char result = '\0';
				if(mBuffer.empty() && !mStream.eof()) {
					load_buffer();
				}
				result = mBuffer.at(0);
				return result;
			}
			inline bool _eof() {
				return mBuffer.empty() && mStream.eof();
			}
			inline void load_buffer() {
				std::array<char, 1024> buffer;
				auto read = mStream.readsome(buffer.data(), 1024);
				if(read > 0)
					mBuffer += std::string(buffer.data(), read);
			}
		private:
			std::string mBuffer;
			std::istream &mStream;
			stream_position mPosition;
		};
	};
};

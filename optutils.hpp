#pragma once

#include <stdexcept>
#include <iostream>
#include <set>

#include "stringutils.hpp"

namespace util
{
	namespace opt
	{
		class option
		{
		public:
			inline option(const std::string &_opt, bool _parameters, bool _long, const std::string _documentation="")
				: mOption(_opt), mParameters(_parameters), mLong(_long), mDocumentation(_documentation) {}
			inline bool is_expecting_parameters() const { return mParameters; }
			inline const std::string &name() const { return mOption; }
			inline const std::string &documentation() const { return mDocumentation; }
			inline bool is_long() const { return mLong; }
			inline bool is_option(const std::string &_opt) const {
				if(mOption == _opt)
					return true;
				if(mLong && mOption.find(_opt) == 0)
					return true;
				return false;
			}
			inline bool operator<(const option &_other) const {
				return (mOption < _other.name());
			}
		private:
			bool mLong;
			bool mParameters;
			std::string mOption;
			std::string mDocumentation;
		};

		class passed_option
		{
		public:
			inline passed_option(const option &_option, const std::string &_argument="")
				: mArgument(_argument), mOption(_option) {}
			inline const option &get_option() const { return mOption; }
			inline const std::string &argument() const { return mArgument; }
			template<typename T>
			inline T get_as() const { return util::string::to<T>(mArgument); }
			template<typename T>
			inline T get_as(T _default) const {
				if(mArgument.empty())
					return _default;
				return get_as<T>();
			}
			inline bool operator<(const passed_option &_other) const {
				return (mOption < _other.get_option());
			}
		private:
			option mOption;
			std::string mArgument;
		};

		class parse_result
		{
		public:
			inline void add(const std::string &_nonoption) {
				mNonOptions.push_back(_nonoption);
			}
			inline bool add(const passed_option &_passed, bool _errors) {
				auto ret = mOptions.insert(_passed);
				if(!ret.second && _errors)
					throw std::runtime_error("option '" + _passed.get_option().name() + "' already set");
				return ret.second;
			}
			inline bool has_option(const std::string &_option) const {
				passed_option dummy(option(_option, false, false));
				auto ret = mOptions.find(dummy);
				return (ret != mOptions.end());
			}
			inline passed_option get_option(const std::string &_option) const {
				passed_option dummy(option(_option, false, false));
				auto ret = mOptions.find(dummy);
				if(ret == mOptions.end())
					throw std::runtime_error("unprovided option '" + _option + "'");
				return *ret;
			}
			inline const std::vector<std::string> &nonoptions() const {
				return mNonOptions;
			}
		private:
			std::set<passed_option> mOptions;
			std::vector<std::string> mNonOptions;
		};

		class parser
		{
		public:
			inline bool add(const option &_option, bool _error=true) {
				auto ret = mOptions.insert(_option);
				if(!ret.second && _error)
					throw std::runtime_error("added argument '" + _option.name() + "' that conflicts");
				return ret.second;
			}
			inline bool has(const std::string &_option) const {
				auto result = std::find_if(std::begin(mOptions), std::end(mOptions),
					[&](const option &_opt){
						return (_opt.is_option(_option));
					}
				);
				return (result != mOptions.end());
			}
			inline parse_result parse(int _argc, char *_argv[]) {
				return parse(util::string::from_args(_argc, _argv));
			}
			inline parse_result parse(const util::string_vector &_args, bool _errors=true) {
				parse_result result;
				std::string optionName;
				bool expectingArgument = false;
				util::string_vector::const_iterator last_arg;
				for(auto i = std::begin(_args); i != std::end(_args); i++) {
					std::string argument = *i;
					if(!expectingArgument) {
						if(is_option(argument)) {
							optionName = get_option_name(argument);
							if(!has(optionName)) {
								if(_errors)
									throw std::runtime_error("invalid option '" + argument + "'");
							} else {
								option opt = get_option(optionName);
								if(opt.is_expecting_parameters())
									expectingArgument = true;
								else
									result.add(passed_option(opt), _errors);
							}
						} else {
							result.add(argument);
						}
					} else {
						expectingArgument = false;
						option opt = get_option(optionName);

						if(!is_option(argument)) {	
							result.add(passed_option(opt, argument), _errors);
						} else {
							result.add(passed_option(opt), _errors);
							i = last_arg;
						}
					}
					last_arg = i;
				}
				if(expectingArgument)
					result.add(passed_option(get_option(optionName)), _errors);
				return result;
			}
		private:
			inline bool is_option(const std::string &_str) const {
				return ((_str.find("-") == 0) && _str != "--" && _str != "-");
			}
			inline std::string get_option_name(const std::string &_str) const {
				if(_str.find("--") == 0 && _str != "--")
					return _str.substr(2);
				if(_str.find("-") == 0 && _str != "-")
					return _str.substr(1);
				return _str;
			}
			inline option get_option(const std::string &_option) {
				auto ret = std::find_if(std::begin(mOptions), std::end(mOptions),
					[&](const option &_opt){
						return (_opt.is_option(_option));
					}
				);
				if(ret == mOptions.end())
					throw std::runtime_error("internal error: attempted to get invalid option");
				return *ret;
			}
		private:
			std::set<option> mOptions;
		};
	}
}

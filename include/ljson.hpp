/*
 * Copyright: 2025 nodeluna
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <any>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <stack>
#include <fstream>
#include <format>
#include <functional>
#include <unordered_set>
#include <variant>
#include <vector>
#include <cassert>
#include <source_location>
#include <type_traits>

namespace ljson {

	struct monostate {};

	template<class T = monostate, class E = monostate>
	class expected {
		private:
			bool		   _has_value = false;
			std::variant<T, E> _value_or_error;

		public:
			expected(const T& t) : _has_value(true), _value_or_error(t)
			{
			}

			expected(const E& e) : _has_value(false), _value_or_error(e)
			{
			}

			expected() : _has_value(true), _value_or_error(T())
			{
			}

			expected& operator=(const expected& other)
			{
				if (this != &other)
				{
					this->~expected();
					_has_value	= other._has_value;
					_value_or_error = other._value_or_error;
				}

				return *this;
			}

			template<class U>
			expected(const expected<U, E>& other) : _has_value(other.has_value())
			{
				if (_has_value)
				{
					if constexpr (std::is_same_v<U, monostate>)
					{
						_value_or_error = T();
					}
					else if constexpr (std::is_same_v<U, T>)
					{
						_value_or_error = other.value();
					}
					else
					{
						static_assert(
						    not std::is_same_v<U, T> && "no available conversion between the provided value types");
					}
				}
				else
				{
					_value_or_error = other.error();
				}
			}

			expected(const expected&& other) noexcept
			    : _has_value(other._has_value), _value_or_error(std::move(other._value_or_error))
			{
			}

			expected& operator=(const expected&& other) noexcept
			{
				if (this != &other)
				{
					this->_has_value      = other._has_value;
					this->_value_or_error = std::move(other._value_or_error);
				}

				return *this;
			}

			~expected()
			{
			}

			bool has_value() const noexcept
			{
				return _has_value;
			}

			explicit operator bool() const noexcept
			{
				return this->has_value();
			}

			constexpr const T& value() const&
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return std::get<T>(_value_or_error);
			}

			constexpr const E& error() const&
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return std::get<E>(_value_or_error);
			}

			constexpr T& value() &
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return std::get<T>(_value_or_error);
			}

			constexpr E& error() &
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return std::get<E>(_value_or_error);
			}

			constexpr const T&& value() const&&
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return std::get<T>(_value_or_error);
			}

			constexpr const E&& error() const&&
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return std::get<E>(_value_or_error);
			}

			constexpr T&& value() &&
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return std::get<T>(_value_or_error);
			}

			constexpr E&& error() &&
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return std::get<E>(_value_or_error);
			}

			template<class U = std::remove_cv_t<T>>
			constexpr T value_or(U&& other) const&
			{
				static_assert(std::is_convertible_v<U, T>);
				if (_has_value)
					return std::get<T>(_value_or_error);
				else
					return static_cast<T>(std::forward<U>(other));
			}

			template<class U = std::remove_cv_t<T>>
			constexpr E error_or(U&& other) const&
			{
				static_assert(std::is_convertible_v<U, E>);
				if (not _has_value)
					return std::get<E>(_value_or_error);
				else
					return static_cast<E>(std::forward<U>(other));
			}
	};

	template<class E>
	expected<monostate, E> unexpected(const E& e)
	{
		return expected<monostate, E>(e);
	}

	expected<monostate, std::string> unexpected(const char* e)
	{
		return unexpected<std::string>(std::string(e));
	}

	std::string log(const std::string& msg, std::source_location location = std::source_location::current())
	{
		const std::string reset	 = "\x1b[0m";
		const std::string color1 = "\x1b[31m";
		const std::string color2 = "\x1b[32m";
		const std::string color3 = "\x1b[34m";

		std::string output = std::format("{}[{}:{}]{}\n", color1, location.file_name(), location.line(), reset);
		output += std::format("{}\t|{}\n", color3, reset);
		output += std::format("{}\t|__function--> {}{}\n", color3, location.function_name(), reset);
		output += std::format("{}\t\t|{}\n", color2, reset);
		output += std::format("{}\t\t|__message--> '{}'{}\n", color2, msg, reset);
		output += std::format("\n");

		return output;
	}

	void print_log(const std::string& msg, std::source_location location = std::source_location::current())
	{
		std::cout << log(msg, location);
	}

	bool is_num_decimal(const std::string& x)
	{
		if (x.empty())
			return false;
		bool has_decimal = false;
		return std::all_of(x.begin(), x.end(),
		    [&](char c)
		    {
			    if (std::isdigit(c))
				    return true;
			    else if (c == '.' && not has_decimal)
			    {
				    has_decimal = true;
				    return true;
			    }
			    else
				    return false;
		    });
	}

	enum class error_type {
		none,
		key_not_found,
		filesystem_error,
		parsing_error,
		parsing_error_wrong_type,
		wrong_type,
		wronge_index,
	};

	class error {
		private:
			error_type  err_type;
			std::string msg;

		public:
			error(error_type err, const std::string& message);

			const char*	   what() const noexcept;
			const std::string& message() const noexcept;
			error_type	   value();
	};

	enum class value_type {
		none,
		string,
		number,
		integer,
		double_t,
		null,
		boolean,
		temp_escape_type,
		unknown,
	};

	enum class node_type {
		object,
		array,
		value,
	};

	enum class json_syntax {
		opening_bracket,
		closing_bracket,
		quotes_1,
		quotes_2,
		column,
		string_value,
		object,
		array,
		maybe_empty_space_after,
		flush_value,
	};

	enum class key_type {
		none,
		simple_key,
		object,
		array,
	};

	class null_type {
		public:
			null_type() = default;

			bool operator==(const null_type&) const
			{
				return true;
			}

			bool operator!=(const null_type&) const
			{
				return false;
			}
	};

	inline null_type null;

	template<typename allowed_value_types>
	concept is_allowed_value_type = std::is_same_v<allowed_value_types, std::string> ||
					std::is_same_v<allowed_value_types, const char*> || std::is_arithmetic_v<allowed_value_types> ||
					std::is_same_v<allowed_value_types, null_type> || std::is_same_v<allowed_value_types, bool>;

	class value {
		private:
			using value_type_variant  = std::variant<std::string, double, int64_t, bool, null_type, monostate>;
			value_type_variant _value = monostate();
			value_type	   _type  = value_type::none;

			template<is_allowed_value_type val_type>
			void set_state(const val_type& val) noexcept
			{
				if constexpr (std::is_same_v<val_type, bool>)
				{
					_type  = value_type::boolean;
					_value = val;
				}
				else if constexpr (std::is_arithmetic_v<val_type>)
				{
					if constexpr (std::is_floating_point_v<val_type>)
					{
						_type = value_type::double_t;
					}
					else
					{
						_type = value_type::integer;
					}

					_value = val;
				}
				else if constexpr (std::is_same_v<val_type, std::string> || std::is_same_v<val_type, const char*> ||
						   std::is_same_v<val_type, char*>)
				{
					_type  = value_type::string;
					_value = val;
				}
				else if constexpr (std::is_same_v<val_type, null_type>)
				{
					_type  = value_type::null;
					_value = val;
				}
				else if constexpr (std::is_same_v<val_type, monostate>)
				{
					_type  = value_type::none;
					_value = monostate();
				}
				else
				{
					static_assert(false && "unsupported value_type in class value");
				}
			}

			expected<monostate, error> set_state(const std::string& val, value_type t)
			{
				_type = t;
				if (t == value_type::double_t)
				{
					_value = std::stod(val);
				}
				else if (t == value_type::integer)
				{
					_value = std::stoll(val);
				}
				else if (t == value_type::string)
				{
					_value = val;
				}
				else if (t == value_type::boolean)
				{
					_value = (val == "true" ? true : false);
				}
				else if (t == value_type::null)
				{
					_value = null_type();
				}
				else if (t == value_type::none)
				{
					_value = monostate();
				}
				else
				{
					_type  = value_type::none;
					_value = monostate();
					return unexpected(error(error_type::wrong_type, "unsupported value_type in class value"));
				}

				return monostate();
			}

		public:
			template<is_allowed_value_type val_type>
			value(const val_type& val) noexcept
			{
				this->set_state(val);
			}

			value(const value& other) : _value(other._value), _type(other._type)
			{
			}

			value& operator=(const value& other)
			{
				_value = other._value;
				_type  = other._type;
				return *this;
			}

			value(const value&& other) : _value(std::move(other._value)), _type(other._type)
			{
			}

			value& operator=(const value&& other)
			{
				_value = std::move(other._value);
				_type  = other._type;
				return *this;
			}

			value() : _value(monostate()), _type(value_type::none)
			{
			}

			ljson::value_type type() const
			{
				return _type;
			}

			template<is_allowed_value_type val_type>
			void set_value_type(const val_type& val) noexcept
			{
				this->set_state(val);
			}

			expected<monostate, error> set_value_type(const std::string& val, value_type t)
			{
				return this->set_state(val, t);
			}

			bool is_string() const
			{
				return std::holds_alternative<std::string>(_value);
			}

			bool is_number() const
			{
				return std::holds_alternative<double>(_value) || std::holds_alternative<int64_t>(_value);
			}

			bool is_double() const
			{
				return std::holds_alternative<double>(_value);
			}

			bool is_integer() const
			{
				return std::holds_alternative<int64_t>(_value);
			}

			bool is_boolean() const
			{
				return std::holds_alternative<bool>(_value);
			}

			bool is_null() const
			{
				return std::holds_alternative<null_type>(_value);
			}

			bool is_empty() const
			{
				return std::holds_alternative<monostate>(_value);
			}

			std::string as_string()
			{
				if (not this->is_string())
					throw error(error_type::wrong_type, "wrong type: trying to cast a non-string to a string");

				return std::get<std::string>(_value);
			}

			double as_number()
			{
				if (not this->is_number())
					throw error(error_type::wrong_type, "wrong type: trying to cast a non-number to a number");

				if (not this->is_double())
					return std::get<double>(_value);
				else
					return std::get<int64_t>(_value);
			}

			int64_t as_integer()
			{
				if (not this->is_integer())
					throw error(error_type::wrong_type, "wrong type: trying to cast a non-integer to an integer");

				return std::get<int64_t>(_value);
			}

			double as_double()
			{
				if (not this->is_double())
					throw error(error_type::wrong_type, "wrong type: trying to cast a non-double to a double");

				return std::get<double>(_value);
			}

			bool as_boolean()
			{
				if (not this->is_boolean())
					throw error(error_type::wrong_type, "wrong type: trying to cast a non-boolean to a boolean");

				return std::get<bool>(_value);
			}

			null_type as_null()
			{
				if (not this->is_null())
					throw error(error_type::wrong_type, "wrong type: trying to cast a non-null to a null");

				return std::get<null_type>(_value);
			}

			std::string stringify() const noexcept
			{
				if (this->is_double())
				{
					std::string str = std::to_string(std::get<double>(_value));

					auto pop_zeros_at_the_end = [&]()
					{
						bool found_zero = false;
						for (size_t i = str.size() - 1;; i--)
						{
							if (str[i] == '0' && not found_zero)
							{
								found_zero = true;
							}
							else if (str[i] == '0' && found_zero)
							{
								str.pop_back();
							}
							else if (found_zero)
							{
								str.pop_back();
								found_zero = false;
							}
							else
								break;

							if (i == 0)
								break;
						}

						if (not str.empty() && str.back() == '.')
							str += "0";
					};

					pop_zeros_at_the_end();

					return str;
				}
				else if (this->is_integer())
				{
					return std::to_string(std::get<int64_t>(_value));
				}
				else if (this->is_string())
				{
					return std::get<std::string>(_value);
				}
				else if (this->is_boolean())
				{
					return std::get<bool>(_value) == true ? "true" : "false";
				}
				else if (this->is_null())
				{
					return "null";
				}
				else
				{
					return "";
				}
			}

			std::string type_name() const noexcept
			{
				if (this->is_string())
					return "string";
				else if (this->is_boolean())
					return "boolean";
				else if (this->is_null())
					return "null";
				else if (this->is_double())
					return "double";
				else if (this->is_integer())
					return "integer";
				else if (this->is_empty())
					return "none";
				else
					return "unknown";
			}
	};

	class json;
	class array;
	class object;
	class node;

	template<typename allowed_node_types>
	concept is_allowed_node_type = std::is_same_v<allowed_node_types, std::string> || std::is_same_v<allowed_node_types, const char*> ||
				       std::is_arithmetic_v<allowed_node_types> || std::is_same_v<allowed_node_types, null_type> ||
				       std::is_same_v<allowed_node_types, bool> || std::is_same_v<allowed_node_types, ljson::node> ||
				       std::is_same_v<allowed_node_types, class value>;

	template<typename container_type>
	concept is_key_value_container = requires(container_type container) {
		typename container_type::key_type;
		typename container_type::mapped_type;
		{ container.begin() } -> std::same_as<typename container_type::iterator>;
		{ container.end() } -> std::same_as<typename container_type::iterator>;
	} && std::is_same_v<typename container_type::key_type, std::string> && is_allowed_node_type<typename container_type::mapped_type>;

	template<typename container_type>
	concept is_value_container = requires(container_type container) {
		typename container_type::value_type;
		{ container.begin() } -> std::same_as<typename container_type::iterator>;
		{ container.end() } -> std::same_as<typename container_type::iterator>;
	} && not is_key_value_container<container_type> && is_allowed_node_type<typename container_type::value_type>;

	template<typename container_type>
	concept container_type_concept = is_key_value_container<container_type> || is_value_container<container_type>;

	template<typename value_type>
	concept node_type_concept = container_type_concept<value_type> || is_allowed_node_type<value_type>;

	using object_pairs = std::initializer_list<std::pair<std::string, std::any>>;
	using array_values = std::initializer_list<std::any>;

	using json_object = std::map<std::string, class node>;
	using json_array  = std::vector<class node>;
	using json_node	  = std::variant<std::shared_ptr<class value>, std::shared_ptr<ljson::array>, std::shared_ptr<ljson::object>>;

	class node {
		private:
			json_node _node;

		protected:
			void handle_std_any(const std::any& any_value, std::function<void(std::any)> insert_func);

			template<typename is_allowed_node_type>
			std::variant<class value, ljson::node> handle_allowed_node_types(const is_allowed_node_type& value);

		public:
			explicit node();
			explicit node(const json_node& n);
			explicit node(enum node_type type);
			explicit node(const class value& value);

			template<typename container_type_concept>
			explicit node(const container_type_concept& container);

			node(const std::initializer_list<std::pair<std::string, std::any>>& pairs);
			node(const std::initializer_list<std::any>& val);

			template<typename node_type_concept>
			expected<class ljson::node, error> insert(const std::string& key, const node_type_concept& node);

			template<typename node_type_concept>
			expected<class ljson::node, error> push_back(const node_type_concept& node);

			std::shared_ptr<class value>  as_value() const;
			std::shared_ptr<ljson::array>  as_array() const;
			std::shared_ptr<ljson::object> as_object() const;
			bool			       is_value() const;
			bool			       is_array() const;
			bool			       is_object() const;
			node_type		       type() const;
			bool			       contains(const std::string& key) const;
			class node&		       at(const std::string& object_key) const;
			class node&		       at(const size_t array_index) const;

			template<typename number_type>
			class node& operator=(const number_type& val);
			class node& operator=(const std::shared_ptr<class value>& val);
			class node& operator=(const class value& val);
			class node& operator=(const std::shared_ptr<ljson::array>& arr);
			class node& operator=(const std::shared_ptr<ljson::object>& obj);
			class node& operator=(const std::string& val);
			class node& operator=(const char* val);
			class node& operator=(const bool val);
			class node& operator=(const null_type val);

			class node& operator+=(const std::initializer_list<std::pair<std::string, std::any>>& pairs);
			class node& operator+=(const std::initializer_list<std::any>& val);
			class node  operator+(const node& other_node);

			template<typename number_type>
			expected<monostate, error> set(const number_type value);
			expected<monostate, error> set(const class value& value);
			expected<monostate, error> set(const std::string& value);
			expected<monostate, error> set(const bool value);
			expected<monostate, error> set(const ljson::null_type value);
			expected<monostate, error> set(const char* value);

			void dump(const std::function<void(std::string)> out_func, const std::pair<char, int>& indent_conf = {' ', 4},
			    int indent = 0) const;
			void dump_to_stdout(const std::pair<char, int>& indent_conf = {' ', 4});
			std::string		   dump_to_string(const std::pair<char, int>& indent_conf = {' ', 4});
			expected<monostate, error> write_to_file(
			    const std::filesystem::path& path, const std::pair<char, int>& indent_conf = {' ', 4});

			expected<class ljson::node, error> add_value_to_key(const std::string& key, const class value& value);
			expected<class ljson::node, error> add_node_to_key(const std::string& key, const ljson::node& node);
			expected<class ljson::node, error> add_value_to_array(const class value& value);
			expected<class ljson::node, error> add_value_to_array(const size_t index, const class value& value);
			expected<class ljson::node, error> add_node_to_array(const ljson::node& node);
			expected<class ljson::node, error> add_node_to_array(const size_t index, const ljson::node& node);
			expected<class ljson::node, error> add_array_to_key(const std::string& key);
			expected<class ljson::node, error> add_object_to_array();
			expected<class ljson::node, error> add_object_to_key(const std::string& key);
	};

	class array {
		private:
			json_array _array;

		public:
			explicit array(const json_array& arr) : _array(arr)
			{
			}

			explicit array()
			{
			}

			void push_back(const class node& element)
			{
				return _array.push_back(element);
			}

			void pop_back()
			{
				return _array.pop_back();
			}

			json_array::iterator erase(const json_array::iterator pos)
			{
				return _array.erase(pos);
			}

			json_array::iterator erase(const json_array::iterator begin, const json_array::iterator end)
			{
				return _array.erase(begin, end);
			}

			class node& front()
			{
				return _array.front();
			}

			class node& back()
			{
				return _array.back();
			}

			size_t size()
			{
				return _array.size();
			}

			bool empty()
			{
				return _array.empty();
			}

			json_array::iterator begin()
			{
				return _array.begin();
			}

			json_array::iterator end()
			{
				return _array.end();
			}

			class ljson::node& at(size_t i)
			{
				return _array.at(i);
			}

			class ljson::node& operator[](size_t i)
			{
				return _array[i];
			}
	};

	class object {
		private:
			json_object _object;

		public:
			explicit object() : _object(json_object{})
			{
			}

			ljson::node insert(const std::string& key, const class node& element)
			{
				auto itr = _object[key] = element;
				return itr;
			}

			json_object::size_type erase(const std::string& key)
			{
				return _object.erase(key);
			}

			json_object::iterator erase(const json_object::iterator pos)
			{
				return _object.erase(pos);
			}

			json_object::iterator erase(const json_object::iterator begin, const json_object::iterator end)
			{
				return _object.erase(begin, end);
			}

			size_t size()
			{
				return _object.size();
			}

			bool empty()
			{
				return _object.empty();
			}

			json_object::iterator find(const std::string& key)
			{
				return _object.find(key);
			}

			json_object::iterator begin()
			{
				return _object.begin();
			}

			json_object::iterator end()
			{
				return _object.end();
			}

			class ljson::node& at(const std::string& key)
			{
				return _object[key];
			}

			class ljson::node& operator[](const std::string& key)
			{
				return _object[key];
			}
	};

	class parser {
		private:
			bool done_or_not_ok(const expected<bool, error>& ok);
			void throw_error_if_not_ok(const expected<bool, error>& ok);
			void check_unhandled_hierarchy(const std::string& raw_json, struct parsing_data& data);
			void parsing(struct parsing_data& data);

		public:
			explicit parser();
			~parser();
			ljson::node parse(const std::filesystem::path& path);
			ljson::node parse(const std::string& raw_json);
			ljson::node parse(const char* raw_json);
	};
}

namespace ljson {
	struct parsing_data {
			std::string				     line;
			size_t					     i		 = 0;
			size_t					     line_number = 1;
			std::stack<std::pair<std::string, key_type>> keys;
			std::stack<ljson::node>			     json_objs;
			std::pair<std::string, value_type>	     raw_value;
			class value				     value;
			std::stack<std::pair<json_syntax, size_t>>   hierarchy;
	};

	struct parser_syntax {
			struct open_bracket {
					static expected<bool, error> handle_open_bracket(struct parsing_data& data)
					{
						if (data.line[data.i] == '{' && not value::is_string(data))
						{
							data.hierarchy.push({json_syntax::opening_bracket, data.line_number});
							return true;
						}

						return false;
					}
			};

			struct empty {
					static expected<bool, error> handle_empty(struct parsing_data& data)
					{
						if (data.line[data.i] != ' ' && data.line[data.i] != '\t')
							return false;
						else if (data.hierarchy.empty())
							return true;
						else if (quotes::is_hierarchy_qoutes(data.hierarchy))
							return false;
						else if (data.hierarchy.top().first == json_syntax::string_value)
							return false;
						else if (data.raw_value.second != ljson::value_type::none &&
							 data.raw_value.second != ljson::value_type::unknown)
							return false;
						else if (data.raw_value.second != ljson::value_type::string &&
							 not data.raw_value.first.empty())
						{
							auto ok = end_statement::flush_value(data);
							if (not ok && ok.error().value() == error_type::parsing_error_wrong_type)
								return unexpected(error(error_type::parsing_error,
								    "reached an empty-space on non-string unknown type: " +
									data.raw_value.first));
							else
								return ok;
						}

						return true;
					}
			};

			struct key {
					static expected<bool, error> handle_key(struct parsing_data& data)
					{
						if (quotes::is_hierarchy_qoutes(data.hierarchy) && not array::is_array(data))
						{
							data.keys.top().first += data.line[data.i];
							return true;
						}

						return false;
					}
			};

			struct quotes {
					static expected<bool, error> handle_quotes(struct parsing_data& data)
					{
						bool found_qoute = false;
						if (quote(data))
						{
							if (data.hierarchy.top().first == json_syntax::quotes_1)
							{
								if (not data.keys.empty() && data.keys.top().first.empty())
									data.keys.top().second = key_type::simple_key;
								data.hierarchy.pop();
								return true;
							}

							found_qoute = true;
						}

						if (found_qoute)
						{
							if (not data.hierarchy.empty() &&
							    (data.hierarchy.top().first == json_syntax::column ||
								data.hierarchy.top().first == json_syntax::array))
							{
								data.hierarchy.push({json_syntax::string_value, data.line_number});
								data.raw_value.second = ljson::value_type::string;
							}
							else if (not data.hierarchy.empty() &&
								 data.hierarchy.top().first == json_syntax::string_value)
							{
								data.hierarchy.pop();
								return end_statement::flush_value(data);
							}
							else
								data.hierarchy.push({json_syntax::quotes_1, data.line_number});

							return true;
						}

						return false;
					}

					static bool quote(const struct parsing_data& data)
					{
						if (data.line[data.i] == '"' && data.raw_value.second != value_type::temp_escape_type)
							return true;
						else
							return false;
					}

					static bool is_hierarchy_qoutes(const std::stack<std::pair<json_syntax, size_t>>& hierarchy)
					{
						if (hierarchy.empty())
							return false;
						else if (hierarchy.top().first == json_syntax::quotes_1)
							return true;
						else
							return false;
					}

					static bool is_hierarchy_qoutes(const char ch)
					{
						if (ch == '"' || ch == '\'')
							return true;
						else
							return false;
					}
			};

			struct array {
					static expected<bool, error> handle_array(struct parsing_data& data)
					{
						if (data.hierarchy.empty() || end_statement::is_end_statement(data) ||
						    value::is_string(data))
							return false;

						if (data.line[data.i] == '[')
						{
							data.hierarchy.push({json_syntax::array, data.line_number});
							data.keys.top().second = key_type::array;

							auto ok = data.json_objs.top().add_array_to_key(data.keys.top().first);
							if (not ok)
								return unexpected(ok.error());
							data.json_objs.push(ok.value());

							if (end_statement::next_char_is_newline(data))
								data.line.pop_back();
							return true;
						}
						else if (is_array(data) && data.line[data.i] == ']')
						{
							data.hierarchy.pop();

							if (not data.json_objs.empty())
								data.json_objs.pop();

							data.hierarchy.push({json_syntax::maybe_empty_space_after, data.line_number});

							return true;
						}

						return false;
					}

					static bool is_array(const struct parsing_data& data)
					{
						if (not data.json_objs.empty() && data.json_objs.top().type() == node_type::array)
							return true;
						return false;
					}

					static bool is_array_char(const char ch)
					{
						if (ch == '[' || ch == ']')
							return true;
						return false;
					}

					static bool brackets_at_end_of_line(const struct parsing_data& data)
					{
						if (data.i == data.line.size() - 1 &&
						    (data.line[data.i] == '[' || data.line[data.i] == ']'))
							return true;
						return false;
					}
			};

			struct column {
					static expected<bool, error> handle_column(struct parsing_data& data)
					{
						if (data.line[data.i] != ':')
							return false;
						else if (column_in_quotes(data.hierarchy))
							return false;

						if (two_consecutive_columns(data.hierarchy))
						{
							return unexpected(error(error_type::parsing_error,
							    std::format("two consecutive columns at: {}, key: {}, val: {}, line: {}",
								data.line_number, data.keys.top().first, data.raw_value.first, data.line)));
						}
						else
						{
							data.hierarchy.push({json_syntax::column, data.line_number});
						}

						return true;
					}

					static bool column_in_quotes(const std::stack<std::pair<json_syntax, size_t>>& hierarchy)
					{
						if (hierarchy.empty())
							return false;
						else if (hierarchy.top().first == json_syntax::quotes_1 ||
							 hierarchy.top().first == json_syntax::string_value)
							return true;
						return false;
					}

					static bool two_consecutive_columns(const std::stack<std::pair<json_syntax, size_t>>& hierarchy)
					{
						if (not hierarchy.empty() && hierarchy.top().first == json_syntax::column)
							return true;
						return false;
					}
			};

			struct object {
					static expected<bool, error> handle_object(struct parsing_data& data)
					{
						if (data.hierarchy.empty() || end_statement::is_end_statement(data) ||
						    value::is_string(data))
							return false;

						if (data.line[data.i] == '{')
						{
							if (array::is_array(data))
							{
								auto ok = data.json_objs.top().add_object_to_array();
								if (not ok)
									return unexpected(ok.error());
								data.json_objs.push(ok.value());
							}
							else
							{
								auto ok = data.json_objs.top().add_object_to_key(data.keys.top().first);
								if (not ok)
									return unexpected(ok.error());
								data.json_objs.push(ok.value());
							}

							data.keys.push({"", key_type::simple_key});
							data.hierarchy.push({json_syntax::object, data.line_number});

							return true;
						}
						else if (is_object(data) && data.line[data.i] == '}')
						{
							data.hierarchy.pop();
							data.keys.pop();

							if (end_statement::end_of_line_after_bracket(data))
								data.hierarchy.pop();

							if (not data.json_objs.empty())
								data.json_objs.pop();

							if (not data.keys.empty())
								data.keys.top().second = key_type::none;
							data.hierarchy.push({json_syntax::maybe_empty_space_after, data.line_number});
							return true;
						}
						else if (array::is_array(data) && data.line[data.i] == '}')
						{
							data.hierarchy.pop();
							data.keys.pop();
							if (not data.json_objs.empty())
								data.json_objs.pop();
							data.hierarchy.push({json_syntax::maybe_empty_space_after, data.line_number});
							return true;
						}

						return false;
					}

					static bool is_object(const struct parsing_data& data)
					{
						if (data.json_objs.empty() || data.json_objs.size() == 1)
							return false;
						else if (data.json_objs.top().type() == node_type::object)
							return true;
						return false;
					}

					static bool is_object_char(const char ch)
					{
						if (ch == '{' || ch == '}')
							return true;
						return false;
					}

					static bool brackets_at_end_of_line(const struct parsing_data& data)
					{
						if (data.i == data.line.size() - 1 &&
						    (data.line[data.i] == '{' || data.line[data.i] == '}'))
							return true;
						return false;
					}
			};

			struct escape {
					static expected<bool, error> handle_escape_char(struct parsing_data& data)
					{
						if (data.raw_value.second != value_type::temp_escape_type)
							return false;

						if (not is_next_char_correct(data.line[data.i]))
						{
							std::string err = std::format("escape sequence is incorrect. expected [\", \\, "
										      "t, b, f, n, r, u, /] found: {}\nline: {}",
							    data.line[data.i], data.line);
							return unexpected(error(error_type::parsing_error, err));
						}

						return true;
					}

					static bool is_escape_char(const struct parsing_data& data)
					{
						if (data.raw_value.second == value_type::string && data.line[data.i] == '\\')
						{
							return true;
						}
						else
							return false;
					}

					static bool is_next_char_correct(const char ch)
					{
						std::unordered_set<char> chars = {'"', '\\', 't', 'b', 'f', 'n', 'r', 'u', '/'};
						// TODO: handle the unicode sequence
						if (auto itr = chars.find(ch); itr != chars.end())
							return true;
						else
							return false;
					}
			};

			struct value {
					static expected<bool, error> handle_value(struct parsing_data& data)
					{
						if (data.hierarchy.empty())
							return false;
						else if (is_not_value(data))
							return false;
						else if (object::is_object_char(data.line[data.i]) && not value::is_string(data))
							return false;
						else if (array::is_array_char(data.line[data.i]) && not value::is_string(data))
							return false;
						else if (end_statement::is_end_statement(data))
							return false;

						if (empty_value_in_non_string(data))
							return end_statement::flush_value(data);

						if (not array::is_array(data))
						{
							data.keys.top().second = key_type::simple_key;
						}

						if (escape::is_escape_char(data))
						{
							data.raw_value.second = value_type::temp_escape_type;
						}
						else if (auto ok = escape::handle_escape_char(data); (ok && ok.value()) || not ok)
						{
							if (not ok)
								return ok;
							data.raw_value.second = value_type::string;
						}
						else
						{
							if (data.hierarchy.top().first == json_syntax::string_value)
							{
								data.raw_value.second = value_type::string;
							}
						}

						data.raw_value.first += data.line[data.i];

						return true;
					}

					static bool empty_value_in_non_string(const struct parsing_data& data)
					{
						if (data.raw_value.first.empty())
							return false;
						else if (data.raw_value.second != ljson::value_type::string &&
							 (data.raw_value.first.back() == ' ' || data.raw_value.first.back() == '\t'))
							return true;
						else
							return false;
					}

					static bool is_string(const struct parsing_data& data)
					{
						if (data.hierarchy.empty())
							return false;
						else if (data.hierarchy.top().first == json_syntax::string_value)
							return true;
						else
							return false;
					}

					static bool is_not_value(const struct parsing_data& data)
					{
						if (not data.json_objs.empty() && data.json_objs.top().type() == node_type::array)
						{
							return false;
						}
						else if (data.hierarchy.top().first != json_syntax::column &&
							 data.hierarchy.top().first != json_syntax::string_value)
							return true;

						return false;
					}
			};

			struct end_statement {
					static bool next_char_is_newline(const struct parsing_data& data)
					{
						if (data.i == data.line.size() - 2 && data.line[data.i + 1] == '\n')
							return true;
						else
							return false;
					}

					static bool end_of_line_after_bracket(const struct parsing_data& data)
					{
						if (data.hierarchy.empty())
							return false;
						else if (data.hierarchy.top().first == json_syntax::column &&
							 data.i == data.line.size() - 1)
							return true;
						return false;
					}

					static expected<bool, error> flush_value(struct parsing_data& data)
					{
						data.hierarchy.push({json_syntax::flush_value, data.line_number});
						return handle_end_statement(data);
					}

					static bool pop_flush_element(const struct parsing_data& data)
					{
						if (not data.hierarchy.empty() && data.hierarchy.top().first == json_syntax::flush_value)
						{
							return true;
						}

						return false;
					}

					static bool empty_space_in_number(struct parsing_data& data)
					{
						bool found_empty = false;

						auto empty_char = [](char ch)
						{
							if (ch == ' ' || ch == '\t')
								return true;
							return false;
						};

						for (size_t& i = data.i; i < data.line.size(); i++)
						{
							if (empty_char(data.line[i]))
								found_empty = true;
							else if (data.line[i] == ',' || data.line[i] == '\n')
								return false;
							else if (found_empty && not empty_char(data.line[i]))
								return true;
						}

						return false;
					}

					static expected<bool, error> handle_end_statement(struct parsing_data& data)
					{
						if (not is_end_statement(data))
						{
							return false;
						}
						else if (not there_is_a_value(data))
							return false;

						if (next_char_is_newline(data))
							data.line.pop_back();

						if (pop_flush_element(data))
							data.hierarchy.pop();

						if (data.raw_value.first == "null")
						{
							data.raw_value.second = value_type::null;
						}
						else if (data.raw_value.first == "true" || data.raw_value.first == "false")
						{
							data.raw_value.second = value_type::boolean;
						}
						else if (is_num_decimal(data.raw_value.first))
						{
							if (data.raw_value.first.find('.') != data.raw_value.first.npos)
							{
								data.raw_value.second = value_type::double_t;
							}
							else
								data.raw_value.second = value_type::integer;
							if (empty_space_in_number(data))
							{
								return unexpected(error(error_type::parsing_error_wrong_type,
								    std::format("type error: '{}', in line: '{}'", data.raw_value.first,
									data.line)));
							}
						}
						else if (data.raw_value.second == ljson::value_type::none && data.raw_value.first.empty())
						{
							return true;
						}
						else if (data.raw_value.second != ljson::value_type::string)
						{
							data.raw_value.second = value_type::unknown;
							return unexpected(error(error_type::parsing_error_wrong_type,
							    std::format(
								"unknown type: '{}', in line: '{}'", data.raw_value.first, data.line)));
						}

						auto ok = data.value.set_value_type(data.raw_value.first, data.raw_value.second);
						if (not ok)
						{
							return ok.error();
						}

						data.raw_value.first.clear();
						data.raw_value.second = value_type::none;

						if (data.keys.top().second == key_type::simple_key)
						{
							auto ok = data.json_objs.top().insert(data.keys.top().first, data.value);
							if (not ok)
							{
								return unexpected(error(error_type::parsing_error,
								    std::format("{}", ljson::log("internal parsing error: [adding "
												 "value to simple_key]"))));
							}
						}
						else if (object::is_object(data))
						{
							auto ok = fill_object_data(data);
							if (not ok)
								return unexpected(ok.error());
						}
						else if (array::is_array(data))
						{
							auto ok = fill_array_data(data);
							if (not ok)
								return unexpected(ok.error());
							return true;
						}

						data.keys.top().first.clear();
						data.keys.top().second = key_type::none;
						data.hierarchy.pop();
						return true;
					}

					static bool is_end_statement(const struct parsing_data& data)
					{
						if (data.line[data.i] == ',' && not value::is_string(data))
							return true;
						else if (data.i == data.line.size() - 1 && not object::brackets_at_end_of_line(data) &&
							 not array::brackets_at_end_of_line(data) && not value::is_string(data))
							return true;
						else if (not data.hierarchy.empty() &&
							 data.hierarchy.top().first == json_syntax::flush_value)
						{
							return true;
						}
						else if (data.line[data.i] == '}' && not data.raw_value.first.empty())
						{
							return true;
						}
						else
							return false;
					}

					static bool there_is_a_value(const struct parsing_data& data)
					{
						if (data.hierarchy.empty())
							return false;
						else if (data.hierarchy.top().first == json_syntax::column)
							return true;
						else if (data.hierarchy.top().first == json_syntax::flush_value)
							return true;
						else if (array::is_array(data))
							return true;

						return false;
					}

					static expected<monostate, error> fill_object_data(struct parsing_data& data)
					{
						if (data.keys.top().second == key_type::simple_key)
						{
							auto ok = data.json_objs.top().insert(data.keys.top().first, data.value);
							if (not ok)
							{
								return unexpected(error(error_type::parsing_error,
								    "internal parsing error\n[adding value to object]"));
							}
						}

						return monostate();
					}

					static expected<monostate, error> fill_array_data(struct parsing_data& data)
					{
						auto ok = data.json_objs.top().push_back(data.value);
						if (not ok)
						{
							return unexpected(error(
							    error_type::parsing_error, "internal parsing error\n[adding value to array]"));
						}

						return monostate();
					}

					static bool run_till_end_of_statement(struct parsing_data& data)
					{
						if (data.hierarchy.empty())
							return false;
						else if (data.hierarchy.top().first != json_syntax::maybe_empty_space_after)
							return false;
						else if (data.line[data.i] == ',' || data.line[data.i] == '\n')
						{
							data.hierarchy.pop();
							if (not data.hierarchy.empty() && data.hierarchy.top().first == json_syntax::column)
								data.hierarchy.pop();

							if (not data.keys.empty())
							{
								data.keys.top().first.clear();
								data.keys.top().second = key_type::none;
							}
							return true;
						}
						else
							return false;
					}
			};

			struct closing_bracket {
					static expected<bool, error> handle_closing_bracket(struct parsing_data& data)
					{
						if (data.line[data.i] != '}' || value::is_string(data))
							return false;

						if (not data.hierarchy.empty() &&
						    data.hierarchy.top().first == json_syntax::opening_bracket)
							data.hierarchy.pop();
						else
						{
							if (not data.hierarchy.empty())
								return unexpected(error(error_type::parsing_error,
								    std::format("error at: {}, hierarchy.size: {}: line_num: {}, line: {}",
									data.line[data.i], data.hierarchy.size(),
									data.hierarchy.top().second, data.line)));
							else
								return unexpected(error(error_type::parsing_error,
								    std::format("extra closing bracket at line: {}", data.line_number)));
						}

						return true;
					}
			};

			struct syntax_error {
					static expected<bool, error> handle_syntax_error(struct parsing_data& data)
					{
						if (end_statement::is_end_statement(data))
							return false;
						return unexpected(
						    error(error_type::parsing_error, std::format("syntax error: line: '{}'\n[error]: {}",
											 data.line, expected_x_but_found_y(data))));
					}

					static std::string expected_x_but_found_y(const struct parsing_data& data)
					{
						if (data.hierarchy.empty())
							return std::format("expected '{{' but found '{}'", data.line[data.i]);

						switch (data.hierarchy.top().first)
						{
							case json_syntax::array:
								return std::format(
								    "expected 'array values' but found '{}'", data.line[data.i]);
							case json_syntax::object:
								return std::format(
								    "expected 'object key/value pairs' but found '{}'", data.line[data.i]);
							case json_syntax::opening_bracket:
								return std::format(
								    "expected [EOF, key, array, object] but found '{}'", data.line[data.i]);
							case json_syntax::column:
								return std::format("expected 'value' but found '{}'", data.line[data.i]);
							case json_syntax::string_value:
								return std::format(
								    "expected 'string value' but found '{}'", data.line[data.i]);
							case json_syntax::quotes_1:
								return std::format(
								    "expected [string value, quote] but found '{}'", data.line[data.i]);
							case json_syntax::quotes_2:
								return std::format(
								    "expected [string value, quote] but found '{}'", data.line[data.i]);
							case json_syntax::closing_bracket:
								return std::format("expected 'EOF' but found '{}'", data.line[data.i]);
							default:
								return std::format("unexpected syntax: found '{}'", data.line[data.i]);
						}
					}
			};
	};

	node::node() : _node(std::make_shared<ljson::object>())
	{
	}

	node::node(const class value& value) : _node(std::make_shared<class value>(value))
	{
	}

	node::node(enum node_type type)
	{
		if (type == node_type::value)
			_node = std::make_shared<class value>();
		else if (type == node_type::array)
			_node = std::make_shared<ljson::array>();
		else
			_node = std::make_shared<ljson::object>();
	}

	template<typename container_type_concept>
	node::node(const container_type_concept& container)
	{
		if constexpr (is_value_container<container_type_concept>)
		{
			_node = std::make_shared<ljson::array>();

			for (auto& val : container)
			{
				std::variant<class value, ljson::node> v = this->handle_allowed_node_types(val);

				if (std::holds_alternative<class value>(v))
				{
					this->push_back(node(std::get<class value>(v)));
				}
				else
				{
					this->push_back(std::get<ljson::node>(v));
				}
			}
		}
		else if constexpr (is_key_value_container<container_type_concept>)
		{
			_node = std::make_shared<ljson::object>();

			for (auto& [key, val] : container)
			{
				std::variant<class value, ljson::node> v = this->handle_allowed_node_types(val);

				if (std::holds_alternative<class value>(v))
				{
					this->insert(key, node(std::get<class value>(v)));
				}
				else
				{
					this->insert(key, std::get<ljson::node>(v));
				}
			}
		}
		else
		{
			static_assert(false && "unsupported type is in the ljson::node constructor");
		}
	}

	template<typename is_allowed_node_type>
	std::variant<class value, ljson::node> node::handle_allowed_node_types(const is_allowed_node_type& value)
	{
		if constexpr (std::is_same<is_allowed_node_type, class value>::value)
		{
			return value;
		}
		else if constexpr (std::is_same<is_allowed_node_type, bool>::value)
		{
			return ljson::value(value);
		}
		else if constexpr (std::is_arithmetic<is_allowed_node_type>::value)
		{
			if constexpr (std::is_floating_point_v<is_allowed_node_type>)
				return ljson::value(value);
			else
				return ljson::value(value);
		}
		else if constexpr (std::is_same<is_allowed_node_type, std::string>::value)
		{
			return ljson::value(value);
		}
		else if constexpr (std::is_same<is_allowed_node_type, const char*>::value)
		{
			return ljson::value(value);
		}
		else if constexpr (std::is_same<is_allowed_node_type, null_type>::value)
		{
			return ljson::value(ljson::null);
		}
		else if constexpr (std::is_same<is_allowed_node_type, ljson::node>::value)
		{
			return value;
		}
		else
		{
			static_assert(false && "unsupported value_type in function node::handle_allowed_node_types(...)");
		}
	}

	void node::handle_std_any(const std::any& any_value, std::function<void(std::any)> insert_func)
	{
		if (any_value.type() == typeid(ljson::node))
		{
			auto val = std::any_cast<ljson::node>(any_value);
			insert_func(val);
		}
		else
		{
			class value value;
			if (any_value.type() == typeid(bool))
			{
				auto val = std::any_cast<bool>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(double))
			{
				auto val = std::any_cast<double>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(int))
			{
				auto val = std::any_cast<int>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(float))
			{
				auto val = std::any_cast<float>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(const char*))
			{
				auto val = std::any_cast<const char*>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(std::string))
			{
				auto val = std::any_cast<std::string>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(ljson::null_type))
			{
				value.set_value_type(ljson::null);
				insert_func(value);
			}
			else
			{
				throw error(error_type::wrong_type,
				    std::string("unknown type given to ljson::node constructor: ") + any_value.type().name());
			}
		}
	}

	node::node(const std::initializer_list<std::pair<std::string, std::any>>& pairs) : _node(std::make_shared<ljson::object>())
	{
		std::string key;
		auto	    map = this->as_object();

		auto insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(ljson::node))
			{
				auto val = std::any_cast<ljson::node>(value);
				map->insert(key, val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				map->insert(key, ljson::node(val));
			}
		};

		for (const auto& pair : pairs)
		{
			key = pair.first;
			this->handle_std_any(pair.second, insert_func);
			key.clear();
		}
	}

	node::node(const std::initializer_list<std::any>& val) : _node(std::make_shared<ljson::array>())
	{
		auto vector = this->as_array();

		auto insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(ljson::node))
			{
				auto val = std::any_cast<ljson::node>(value);
				vector->push_back(val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				vector->push_back(ljson::node(val));
			}
		};

		for (const auto& value : val)
		{
			this->handle_std_any(value, insert_func);
		}
	}

	expected<class ljson::node, error> node::add_array_to_key(const std::string& key)
	{
		if (not this->is_object())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add array to an object node"));

		auto obj = this->as_object();
		return obj->insert(key, ljson::node(node_type::array));
	}

	expected<class ljson::node, error> node::add_object_to_array()
	{
		if (not this->is_array())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add object to an array node"));

		auto arr = this->as_array();
		arr->push_back(ljson::node(node_type::object));
		return arr->back();
	}

	expected<class ljson::node, error> node::add_node_to_array(const ljson::node& node)
	{
		if (not this->is_array())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add node to an array node"));

		auto arr = this->as_array();
		arr->push_back(node);

		return arr->back();
	}

	expected<class ljson::node, error> node::add_node_to_array(const size_t index, const ljson::node& node)
	{
		if (not this->is_array())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add node to an array node"));

		auto arr = this->as_array();
		if (index >= arr->size())
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to add node to an array node at an out-of-band index"));

		(*arr)[index] = node;

		return (*arr)[index];
	}

	expected<class ljson::node, error> node::add_object_to_key(const std::string& key)
	{
		if (not this->is_object())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add object to an object node"));

		auto obj = this->as_object();
		return obj->insert(key, ljson::node(node_type::object));
	}

	expected<class ljson::node, error> node::add_node_to_key(const std::string& key, const ljson::node& node)
	{
		if (not this->is_object())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add node to an object node"));

		auto obj = this->as_object();
		return obj->insert(key, node);
	}

	template<typename node_type_concept>
	expected<class ljson::node, error> node::insert(const std::string& key, const node_type_concept& value)
	{
		if constexpr (is_allowed_node_type<node_type_concept>)
		{
			std::variant<class value, ljson::node> v = this->handle_allowed_node_types(value);

			if (std::holds_alternative<class value>(v))
			{
				return this->add_value_to_key(key, std::get<class value>(v));
			}
			else if (std::holds_alternative<ljson::node>(v))
			{
				return this->add_node_to_key(key, std::get<ljson::node>(v));
			}
			else
			{
				return unexpected(error(error_type::wrong_type, "wrong type: trying to insert to an object node"));
			}
		}
		else if constexpr (container_type_concept<node_type_concept>)
		{
			ljson::node n(value);
			return this->add_node_to_key(key, n);
		}
		else
		{
			static_assert(false && "unsupported type is inserted into an object node");
		}
	}

	template<typename node_type_concept>
	expected<class ljson::node, error> node::push_back(const node_type_concept& value)
	{
		if constexpr (is_allowed_node_type<node_type_concept>)
		{
			std::variant<class value, ljson::node> v = this->handle_allowed_node_types(value);

			if (std::holds_alternative<class value>(v))
			{
				return this->add_value_to_array(std::get<class value>(v));
			}
			else if (std::holds_alternative<ljson::node>(v))
			{
				return this->add_node_to_array(std::get<ljson::node>(v));
			}
			else
			{
				return unexpected(error(error_type::wrong_type, "wrong type: trying to push_back to an array node"));
			}
		}
		else if constexpr (container_type_concept<node_type_concept>)
		{
			ljson::node n(value);
			return this->add_node_to_array(n);
		}
		else
		{
			static_assert(false && "unsupported type is pushed_back into an object node");
		}
	}

	expected<class ljson::node, error> node::add_value_to_key(const std::string& key, const class value& value)
	{
		if (not this->is_object())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add value to an object node"));

		auto obj = this->as_object();
		return obj->insert(key, ljson::node(value));
	}

	expected<class ljson::node, error> node::add_value_to_array(const class value& value)
	{
		if (not this->is_array())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add value to an array node"));

		auto arr = this->as_array();
		arr->push_back(ljson::node(value));
		return arr->back();
	}

	expected<class ljson::node, error> node::add_value_to_array(const size_t index, const class value& value)
	{
		if (not this->is_array())
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add value to an array node"));

		auto arr = this->as_array();
		if (index >= arr->size())
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to add value to an array node at an out-of-band index"));

		(*arr)[index] = ljson::node(value);

		return (*arr)[index];
	}

	std::shared_ptr<class value> node::as_value() const
	{
		if (not this->is_value())
			throw error(error_type::wrong_type, "wrong type: trying to cast a non-value node to a value");

		return std::get<std::shared_ptr<class value>>(_node);
	}

	std::shared_ptr<ljson::array> node::as_array() const
	{
		if (not this->is_array())
			throw error(error_type::wrong_type, "wrong type: trying to cast a non-array node to an array");

		return std::get<std::shared_ptr<ljson::array>>(_node);
	}

	std::shared_ptr<ljson::object> node::as_object() const
	{
		if (not this->is_object())
			throw error(error_type::wrong_type, "wrong type: trying to cast a non-object node to an object");

		return std::get<std::shared_ptr<ljson::object>>(_node);
	}

	bool node::is_value() const
	{
		if (std::holds_alternative<std::shared_ptr<class value>>(_node))
			return true;
		else
			return false;
	}

	bool node::is_array() const
	{
		if (std::holds_alternative<std::shared_ptr<ljson::array>>(_node))
			return true;
		else
			return false;
	}

	bool node::is_object() const
	{
		if (std::holds_alternative<std::shared_ptr<ljson::object>>(_node))
			return true;
		else
			return false;
	}

	node_type node::type() const
	{
		if (this->is_value())
			return node_type::value;
		else if (this->is_array())
			return node_type::array;
		else
			return node_type::object;
	}

	bool node::contains(const std::string& key) const
	{
		if (not this->is_object())
			return false;

		auto obj = this->as_object();
		auto itr = obj->find(key);

		if (itr != obj->end())
			return true;
		else
			return false;
	}

	class node& node::at(const std::string& object_key) const
	{
		auto obj = this->as_object();
		auto itr = obj->find(object_key);
		if (itr == obj->end())
			throw error(error_type::key_not_found, std::format("key: '{}' not found", object_key));
		return itr->second;
	}

	class node& node::at(const size_t array_index) const
	{
		auto arr = this->as_array();
		if (array_index >= arr->size())
			throw error(error_type::key_not_found, std::format("index: '{}' not found", array_index));

		return arr->at(array_index);
	}

	class node& node::operator=(const std::shared_ptr<class value>& val)
	{
		assert(val != nullptr);
		if (this->is_value())
		{
			auto n = this->as_value();
			*n     = *val;
		}
		else
		{
			_node = val;
		}
		return *this;
	}

	class node& node::operator=(const class value& val)
	{
		if (this->is_value())
		{
			auto n = this->as_value();
			*n     = val;
		}
		else
		{
			_node = std::make_shared<class value>(val);
		}
		return *this;
	}

	class node& node::operator=(const std::shared_ptr<ljson::array>& arr)
	{
		_node = arr;
		return *this;
	}

	class node& node::operator=(const std::shared_ptr<ljson::object>& obj)
	{
		_node = obj;
		return *this;
	}

	class node& node::operator=(const std::string& val)
	{
		_node = std::make_shared<class value>(val);
		return *this;
	}

	class node& node::operator=(const char* val)
	{
		assert(val != NULL);
		*this = std::string(val);
		return *this;
	}

	class node& node::operator=(const null_type)
	{
		_node = std::make_shared<class value>(ljson::null);
		return *this;
	}

	template<typename number_type>
	class node& node::operator=(const number_type& val)
	{
		static_assert(std::is_arithmetic<number_type>::value, "Template paramenter must be a numeric type");

		_node = std::make_shared<class value>(val);
		return *this;
	}

	class node& node::operator=(const bool val)
	{
		_node = std::make_shared<class value>(val);
		return *this;
	}

	class node& node::operator+=(const std::initializer_list<std::pair<std::string, std::any>>& pairs)
	{
		if (not this->is_object())
			throw error(error_type::wrong_type, "wrong type: trying to insert pairs to a non-object");

		std::string key;
		auto	    map = this->as_object();

		auto insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(ljson::node))
			{
				auto val = std::any_cast<ljson::node>(value);
				map->insert(key, val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				map->insert(key, ljson::node(val));
			}
		};

		for (const auto& pair : pairs)
		{
			key = pair.first;
			this->handle_std_any(pair.second, insert_func);
			key.clear();
		}
		return *this;
	}

	class node& node::operator+=(const std::initializer_list<std::any>& val)
	{
		if (not this->is_array())
			throw error(error_type::wrong_type, "wrong type: trying to insert pairs to a non-array");

		auto vector = this->as_array();

		auto insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(ljson::node))
			{
				auto val = std::any_cast<ljson::node>(value);
				vector->push_back(val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				vector->push_back(ljson::node(val));
			}
		};

		for (const auto& value : val)
		{
			this->handle_std_any(value, insert_func);
		}
		return *this;
	}

	class node node::operator+(const node& other_node)
	{
		if (this->type() != other_node.type())
			throw error(error_type::wrong_type, "trying to + two nodes with different types");

		if (this->is_object())
		{
			ljson::node new_node(ljson::node_type::object);
			for (const auto& [key, node] : *this->as_object())
			{
				new_node.add_node_to_key(key, node);
			}
			for (const auto& [key, node] : *other_node.as_object())
			{
				new_node.add_node_to_key(key, node);
			}

			return new_node;
		}
		else if (this->is_array())
		{
			ljson::node new_node(ljson::node_type::array);
			for (const auto& node : *this->as_array())
			{
				new_node.add_node_to_array(node);
			}
			for (const auto& node : *other_node.as_array())
			{
				new_node.add_node_to_array(node);
			}

			return new_node;
		}
		else
		{
			ljson::node new_node(this->type());
			auto	    val = this->as_value();

			if (val->type() == value_type::string)
			{
				new_node = this->as_value()->as_string() + other_node.as_value()->as_string();
			}
			else if (val->type() == value_type::double_t || val->type() == value_type::integer)
			{
				new_node = this->as_value()->as_number() + other_node.as_value()->as_number();
			}
			else
			{
				throw error(
				    error_type::wrong_type, "trying to + two nodes with values that are neither a number nor string");
			}

			return new_node;
		}
	}

	expected<monostate, error> node::set(const class value& value)
	{
		if (not this->is_value())
			return unexpected(error(error_type::wrong_type, "wrong type: can't set value to non-value node-class"));

		auto val = this->as_value();

		*val = value;

		return monostate();
	}

	expected<monostate, error> node::set(const std::string& value)
	{
		class value new_value(value);

		this->set(new_value);

		return monostate();
	}

	template<typename number_type>
	expected<monostate, error> node::set(const number_type value)
	{
		static_assert(std::is_arithmetic_v<number_type>, "Template paramenter must be a numeric type");
		class value new_value(value);
		this->set(new_value);

		return monostate();
	}

	expected<monostate, error> node::set(const bool value)
	{
		class value new_value(value);

		this->set(new_value);

		return monostate();
	}

	expected<monostate, error> node::set(const char* value)
	{
		std::string str = value;
		this->set(str);
		return monostate();
	}

	expected<monostate, error> node::set(const ljson::null_type)
	{
		class value new_value(ljson::null);

		this->set(new_value);

		return monostate();
	}

	void node::dump(const std::function<void(std::string)> out_func, const std::pair<char, int>& indent_conf, int indent) const
	{
		using node_or_value = std::variant<std::shared_ptr<class value>, ljson::node>;

		auto dump_val = [&indent, &out_func, &indent_conf](const node_or_value& value_or_nclass)
		{
			if (std::holds_alternative<std::shared_ptr<class value>>(value_or_nclass))
			{
				auto val = std::get<std::shared_ptr<class value>>(value_or_nclass);
				assert(val != nullptr);
				if (val->type() == ljson::value_type::string)
					out_func(std::format("\"{}\"", val->stringify()));
				else
					out_func(std::format("{}", val->stringify()));
			}
			else
			{
				std::get<ljson::node>(value_or_nclass).dump(out_func, indent_conf, indent + indent_conf.second);
			}
		};

		if (this->is_object())
		{
			out_func(std::format("{{\n"));
			auto   map   = this->as_object();
			size_t count = 0;
			for (const auto& pair : *map)
			{
				out_func(
				    std::format("{}\"{}\": ", std::string(indent + indent_conf.second, indent_conf.first), pair.first));
				dump_val(pair.second);

				if (++count != map->size())
					out_func(std::format(","));

				out_func(std::format("\n"));
			}
			out_func(std::format("{}}}", std::string(indent, indent_conf.first)));
		}
		else if (this->is_array())
		{
			out_func(std::format("[\n"));
			auto   vector = this->as_array();
			size_t count  = 0;
			for (const auto& array_value : *vector)
			{
				out_func(std::format("{}", std::string(indent + indent_conf.second, indent_conf.first)));
				dump_val(array_value);

				if (++count != vector->size())
					out_func(std::format(","));

				out_func(std::format("\n"));
			}
			out_func(std::format("{}]", std::string(indent, indent_conf.first)));
		}
		else if (this->is_value())
		{
			auto val = this->as_value();
			dump_val(val);
		}
	}

	void node::dump_to_stdout(const std::pair<char, int>& indent_conf)
	{
		auto func = [](const std::string& output) { std::cout << output; };
		this->dump(func, indent_conf);
	}

	std::string node::dump_to_string(const std::pair<char, int>& indent_conf)
	{
		std::string data;
		auto	    func = [&data](const std::string& output) { data += output; };
		this->dump(func, indent_conf);

		return data;
	}

	expected<monostate, error> node::write_to_file(const std::filesystem::path& path, const std::pair<char, int>& indent_conf)
	{
		std::ofstream file(path);
		if (not file.is_open())
			return unexpected(error(error_type::filesystem_error, std::strerror(errno)));

		auto func = [&file](const std::string& output) { file << output; };
		this->dump(func, indent_conf);
		file.close();

		return monostate();
	}

	parser::parser()
	{
	}

	bool parser::done_or_not_ok(const expected<bool, error>& ok)
	{
		if ((ok && ok.value()) || not ok)
			return true;
		else
			return false;
	}

	void parser::throw_error_if_not_ok(const expected<bool, error>& ok)
	{
		if (not ok)
			throw ok.error();
	}

	void parser::check_unhandled_hierarchy(const std::string& raw_json, struct parsing_data& data)
	{
		if (data.hierarchy.empty() || raw_json.empty())
			return;

		if (raw_json.back() == '}' && data.hierarchy.top().first == json_syntax::opening_bracket)
		{
			data.line = "}";
			data.i	  = 0;
			assert(data.i < data.line.size());
			if (auto ok = parser_syntax::closing_bracket::handle_closing_bracket(data); done_or_not_ok(ok))
			{
				throw_error_if_not_ok(ok);
			}
		}
		else
		{
			data.line = (not raw_json.empty()) ? std::string(raw_json.back(), 1) : "";
			data.i	  = std::min(raw_json.size() - 1, ( size_t ) 0);
			assert(data.i < data.line.size());
			if (auto ok = parser_syntax::syntax_error::handle_syntax_error(data); done_or_not_ok(ok))
			{
				throw_error_if_not_ok(ok);
			}
		}
	}

	void parser::parsing(struct parsing_data& data)
	{
		expected<bool, error> ok;

		if (parser_syntax::end_statement::run_till_end_of_statement(data))
			return;

		if (ok = parser_syntax::empty::handle_empty(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::quotes::handle_quotes(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::key::handle_key(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::column::handle_column(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::value::handle_value(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::object::handle_object(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::array::handle_array(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::end_statement::handle_end_statement(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::open_bracket::handle_open_bracket(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::closing_bracket::handle_closing_bracket(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::syntax_error::handle_syntax_error(data); done_or_not_ok(ok))
		{
			throw_error_if_not_ok(ok);
		}
	}

	ljson::node parser::parse(const std::filesystem::path& path)
	{
		ljson::node json_data = ljson::node(node_type::object);

		std::unique_ptr<std::ifstream> file = std::make_unique<std::ifstream>(path);
		if (not file->is_open())
			throw ljson::error(
			    error_type::filesystem_error, std::format("couldn't open '{}', {}", path.string(), std::strerror(errno)));
		struct parsing_data data;

		data.json_objs.push(json_data);
		data.keys.push({"", key_type::simple_key});

		while (std::getline(*file, data.line))
		{
			data.line += "\n";
			for (data.i = 0; data.i < data.line.size(); data.i++)
			{
				this->parsing(data);
			}

			if (not file->eof())
				data.line.clear();

			data.line_number++;
		}

		this->check_unhandled_hierarchy(data.line, data);

		return json_data;
	}

	ljson::node parser::parse(const char* raw_json)
	{
		assert(raw_json != NULL);
		std::string string_json(raw_json);
		return this->parse(string_json);
	}

	ljson::node parser::parse(const std::string& raw_json)
	{
		ljson::node json_data = ljson::node(node_type::object);

		struct parsing_data data;

		data.json_objs.push(json_data);
		data.keys.push({"", key_type::simple_key});

		for (size_t i = 0; i < raw_json.size(); i++)
		{
			data.line += raw_json[i];

			if (not data.line.empty() && (data.line.back() == '\n' || data.line.back() == ',' || data.line.back() == '}'))
			{

				for (data.i = 0; data.i < data.line.size(); data.i++)
				{
					this->parsing(data);
				}
				data.line.clear();

				data.line_number++;
			}
		}

		this->check_unhandled_hierarchy(raw_json, data);

		return json_data;
	}

	parser::~parser()
	{
	}

	error::error(error_type err, const std::string& message) : err_type(err), msg(message)
	{
	}

	const char* error::what() const noexcept
	{
		return msg.c_str();
	}

	const std::string& error::message() const noexcept
	{
		return msg;
	}

	error_type error::value()
	{
		return err_type;
	}
}

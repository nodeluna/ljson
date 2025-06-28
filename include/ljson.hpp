/*
 * Copyright: 2025 nodeluna
 * SPDX-License-Identifier: Apache-2.0
 * repository: https://github.com/nodeluna/ljson
 */

#pragma once

#include <any>
#include <cstdint>
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


/**
 * @brief the namespace for ljson
 */
namespace ljson {

	/**
	 * @struct monostate
	 * @brief an implementation of std::monostate to allow using ljson with C++20. check the cppreference
	 */
	struct monostate {};

	/**
	 * @class expected
	 * @brief an implementation of std::expected to allow using ljson with C++20. check the cppreference
	 */
	template<class T = monostate, class E = monostate>
	class expected {
		private:
			bool _has_value = false;

			union {
					T _value;
					E _error;
			};

		public:
			constexpr expected(const T& t) : _has_value(true)
			{
				std::construct_at(std::addressof(_value), T(t));
			}

			constexpr expected(const E& e) : _has_value(false)
			{
				std::construct_at(std::addressof(_error), E(e));
			}

			constexpr expected() : _has_value(true)
			{
				static_assert(std::is_default_constructible<T>::value, "");
				std::construct_at(std::addressof(_value), T());
			}

			constexpr expected& operator=(const expected& other)
			{
				static_assert(std::is_copy_assignable<T>::value && std::is_copy_assignable<E>::value, "");
				if (this != &other)
				{
					this->~expected();
					std::construct_at(this, expected(other));
				}

				return *this;
			}

			template<class U>
			constexpr expected(const expected<U, E>& other) : _has_value(other.has_value())
			{
				static_assert(std::is_same<U, monostate>::value, "no available conversion between the provided value types");
				static_assert(std::is_copy_constructible<T>::value && std::is_copy_constructible<E>::value, "");
				if (_has_value)
				{
					std::construct_at(std::addressof(_value), T());
				}
				else
				{
					std::construct_at(std::addressof(_error), E(other.error()));
				}
			}

			constexpr expected(const expected& other) : _has_value(other.has_value())
			{
				static_assert(std::is_copy_constructible<T>::value && std::is_copy_constructible<E>::value, "");
				if (_has_value)
				{
					std::construct_at(std::addressof(_value), T(other.value()));
				}
				else
				{
					std::construct_at(std::addressof(_error), E(other.error()));
				}
			}

			constexpr expected(const expected&& other) noexcept : _has_value(other._has_value)
			{
				static_assert(std::is_move_constructible<T>::value && std::is_move_constructible<E>::value, "");
				if (this->has_value())
				{
					std::construct_at(std::addressof(_value), T(std::move(other._value)));
				}
				else
				{
					std::construct_at(std::addressof(_error), E(std::move(other._error)));
				}
			}

			constexpr expected& operator=(const expected&& other) noexcept
			{
				static_assert(std::is_move_assignable<T>::value && std::is_move_assignable<E>::value, "");
				if (this != &other)
				{
					this->~expected();
					std::construct_at(this, expected(std::move(other)));
				}

				return *this;
			}
			
			constexpr ~expected() 
			{
				if (this->has_value())
					_value.~T();
				else
					_error.~E();
			}

			constexpr bool has_value() const noexcept
			{
				return _has_value;
			}

			constexpr explicit operator bool() const noexcept
			{
				return this->has_value();
			}

			constexpr const T& value() const&
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return _value;
			}

			constexpr const E& error() const&
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return _error;
			}

			constexpr T& value() &
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return _value;
			}

			constexpr E& error() &
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return _error;
			}

			constexpr const T&& value() const&&
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return std::move(_value);
			}

			constexpr const E&& error() const&&
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return std::move(_error);
			}

			constexpr T&& value() &&
			{
				if (not _has_value)
				{
					throw std::runtime_error("Attempted to access the value of a error state");
				}
				return std::move(_value);
			}

			constexpr E&& error() &&
			{
				if (_has_value)
				{
					throw std::runtime_error("Attempted to access the error of a value state");
				}
				return std::move(_error);
			}

			template<class U = typename std::remove_cv<T>::type>
			constexpr T value_or(U&& other) const&
			{
				static_assert(std::is_convertible<U, T>::value, "the provided type must be convertible to the value type");
				if (_has_value)
					return _value;
				else
					return static_cast<T>(std::forward<U>(other));
			}

			template<class U = typename std::remove_cv<E>::type>
			constexpr E error_or(U&& other) const&
			{
				static_assert(std::is_convertible<U, E>::value, "the provided type must be convertible to the error type");
				if (not _has_value)
					return _error;
				else
					return static_cast<E>(std::forward<U>(other));
			}
	};

	/**
	 * @brief a helper function to construct an unexpected type
	 */
	template<class E>
	constexpr expected<monostate, E> unexpected(const E& e)
	{
		return expected<monostate, E>(e);
	}

	/**
	 * @brief a helper function to construct an unexpected type
	 * @detail @cpp
	 * ljson::expected<ljson::node, std::string> node = ljson::unexpected("error");
	 * @ecpp
	 */
	constexpr expected<monostate, std::string> unexpected(const char* e)
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

	/**
	 * @class error
	 * @brief a class type inspired by std::error_code to handle errors
	 */
	class error {
		private:
			error_type  err_type;
			std::string msg;

		public:
			/**
			 * @brief constructor
			 * @param err holds the value of ljson::error_type
			 * @param message the string message of the error
			 */
			error(error_type err, const std::string& message);

			/**
			 * @brief constructor
			 * @param err holds the value of ljson::error_type
			 * @param fmt std::format() type
			 * @param args the arguments for std::format()
			 * @tparam args_t the types of arguments for std::format()
			 */
			template<typename... args_t>
			error(error_type err, std::format_string<args_t...> fmt, args_t&&... args);

			/**
			 * @brief get the string message of the error
			 * @return get the string message of the error
			 */
			const char* what() const noexcept;

			/**
			 * @brief get the string message of the error
			 * @return get the string message of the error
			 */
			const std::string& message() const noexcept;

			/**
			 * @brief get the ljson::error_type of the error
			 * @return get the ljson::error_type of the error
			 */
			error_type value();
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

	/**
	 * @enum node_type
	 * @brief this enum can be used to explicitly make a node that's either an object, array or value
	 * @detail @cpp
	 * ljson::node node(ljson::node_type::array);
	 * ljson::node node; // default is ljson::node_type::object
	 * @ecpp
	 */
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

	/**
	 * @class null_type
	 * @brief an empty class to represent a json null value
	 */
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

	/**
	  @brief puts a constraint on the allowed json types for ljson::value
	 */
	template<typename allowed_value_types>
	concept is_allowed_value_type = std::is_same_v<allowed_value_types, std::string> ||
					std::is_same_v<allowed_value_types, const char*> || std::is_arithmetic_v<allowed_value_types> ||
					std::is_same_v<allowed_value_types, null_type> || std::is_same_v<allowed_value_types, bool>;

	/**
	 * @class value
	 * @brief holds a json value such as <std::string, double, int64_t, bool, null_type, monostate>
	 */
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
				switch (t)
				{
					case value_type::double_t:
					case value_type::number:
						_value = std::stod(val);
						break;
					case value_type::integer:
						_value = std::stoll(val);
						break;
					case value_type::string:
						_value = val;
						break;
					case value_type::boolean:
						_value = (val == "true" ? true : false);
						break;
					case value_type::null:
						_value = null_type();
						break;
					case value_type::none:
						_value = monostate();
						break;
					case value_type::unknown:
					case value_type::temp_escape_type:
					default:
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

			expected<std::string, error> try_as_string() noexcept
			{
				if (not this->is_string())
					return unexpected(error(error_type::wrong_type,
					    "wrong type: trying to cast the value '{}' which is a '{}' to 'string'", this->stringify(),
					    this->type_name()));

				return std::get<std::string>(_value);
			}

			expected<double, error> try_as_number() noexcept
			{
				if (not this->is_number())
					return unexpected(error(error_type::wrong_type,
					    "wrong type: trying to cast the value '{}' which is a '{}' to 'number'", this->stringify(),
					    this->type_name()));

				if (this->is_double())
					return std::get<double>(_value);
				else
					return std::get<int64_t>(_value);
			}

			expected<int64_t, error> try_as_integer() noexcept
			{
				if (not this->is_integer())
					return unexpected(error(error_type::wrong_type,
					    "wrong type: trying to cast the value '{}' which is a '{}' to 'integer'", this->stringify(),
					    this->type_name()));

				return std::get<int64_t>(_value);
			}

			expected<double, error> try_as_double() noexcept
			{
				if (not this->is_double())
					return unexpected(error(error_type::wrong_type,
					    "wrong type: trying to cast the value '{}' which is a '{}' to 'double'", this->stringify(),
					    this->type_name()));

				return std::get<double>(_value);
			}

			expected<bool, error> try_as_boolean() noexcept
			{
				if (not this->is_boolean())
					return unexpected(error(error_type::wrong_type,
					    "wrong type: trying to cast the value '{}' which is a '{}' to 'boolean'", this->stringify(),
					    this->type_name()));

				return std::get<bool>(_value);
			}

			expected<null_type, error> try_as_null() noexcept
			{
				if (not this->is_null())
					return unexpected(error(error_type::wrong_type,
					    "wrong type: trying to cast the value '{}' which is a '{}' to 'null'", this->stringify(),
					    this->type_name()));

				return std::get<null_type>(_value);
			}

			std::string as_string()
			{
				auto ok = this->try_as_string();
				if (not ok)
					throw ok.error();

				return ok.value();
			}

			double as_number()
			{
				auto ok = this->try_as_number();
				if (not ok)
					throw ok.error();

				return ok.value();
			}

			int64_t as_integer()
			{
				auto ok = this->try_as_integer();
				if (not ok)
					throw ok.error();

				return ok.value();
			}

			double as_double()
			{
				auto ok = this->try_as_double();
				if (not ok)
					throw ok.error();

				return ok.value();
			}

			bool as_boolean()
			{
				auto ok = this->try_as_boolean();
				if (not ok)
					throw ok.error();

				return ok.value();
			}

			null_type as_null()
			{
				auto ok = this->try_as_null();
				if (not ok)
					throw ok.error();

				return ok.value();
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
				       std::is_same_v<allowed_node_types, bool> || std::is_same_v<allowed_node_types, class value> ||
				       std::is_same_v<allowed_node_types, ljson::node>;

	/**
	 * @brief concept for enabling ljson::node to accept std key/value containers
	 */
	template<typename container_type>
	concept is_key_value_container = requires(container_type container) {
		typename container_type::key_type;
		typename container_type::mapped_type;
		{
			container.begin()
		} -> std::same_as<typename container_type::iterator>;
		{
			container.end()
		} -> std::same_as<typename container_type::iterator>;
	} && std::is_same_v<typename container_type::key_type, std::string> && is_allowed_node_type<typename container_type::mapped_type>;

	template<typename string_type>
	concept is_string_type = std::is_same_v<string_type, std::string> || std::is_same_v<string_type, std::wstring>;

	/**
	 * @brief concept for enabling ljson::node to accept std array-like containers
	 */
	template<typename container_type>
	concept is_value_container =
	    requires(container_type container) {
		    typename container_type::value_type;
		    {
			    container.begin()
		    } -> std::same_as<typename container_type::iterator>;
		    {
			    container.end()
		    } -> std::same_as<typename container_type::iterator>;
	    } && not is_key_value_container<container_type> && is_allowed_node_type<typename container_type::value_type> &&
	    not is_string_type<container_type>;

	/**
	 * @brief puts a constraint on the allowed std container types to be inserted into ljson::node
	 */
	template<typename container_type>
	concept container_type_concept = is_key_value_container<container_type> || is_value_container<container_type>;

	/**
	 * @brief puts a constraint on the allowed types to be inserted into ljson::node
	 */
	template<typename value_type>
	concept container_or_node_type = container_type_concept<value_type> || is_allowed_node_type<value_type>;

	using object_pairs = std::initializer_list<std::pair<std::string, std::any>>;
	using array_values = std::initializer_list<std::any>;

	using json_object = std::map<std::string, class node>;
	using json_array  = std::vector<class node>;
	using json_node	  = std::variant<std::shared_ptr<class value>, std::shared_ptr<ljson::array>, std::shared_ptr<ljson::object>>;

	/**
	 * @class node
	 * @brief the class that holds a json node which is either ljson::object, ljson::array or ljson::value
	 */
	class node {
		private:
			json_node _node;

		protected:
			void handle_std_any(const std::any& any_value, std::function<void(std::any)> insert_func);

			template<typename is_allowed_node_type>
			constexpr std::variant<class value, ljson::node> handle_allowed_node_types(
			    const is_allowed_node_type& value) noexcept;

			template<typename container_or_node_type>
			constexpr void setting_allowed_node_type(const container_or_node_type& node_value) noexcept;

		public:
			explicit node();
			explicit node(const json_node& n);
			explicit node(enum node_type type);

			template<typename container_or_node_type>
			explicit node(const container_or_node_type& container) noexcept;

			node(const std::initializer_list<std::pair<std::string, std::any>>& pairs);
			node(const std::initializer_list<std::any>& val);

			template<typename container_or_node_type>
			expected<class ljson::node, error> insert(const std::string& key, const container_or_node_type& node);

			template<typename container_or_node_type>
			expected<class ljson::node, error> push_back(const container_or_node_type& node);

			expected<std::shared_ptr<class value>, error>	try_as_value() const noexcept;
			expected<std::shared_ptr<ljson::array>, error>	try_as_array() const noexcept;
			expected<std::shared_ptr<ljson::object>, error> try_as_object() const noexcept;
			std::shared_ptr<class value>			as_value() const;
			std::shared_ptr<ljson::array>			as_array() const;
			std::shared_ptr<ljson::object>			as_object() const;

			template<is_allowed_value_type T>
			expected<T, error> access_value(std::function<expected<T, error>(std::shared_ptr<class value>)> fun) const;

			/**
			 * @brief cast a node into a std::string if it is holding ljson::value that is a json string (std::string)
			 * @detail @cpp
			 * ljson::expected<std::string, error> string = node.try_as_string();
			 * if (not string)
			 * {
			 *	// handle error
			 *	std::println("{}", string.error().message());
			 * }
			 * else
			 * {
			 *	// success
			 *	std::string string_value = string.value();
			 * }
			 * @ecpp
			 * @return std::string or ljson::error if it doesn't hold a string
			 */
			expected<std::string, error> try_as_string() const noexcept;
			expected<int64_t, error>     try_as_integer() const noexcept;
			expected<double, error>	     try_as_double() const noexcept;
			expected<double, error>	     try_as_number() const noexcept;
			expected<bool, error>	     try_as_boolean() const noexcept;

			/**
			 * @brief cast a node into a ljson::null_type if it is holding ljson::value that is a json null
			 * @detail @cpp
			 * ljson::expected<null_type, error> null = node.try_as_null();
			 * if (not null)
			 * {
			 *	// handle error
			 *	std::println("{}", null.error().message());
			 * }
			 * else
			 * {
			 *	// success
			 *	ljson::null_type null_value = null.value();
			 * }
			 * @ecpp
			 * @return ljson::null_type or ljson::error if it doesn't hold a null (ljson::null_type)
			 */
			expected<null_type, error> try_as_null() const noexcept;

			/**
			 * @brief cast a node into a string if it is holding ljson::value that is a json string (ljson::null_type)
			 * @exception ljson::error if it doesn't hold a ljson::value and string
			 * @return json null type
			 */
			std::string as_string() const;

			/**
			 * @brief cast a node into a int64_t if it is holding ljson::value that is a json number (int64_t)
			 * @exception ljson::error if it doesn't hold a ljson::value and int64_t
			 * @return json null type
			 */
			int64_t as_integer() const;

			/**
			 * @brief cast a node into a double if it is holding ljson::value that is a json number (double)
			 * @exception ljson::error if it doesn't hold a ljson::value and double
			 * @return json null type
			 */
			double as_double() const;

			/**
			 * @brief cast a node into a double if it is holding ljson::value that is a json number (int64_t or double)
			 * @exception ljson::error if it doesn't hold a ljson::value and (int64_t or double)
			 * @return json null type
			 */
			double as_number() const;

			/**
			 * @brief cast a node into a bool if it is holding ljson::value that is a json boolean (bool)
			 * @exception ljson::error if it doesn't hold a ljson::value and bool
			 * @return json null type
			 */
			bool as_boolean() const;

			/**
			 * @brief cast a node into a ljson::null_type if it is holding ljson::value that is a json null (ljson::null_type)
			 * @exception ljson::error if it doesn't hold a ljson::value and ljson::null_type
			 * @return json null type
			 */
			null_type as_null() const;

			/**
			 * @brief checks if ljson::node is holding ljson::value
			 * @return true if it does
			 * @detail @cpp
			 * ljson::node node(ljson::node_type::value);
			 * if (node.is_value())
			 * {
			 *	// do something
			 * }
			 * @ecpp
			 */
			bool is_value() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::array
			 * @return true if it does
			 */
			bool is_array() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::object
			 * @return true if it does
			 */
			bool is_object() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::value that is holding json string (std::string)
			 * @return true if it does
			 */
			bool is_string() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::value that is holding json number (int64_t)
			 * @return true if it does
			 */
			bool is_integer() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::value that is holding json number (double)
			 * @return true if it does
			 */
			bool is_double() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::value that is holding json number (double or int64_t)
			 * @return true if it does
			 */
			bool is_number() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::value that is holding json boolean (bool)
			 * @return true if it does
			 */
			bool is_boolean() const noexcept;

			/**
			 * @brief checks if ljson::node is holding ljson::value that is holding json null (ljson::null_type)
			 * @return true if it does
			 */
			bool is_null() const noexcept;

			/**
			 * @brief gets the ljson::node_type of the internal node
			 * @return node_type
			 */
			node_type type() const noexcept;

			/**
			 * @brief gets string representation of ljson::node_type of the internal node
			 * @return string name of the node_type
			 */
			std::string type_name() const noexcept;

			/**
			 * @brief gets the ljson::value_type of the node if it's holding ljson::value. otherwise it returns value_type::none
			 * @return value_type
			 */
			value_type valuetype() const noexcept;

			/**
			 * @brief gets string representation of ljson::value_type of the internal node if it's holding ljson::value
			 * @return string name of the value_type
			 */
			std::string value_type_name() const noexcept;

			/**
			 * @brief stringify the json (object, array or value) inside the ljson::node
			 * @detail @json
			 * {
			 *   "key1": "val1",
			 *   "key2": "val2"
			 * }
			 * @ejson
			 * @detail @json
			 * [
			 *   "val1",
			 *   "val2",
			 *   "val3"
			 * ]
			 * @ejson
			 * @detail @json
			 *  value
			 * @ejson
			 * @return serialized json
			 */
			std::string stringify() const noexcept;

			/**
			 * @brief checks if a key exists in a json object
			 * @param key key to lookup
			 * @return true if it does
			 */
			bool contains(const std::string& key) const noexcept;

			/**
			 * @brief checks if a key exists in a json object
			 * @param object_key string value of the json object key
			 * @return ljson::node& at the specified key
			 */
			class node& at(const std::string& object_key) const;

			/**
			 * @brief access the node at the specified array index
			 * @param array_index size_t value of the json array index
			 * @return ljson::node& at the specified index
			 */
			class node& at(const size_t array_index) const;

			/**
			 * @brief set a node with a container_or_node_type
			 * @param node_value value to be set
			 */
			template<typename container_or_node_type>
			void set(const container_or_node_type& node_value) noexcept;

			/**
			 * @brief asign a node with a container_or_node_type
			 * @param node_value value to be set
			 * @return the address of the node which can be used to modify the value
			 */
			template<typename container_or_node_type>
			class node& operator=(const container_or_node_type& node_value) noexcept;

			class node& operator+=(const std::initializer_list<std::pair<std::string, std::any>>& pairs);
			class node& operator+=(const std::initializer_list<std::any>& val);
			class node  operator+(const node& other_node);

			void dump(const std::function<void(std::string)> out_func, const std::pair<char, int>& indent_conf = {' ', 4},
			    int indent = 0) const;
			void dump_to_stdout(const std::pair<char, int>& indent_conf = {' ', 4}) const;
			std::string		   dump_to_string(const std::pair<char, int>& indent_conf = {' ', 4}) const;
			expected<monostate, error> dump_to_file(
			    const std::filesystem::path& path, const std::pair<char, int>& indent_conf = {' ', 4}) const;

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

	/**
	 * @class array
	 * @brief the class that holds a json array
	 */
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

	/**
	 * @class object
	 * @brief the class that holds a json object
	 */
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
			static bool			  done_or_not_ok(const expected<bool, error>& ok);
			static expected<monostate, error> return_error_if_not_ok(const expected<bool, error>& ok);
			static expected<monostate, error> check_unhandled_hierarchy(const std::string& raw_json, struct parsing_data& data);
			static expected<monostate, error> parsing(struct parsing_data& data);

		public:
			explicit parser();
			~parser();
			static ljson::node		    parse(const std::filesystem::path& path);
			static ljson::node		    parse(const std::string& raw_json);
			static ljson::node		    parse(const char* raw_json);
			static expected<ljson::node, error> try_parse(const std::filesystem::path& path) noexcept;
			static expected<ljson::node, error> try_parse(const std::string& raw_json) noexcept;
			static expected<ljson::node, error> try_parse(const char* raw_json) noexcept;
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
							case json_syntax::flush_value:
							case json_syntax::maybe_empty_space_after:
							default:
								return std::format("unexpected syntax: found '{}'", data.line[data.i]);
						}
					}
			};
	};

	node::node() : _node(std::make_shared<ljson::object>())
	{
	}

	node::node(enum node_type type)
	{
		switch (type)
		{
			case node_type::value:
				_node = std::make_shared<class value>();
				break;
			case node_type::array:
				_node = std::make_shared<ljson::array>();
				break;
			case ljson::node_type::object:
				_node = std::make_shared<ljson::object>();
				break;
		}
	}

	template<typename container_or_node_type>
	node::node(const container_or_node_type& node_value) noexcept
	{
		this->setting_allowed_node_type(node_value);
	}

	template<typename is_allowed_node_type>
	constexpr std::variant<class value, ljson::node> node::handle_allowed_node_types(const is_allowed_node_type& value) noexcept
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

	template<typename container_or_node_type>
	constexpr void node::setting_allowed_node_type(const container_or_node_type& node_value) noexcept
	{
		if constexpr (is_value_container<container_or_node_type>)
		{
			_node = std::make_shared<ljson::array>();

			for (auto& val : node_value)
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
		else if constexpr (is_key_value_container<container_or_node_type>)
		{
			_node = std::make_shared<ljson::object>();

			for (auto& [key, val] : node_value)
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
		else if constexpr (is_allowed_node_type<container_or_node_type>)
		{
			std::variant<class value, ljson::node> n = this->handle_allowed_node_types(node_value);
			if (std::holds_alternative<class value>(n))
			{
				_node = std::make_shared<class value>(std::get<class value>(n));
			}
			else
			{
				_node = std::get<ljson::node>(n)._node;
			}
		}
		else
		{
			static_assert(false && "unsupported type in the ljson::node constructor");
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

	template<typename container_or_node_type>
	expected<class ljson::node, error> node::insert(const std::string& key, const container_or_node_type& value)
	{
		ljson::node n(value);
		return this->add_node_to_key(key, n);
	}

	template<typename container_or_node_type>
	expected<class ljson::node, error> node::push_back(const container_or_node_type& value)
	{
		ljson::node n(value);
		return this->add_node_to_array(n);
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

	expected<std::shared_ptr<class value>, error> node::try_as_value() const noexcept
	{
		if (not this->is_value())
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to cast a '{}' node to a value", this->type_name()));

		return std::get<std::shared_ptr<class value>>(_node);
	}

	expected<std::shared_ptr<ljson::array>, error> node::try_as_array() const noexcept
	{
		if (not this->is_array())
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to cast a '{} node to an array", this->type_name()));

		return std::get<std::shared_ptr<ljson::array>>(_node);
	}

	expected<std::shared_ptr<ljson::object>, error> node::try_as_object() const noexcept
	{
		if (not this->is_object())
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to cast a '{}' node to an object", this->type_name()));

		return std::get<std::shared_ptr<ljson::object>>(_node);
	}

	std::shared_ptr<class value> node::as_value() const
	{
		auto ok = this->try_as_value();
		if (not ok)
			throw ok.error();

		return ok.value();
	}

	std::shared_ptr<ljson::array> node::as_array() const
	{
		auto ok = this->try_as_array();
		if (not ok)
			throw ok.error();

		return ok.value();
	}

	std::shared_ptr<ljson::object> node::as_object() const
	{
		auto ok = this->try_as_object();
		if (not ok)
			throw ok.error();

		return ok.value();
	}

	bool node::is_value() const noexcept
	{
		if (std::holds_alternative<std::shared_ptr<class value>>(_node))
			return true;
		else
			return false;
	}

	bool node::is_array() const noexcept
	{
		if (std::holds_alternative<std::shared_ptr<ljson::array>>(_node))
			return true;
		else
			return false;
	}

	bool node::is_object() const noexcept
	{
		if (std::holds_alternative<std::shared_ptr<ljson::object>>(_node))
			return true;
		else
			return false;
	}

	bool node::is_string() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_string();
	}

	bool node::is_integer() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_integer();
	}

	bool node::is_double() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_double();
	}

	bool node::is_number() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_number();
	}

	bool node::is_boolean() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_boolean();
	}

	bool node::is_null() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_null();
	}

	node_type node::type() const noexcept
	{
		if (this->is_value())
			return node_type::value;
		else if (this->is_array())
			return node_type::array;
		else
			return node_type::object;
	}

	std::string node::type_name() const noexcept
	{
		if (this->is_value())
			return "node value";
		else if (this->is_array())
			return "node array";
		else
			return "node object";
	}

	template<is_allowed_value_type T>
	expected<T, error> node::access_value(std::function<expected<T, error>(std::shared_ptr<class value>)> fun) const
	{
		auto val = this->try_as_value();
		if (not val)
		{
			return val.error();
		}

		return fun(val.value());
	}

	expected<std::string, error> node::try_as_string() const noexcept
	{
		auto cast_fn = [](std::shared_ptr<class ljson::value> val) -> expected<std::string, error> { return val->try_as_string(); };
		return this->access_value<std::string>(cast_fn);
	}

	expected<int64_t, error> node::try_as_integer() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class ljson::value> val) -> expected<int64_t, error> { return val->try_as_integer(); };
		return this->access_value<int64_t>(cast_func);
	}

	expected<double, error> node::try_as_double() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class ljson::value> val) -> expected<double, error> { return val->try_as_double(); };
		return this->access_value<double>(cast_func);
	}

	expected<double, error> node::try_as_number() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class ljson::value> val) -> expected<double, error> { return val->try_as_number(); };
		return this->access_value<double>(cast_func);
	}

	expected<bool, error> node::try_as_boolean() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class ljson::value> val) -> expected<bool, error> { return val->try_as_boolean(); };
		return this->access_value<bool>(cast_func);
	}

	expected<null_type, error> node::try_as_null() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class ljson::value> val) -> expected<null_type, error> { return val->try_as_null(); };
		return this->access_value<null_type>(cast_func);
	}

	std::string node::as_string() const
	{
		auto ok = this->try_as_string();
		if (not ok)
			throw ok.error();
		return ok.value();
	}

	int64_t node::as_integer() const
	{
		auto ok = this->try_as_integer();
		if (not ok)
			throw ok.error();
		return ok.value();
	}

	double node::as_double() const
	{
		auto ok = this->try_as_double();
		if (not ok)
			throw ok.error();
		return ok.value();
	}

	double node::as_number() const
	{
		auto ok = this->try_as_number();
		if (not ok)
			throw ok.error();
		return ok.value();
	}

	bool node::as_boolean() const
	{
		auto ok = this->try_as_boolean();
		if (not ok)
			throw ok.error();
		return ok.value();
	}

	null_type node::as_null() const
	{
		auto ok = this->try_as_null();
		if (not ok)
			throw ok.error();
		return ok.value();
	}

	value_type node::valuetype() const noexcept
	{
		if (not this->is_value())
			return value_type::none;
		return this->as_value()->type();
	}

	std::string node::value_type_name() const noexcept
	{
		if (not this->is_value())
			return "none";
		return this->as_value()->type_name();
	}

	std::string node::stringify() const noexcept
	{
		if (this->is_value())
			return this->as_value()->stringify();
		else
			return this->dump_to_string();
	}

	bool node::contains(const std::string& key) const noexcept
	{
		auto obj = this->try_as_object();
		if (not obj)
			return false;
		auto itr = obj.value()->find(key);

		if (itr != obj.value()->end())
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
			throw error(error_type::key_not_found, "index: '{}' not found", array_index);

		return arr->at(array_index);
	}

	template<typename container_or_node_type>
	class node& node::operator=(const container_or_node_type& node_value) noexcept
	{
		if constexpr (std::is_same_v<container_or_node_type, ljson::node>)
		{
			if (this != &node_value)
			{
				this->setting_allowed_node_type(node_value);
			}
		}
		else
		{
			this->setting_allowed_node_type(node_value);
		}
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

	template<typename container_or_node_type>
	void node::set(const container_or_node_type& node_value) noexcept
	{
		this->setting_allowed_node_type(node_value);
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

	void node::dump_to_stdout(const std::pair<char, int>& indent_conf) const
	{
		auto func = [](const std::string& output) { std::cout << output; };
		this->dump(func, indent_conf);
	}

	std::string node::dump_to_string(const std::pair<char, int>& indent_conf) const
	{
		std::string data;
		auto	    func = [&data](const std::string& output) { data += output; };
		this->dump(func, indent_conf);

		return data;
	}

	expected<monostate, error> node::dump_to_file(const std::filesystem::path& path, const std::pair<char, int>& indent_conf) const
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

	expected<monostate, error> parser::return_error_if_not_ok(const expected<bool, error>& ok)
	{
		if (not ok)
			return unexpected(ok.error());
		return monostate();
	}

	expected<monostate, error> parser::check_unhandled_hierarchy(const std::string& raw_json, struct parsing_data& data)
	{
		if (data.hierarchy.empty() || raw_json.empty())
			return monostate();

		if (raw_json.back() == '}' && data.hierarchy.top().first == json_syntax::opening_bracket)
		{
			data.line = "}";
			data.i	  = 0;
			assert(data.i < data.line.size());
			if (auto ok = parser_syntax::closing_bracket::handle_closing_bracket(data); done_or_not_ok(ok))
			{
				return return_error_if_not_ok(ok);
			}
		}
		else
		{
			data.line = (not raw_json.empty()) ? std::string(raw_json.back(), 1) : "";
			data.i	  = std::min(raw_json.size() - 1, ( size_t ) 0);
			assert(data.i < data.line.size());
			if (auto ok = parser_syntax::syntax_error::handle_syntax_error(data); done_or_not_ok(ok))
			{
				return return_error_if_not_ok(ok);
			}
		}

		return monostate();
	}

	expected<monostate, error> parser::parsing(struct parsing_data& data)
	{
		expected<bool, error> ok;

		if (parser_syntax::end_statement::run_till_end_of_statement(data))
			return monostate();

		if (ok = parser_syntax::empty::handle_empty(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::quotes::handle_quotes(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::key::handle_key(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::column::handle_column(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::value::handle_value(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::object::handle_object(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::array::handle_array(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::end_statement::handle_end_statement(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::open_bracket::handle_open_bracket(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::closing_bracket::handle_closing_bracket(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = parser_syntax::syntax_error::handle_syntax_error(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}

		return monostate();
	}

	ljson::node parser::parse(const std::filesystem::path& path)
	{
		expected<ljson::node, error> ok = ljson::parser::try_parse(path);
		if (not ok)
			throw ok.error();

		return ok.value();
	}

	ljson::node parser::parse(const char* raw_json)
	{
		expected<ljson::node, error> ok = ljson::parser::try_parse(raw_json);
		if (not ok)
			throw ok.error();

		return ok.value();
	}

	ljson::node parser::parse(const std::string& raw_json)
	{
		expected<ljson::node, error> ok = ljson::parser::try_parse(raw_json);
		if (not ok)
			throw ok.error();

		return ok.value();
	}

	expected<ljson::node, error> parser::try_parse(const std::filesystem::path& path) noexcept
	{
		ljson::node json_data = ljson::node(node_type::object);

		std::unique_ptr<std::ifstream> file = std::make_unique<std::ifstream>(path);
		if (not file->is_open())
			return unexpected(ljson::error(
			    error_type::filesystem_error, std::format("couldn't open '{}', {}", path.string(), std::strerror(errno))));
		struct parsing_data data;

		data.json_objs.push(json_data);
		data.keys.push({"", key_type::simple_key});

		expected<monostate, error> ok;

		while (std::getline(*file, data.line))
		{
			data.line += "\n";
			for (data.i = 0; data.i < data.line.size(); data.i++)
			{
				ok = ljson::parser::parsing(data);
				if (not ok)
					return unexpected(ok.error());
			}

			if (not file->eof())
				data.line.clear();

			data.line_number++;
		}

		ok = ljson::parser::check_unhandled_hierarchy(data.line, data);
		if (not ok)
			return unexpected(ok.error());

		return json_data;
	}

	expected<ljson::node, error> parser::try_parse(const std::string& raw_json) noexcept
	{
		ljson::node json_data = ljson::node(node_type::object);

		struct parsing_data data;

		data.json_objs.push(json_data);
		data.keys.push({"", key_type::simple_key});

		expected<monostate, error> ok;

		for (size_t i = 0; i < raw_json.size(); i++)
		{
			data.line += raw_json[i];

			if (not data.line.empty() && (data.line.back() == '\n' || data.line.back() == ',' || data.line.back() == '}'))
			{

				for (data.i = 0; data.i < data.line.size(); data.i++)
				{
					ok = ljson::parser::parsing(data);
					if (not ok)
						return unexpected(ok.error());
				}
				data.line.clear();

				data.line_number++;
			}
		}

		ok = ljson::parser::check_unhandled_hierarchy(raw_json, data);
		if (not ok)
			return unexpected(ok.error());

		return json_data;
	}

	expected<ljson::node, error> parser::try_parse(const char* raw_json) noexcept
	{
		assert(raw_json != NULL);
		std::string string_json(raw_json);
		return ljson::parser::try_parse(string_json);
	}

	parser::~parser()
	{
	}

	error::error(error_type err, const std::string& message) : err_type(err), msg(message)
	{
	}

	template<typename... args_t>
	error::error(error_type err, std::format_string<args_t...> fmt, args_t&&... args)
	    : err_type(err), msg(std::format(fmt, std::forward<args_t>(args)...))
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

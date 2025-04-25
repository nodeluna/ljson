#pragma once

#include <any>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <expected>
#include <map>
#include <stack>
#include <fstream>
#include <format>
#include <functional>
#include <system_error>
#include <unordered_set>
#include <variant>
#include <vector>
#include <source_location>

namespace ljson {
	std::string log(const std::string& msg, std::source_location location = std::source_location::current()) {
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

	void print_log(const std::string& msg, std::source_location location = std::source_location::current()) {
		std::cout << log(msg, location);
	}

	bool is_num_decimal(const std::string& x) {
		if (x.empty())
			return false;
		bool has_decimal = false;
		return std::all_of(x.begin(), x.end(), [&](char c) {
			if (std::isdigit(c))
				return true;
			else if (c == '.' && not has_decimal) {
				has_decimal = true;
				return true;
			} else
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

	class error : public std::exception {
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
		null,
		array,
		boolean,
		object,
		temp_escape_type,
		unknown,
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

	struct value {
			std::string value;
			value_type  type = value_type::none;
	};

	class json;

	template<typename T>
	concept value_or_jclass_concept_t = std::is_same_v<T, struct value> || std::is_same_v<T, json>;

	template<typename T>
	concept value_concept_t = std::is_same_v<T, struct value>;

	template<typename T>
	concept value_or_jclass_concept_ptr_t =
	    std::is_same_v<T, std::shared_ptr<struct value>> || std::is_same_v<T, std::shared_ptr<json>>;

	class json {
		private:
			using value_or_json_class = std::variant<std::shared_ptr<struct value>, std::shared_ptr<json>>;
			using json_object	  = std::map<std::string, value_or_json_class>;
			using json_array	  = std::vector<value_or_json_class>;
			std::variant<json_object, json_array, value_or_json_class> data;

			bool data_is_map() {
				if (std::holds_alternative<json_object>(data))
					return true;
				return false;
			}

			bool data_is_array() {
				if (std::holds_alternative<json_array>(data))
					return true;
				return false;
			}

			bool data_is_value() {
				if (std::holds_alternative<value_or_json_class>(data))
					return true;
				return false;
			}

		public:
			explicit json() {
			}

			explicit json(enum value_type type) {
				if (type == value_type::object) {
					data = json_object{};
				} else if (type == value_type::array) {
					data = json_array{};
				} else {
					data = value_or_json_class{};
				}
			}

			void handle_std_any(const std::any& any_value, std::function<void(std::any)> insert_func) {
				if (any_value.type() == typeid(json)) {
					auto val = std::any_cast<json>(any_value);
					insert_func(val);
				} else {
					struct value value;
					if (any_value.type() == typeid(bool)) {
						auto val    = std::any_cast<bool>(any_value);
						value.type  = value_type::boolean;
						value.value = (val == true ? "true" : "false");
						insert_func(value);
					} else if (any_value.type() == typeid(double)) {
						auto val    = std::any_cast<double>(any_value);
						value.type  = value_type::number;
						value.value = std::to_string(val);
						insert_func(value);
					} else if (any_value.type() == typeid(int)) {
						auto val    = std::any_cast<int>(any_value);
						value.type  = value_type::number;
						value.value = std::to_string(val);
						insert_func(value);
					} else if (any_value.type() == typeid(float)) {
						auto val    = std::any_cast<float>(any_value);
						value.type  = value_type::number;
						value.value = std::to_string(val);
						insert_func(value);
					} else if (any_value.type() == typeid(const char*)) {
						auto val    = std::any_cast<const char*>(any_value);
						value.type  = value_type::string;
						value.value = val;
						insert_func(value);
					} else if (any_value.type() == typeid(std::string)) {
						auto val    = std::any_cast<std::string>(any_value);
						value.type  = value_type::string;
						value.value = val;
						insert_func(value);
					} else {
						throw error(error_type::wrong_type,
						    std::string("unknown type given to json constructor: ") + any_value.type().name());
					}
				}
			}

			json(const std::initializer_list<std::pair<std::string, std::any>>& pairs) : data(json_object{}) {
				auto& map = std::get<json_object>(data);

				std::string key;

				auto insert_func = [&](const std::any& value) {
					if (value.type() == typeid(json)) {
						auto val = std::any_cast<json>(value);
						map.insert({key, std::make_shared<json>(val)});
					} else {
						auto val = std::any_cast<struct value>(value);
						map.insert({key, std::make_shared<struct value>(val)});
					}
				};

				for (const auto& pair : pairs) {
					key = pair.first;
					this->handle_std_any(pair.second, insert_func);
					key.clear();
				}
			}

			json(const std::initializer_list<std::any>& val) : data(json_array{}) {
				auto& vector = std::get<json_array>(data);

				auto insert_func = [&](const std::any& value) {
					if (value.type() == typeid(json)) {
						auto val = std::any_cast<json>(value);
						vector.push_back(std::make_shared<json>(val));
					} else {
						auto val = std::any_cast<struct value>(value);
						vector.push_back(std::make_shared<struct value>(val));
					}
				};

				for (const auto& value : val) {
					this->handle_std_any(value, insert_func);
				}
			}

			template<value_or_jclass_concept_t value_t>
			explicit json(const value_t& value) : data(value_or_json_class{}) {
				auto& val = std::get<value_or_json_class>(data);

				if (std::is_same<value_t, json>::value) {
					val = std::make_shared<json>(value);
				} else {
					val = std::make_shared<struct value>(value);
				}
			}

			template<value_or_jclass_concept_ptr_t value_t>
			explicit json(const value_t& value) : data(value_or_json_class{}) {
				auto& val = std::get<value_or_json_class>(data);

				if (std::is_same<value_t, std::shared_ptr<json>>::value) {
					val = value;
				} else {
					val = value;
				}
			}

			value_type type() {
				if (data_is_map())
					return value_type::object;
				else if (data_is_array())
					return value_type::array;
				else if (data_is_value()) {
					auto& val = std::get<value_or_json_class>(data);
					if (std::holds_alternative<std::shared_ptr<json>>(val))
						return value_type::object;
					else if (std::holds_alternative<std::shared_ptr<struct value>>(val)) {
						auto value = std::get<std::shared_ptr<struct value>>(val);
						if (value->type == ljson::value_type::string)
							return ljson::value_type::string;
						else if (value->type == ljson::value_type::boolean)
							return ljson::value_type::boolean;
						else if (value->type == ljson::value_type::number)
							return ljson::value_type::number;
						else if (value->type == ljson::value_type::null)
							return ljson::value_type::null;
						else
							return ljson::value_type::unknown;
					} else
						return value_type::unknown;
				} else
					return value_type::unknown;
			}

			std::string type_name() {
				auto Type = this->type();
				switch (Type) {
					case ljson::value_type::string:
						return "string";
					case ljson::value_type::boolean:
						return "boolean";
					case ljson::value_type::null:
						return "null";
					case ljson::value_type::array:
						return "array";
					case ljson::value_type::object:
						return "object";
					case ljson::value_type::number:
						return "number";
					default:
						return "unknown";
				}
			}

			template<value_or_jclass_concept_t value_t>
			std::expected<std::monostate, error> add_value_to_key(const std::string& key, const value_t& value) {
				if (not data_is_map()) {
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding to map"));
				}

				auto& map = std::get<json_object>(data);
				map[key]  = std::make_shared<value_t>(value);

				return std::monostate();
			}

			template<value_or_jclass_concept_t value_t>
			std::expected<std::monostate, error> add_value_to_array(const size_t key, const value_t& value) {
				if (not data_is_array())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding to array"));

				auto& vector = std::get<json_array>(data);
				if (key >= vector.size())
					return std::unexpected(error(error_type::wronge_index, "wrong index"));

				vector[key] = std::make_shared<value_t>(value);

				return std::monostate();
			}

			template<value_or_jclass_concept_t value_t>
			std::expected<std::monostate, error> add_value_to_array(const value_t& value) {
				if (not data_is_array())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding to array"));

				auto& vector = std::get<json_array>(data);
				vector.push_back(std::make_shared<value_t>(value));

				return std::monostate();
			}

			std::expected<std::shared_ptr<json>, error> add_object_to_key(const std::string& key) {
				if (not data_is_map())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding to map"));

				auto& map = std::get<json_object>(data);
				auto& itr = map[key] = std::make_shared<json>(value_type::object);

				const auto& ptr = std::get<std::shared_ptr<json>>(itr);

				return ptr;
			}

			std::expected<std::monostate, error> add_object_to_key(const std::string& key, const json& json) {
				if (not data_is_map())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding to map"));

				auto& map = std::get<json_object>(data);
				map[key]  = std::make_shared<ljson::json>(json);

				return std::monostate();
			}

			std::expected<std::shared_ptr<json>, error> add_object_to_array() {
				if (not data_is_array())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding obj to array"));

				auto& vector = std::get<json_array>(data);
				vector.push_back(std::make_shared<ljson::json>(value_type::object));

				return std::get<std::shared_ptr<json>>(vector.back());
			}

			std::expected<std::shared_ptr<json>, error> add_object_to_array(const json& obj) {
				if (not data_is_array())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding obj to array"));

				auto& vector = std::get<json_array>(data);
				vector.push_back(std::make_shared<ljson::json>(obj));

				return std::get<std::shared_ptr<json>>(vector.back());
			}

			std::expected<std::shared_ptr<json>, error> add_array_to_key(const std::string& key) {
				if (not data_is_map())
					return std::unexpected(error(error_type::wrong_type, "wrong type: adding to map"));

				auto& map = std::get<json_object>(data);
				auto& itr = map[key] = std::make_shared<json>(value_type::array);

				return std::get<std::shared_ptr<json>>(itr);
			}

			template<value_concept_t value_t>
			std::expected<std::monostate, error> set(const value_t& value) {
				if (not data_is_value())
					return std::unexpected(error(error_type::wrong_type, "wrong type: can't set value to non-value json-class"));

				auto& val = std::get<value_or_json_class>(data);

				if (std::holds_alternative<std::shared_ptr<ljson::json>>(val)) {
					return std::unexpected(error(error_type::wrong_type, "wrong type: can't set value to non-value json-class"));
				} else {
					const auto& struct_val = std::get<std::shared_ptr<struct value>>(val);
					(*struct_val)	       = value;
				}

				return std::monostate();
			}

			std::expected<std::monostate, error> set(const std::string& value) {
				struct value new_value;
				new_value.type	= ljson::value_type::string;
				new_value.value = value;

				this->set(new_value);

				return std::monostate();
			}

			std::expected<std::monostate, error> set(const double value) {
				struct value new_value;
				new_value.type	= ljson::value_type::number;
				new_value.value = value;

				this->set(new_value);

				return std::monostate();
			}

			std::expected<std::monostate, error> set(const bool value) {
				struct value new_value;
				new_value.type	= ljson::value_type::boolean;
				new_value.value = (value == true ? "true" : "false");

				this->set(new_value);

				return std::monostate();
			}

			std::expected<std::monostate, error> set(const char* value) {
				std::string str = value;
				this->set(str);
				return std::monostate();
			}

			std::expected<std::monostate, error> set(const void* value) {
				if (value != NULL)
					return std::unexpected(error(error_type::wrong_type, "setting wrong value for type: 'null'"));

				struct value new_value;
				new_value.type	= ljson::value_type::null;
				new_value.value = "null";

				this->set(new_value);

				return std::monostate();
			}

			std::expected<std::shared_ptr<struct value>, error> get_value() {
				if (not data_is_value())
					return std::unexpected(error(error_type::wrong_type, "wrong type: getting value"));

				auto& val = std::get<value_or_json_class>(data);

				if (not std::holds_alternative<std::shared_ptr<struct value>>(val))
					return std::unexpected(error(error_type::wrong_type, "wrong type: getting value"));

				return std::get<std::shared_ptr<struct value>>(val);
			}

			bool contains(const std::string& key) {
				if (not data_is_map())
					return false;

				auto& map = std::get<json_object>(data);
				auto  itr = map.find(key);

				if (itr != map.end())
					return true;
				else
					return false;
			}

			void dump(const std::function<void(std::string)> out_func, const std::pair<char, int>& indent_conf = {' ', 4},
			    int indent = 0) {
				auto dump_val = [&indent, &out_func, &indent_conf](const value_or_json_class& value_or_json_class) {
					if (std::holds_alternative<std::shared_ptr<struct value>>(value_or_json_class)) {
						auto val = std::get<std::shared_ptr<struct value>>(value_or_json_class);
						if (val->type == ljson::value_type::string)
							out_func(std::format("\"{}\"", val->value));
						else
							out_func(std::format("{}", val->value));
					} else {
						std::get<std::shared_ptr<json>>(value_or_json_class)
						    ->dump(out_func, indent_conf, indent + indent_conf.second);
					}
				};
				if (data_is_map()) {
					out_func(std::format("{{\n"));
					auto&  map   = std::get<json_object>(data);
					size_t count = 0;
					for (const auto& pair : map) {
						out_func(std::format(
						    "{}\"{}\": ", std::string(indent + indent_conf.second, indent_conf.first), pair.first));
						dump_val(pair.second);

						if (++count != map.size())
							out_func(std::format(","));

						out_func(std::format("\n"));
					}
					out_func(std::format("{}}}", std::string(indent, indent_conf.first)));
				} else if (data_is_array()) {
					out_func(std::format("[\n"));
					auto&  vector = std::get<json_array>(data);
					size_t count  = 0;
					for (const auto& array_value : vector) {
						out_func(std::format("{}", std::string(indent + indent_conf.second, indent_conf.first)));
						dump_val(array_value);

						if (++count != vector.size())
							out_func(std::format(","));

						out_func(std::format("\n"));
					}
					out_func(std::format("{}]", std::string(indent, indent_conf.first)));
				} else if (data_is_value()) {
					auto& val = std::get<value_or_json_class>(data);
					dump_val(val);
				}
			}

			void dump_to_stdout(const std::pair<char, int>& indent_conf = {' ', 4}) {
				auto func = [](const std::string& output) { std::cout << output; };
				this->dump(func, indent_conf);
			}

			std::expected<std::monostate, error> write_to_file(
			    const std::filesystem::path& path, const std::pair<char, int>& indent_conf = {' ', 4}) {
				std::ofstream file(path);
				if (not file.is_open())
					return std::unexpected(error(error_type::filesystem_error, std::strerror(errno)));

				auto func = [&file](const std::string& output) { file << output; };
				this->dump(func, indent_conf);
				file.close();

				return std::monostate();
			}

			std::expected<std::shared_ptr<json>, error> try_at(const std::string& key) {
				if (data_is_value()) {
					auto& val = std::get<value_or_json_class>(data);
					if (std::holds_alternative<std::shared_ptr<json>>(val))
						return std::get<std::shared_ptr<json>>(val)->try_at(key);
				}
				if (not data_is_map())
					return std::unexpected(error(error_type::wrong_type, "wrong type"));

				auto& map = std::get<json_object>(data);
				auto  itr = map.find(key);
				if (itr == map.end()) {
					return std::unexpected(error(error_type::key_not_found, "key not found"));
				}

				if (std::holds_alternative<std::shared_ptr<json>>(itr->second))
					return std::get<std::shared_ptr<json>>(itr->second);
				else
					return std::make_shared<json>(std::get<std::shared_ptr<struct value>>(itr->second));
			}

			std::expected<std::shared_ptr<json>, error> try_at(const size_t key) {
				if (not data_is_array())
					return std::unexpected(error(error_type::wrong_type, "wrong type"));

				auto& vector = std::get<json_array>(data);
				if (key >= vector.size())
					return std::unexpected(error(error_type::wronge_index, "wrong index"));

				if (std::holds_alternative<std::shared_ptr<json>>(vector[key]))
					return std::get<std::shared_ptr<json>>(vector[key]);
				else
					return std::make_shared<json>(std::get<std::shared_ptr<struct value>>(vector[key]));
			}

			template<typename value_concept_t>
			json& operator=(const value_concept_t& other) {
				this->set(other);
				return *this;
			}

			std::shared_ptr<json> at(const std::string& key) {
				if (data_is_value()) {
					auto& val = std::get<value_or_json_class>(data);
					if (std::holds_alternative<std::shared_ptr<json>>(val))
						return std::get<std::shared_ptr<json>>(val)->at(key);
				}
				if (not data_is_map())
					throw error(error_type::wrong_type, "wrong type, not a map");

				auto& map = std::get<json_object>(data);
				auto  itr = map.find(key);
				if (itr == map.end()) {
					throw error(error_type::key_not_found, "key not found");
				}

				if (std::holds_alternative<std::shared_ptr<json>>(itr->second))
					return std::get<std::shared_ptr<json>>(itr->second);
				else
					return std::make_shared<json>(std::get<std::shared_ptr<struct value>>(itr->second));
			}

			std::shared_ptr<json> at(const size_t key) {
				if (not data_is_array())
					throw error(error_type::wrong_type, "wrong type, not an array");

				auto& vector = std::get<json_array>(data);
				if (key >= vector.size())
					throw error(error_type::wronge_index, "wrong index");

				if (std::holds_alternative<std::shared_ptr<json>>(vector[key]))
					return std::get<std::shared_ptr<json>>(vector[key]);
				else
					return std::make_shared<json>(std::get<std::shared_ptr<struct value>>(vector[key]));
			}
	};

	class parser {
		private:
			std::unique_ptr<std::ifstream> file;
			std::shared_ptr<json>	       json_data = std::make_shared<ljson::json>(value_type::object);

		public:
			explicit parser(const std::filesystem::path& path);
			~parser();
			std::shared_ptr<json> parse();
	};
}

namespace ljson {
	struct parsing_data {
			std::string				     line;
			std::stack<std::pair<std::string, key_type>> keys;
			std::stack<std::shared_ptr<json>>	     json_objs;
			struct value				     value;
			std::stack<std::pair<json_syntax, size_t>>   hierarchy;
			size_t					     i		 = 0;
			size_t					     line_number = 1;
	};

	struct parser_syntax {
			struct open_bracket {
					static std::expected<bool, error> handle_open_bracket(struct parsing_data& data) {
						if (data.line[data.i] == '{') {
							data.hierarchy.push({json_syntax::opening_bracket, data.line_number});
							return true;
						}

						return false;
					}
			};

			struct empty {
					static std::expected<bool, error> handle_empty(struct parsing_data& data) {
						if (data.line[data.i] != ' ' && data.line[data.i] != '\t')
							return false;
						else if (data.hierarchy.empty())
							return true;
						else if (quotes::is_hierarchy_qoutes(data.hierarchy))
							return false;
						else if (data.hierarchy.top().first == json_syntax::string_value)
							return false;
						else if (data.value.type != ljson::value_type::none &&
							 data.value.type != ljson::value_type::unknown)
							return false;
						else if (data.value.type != ljson::value_type::string && not data.value.value.empty()) {
							auto ok = end_statement::flush_value(data);
							if (not ok && ok.error().value() == error_type::parsing_error_wrong_type)
								return std::unexpected(error(error_type::parsing_error,
								    "reached an empty-space on non-string unknown type: " +
									data.value.value));
							else
								return ok;
						}

						return true;
					}
			};

			struct key {
					static std::expected<bool, error> handle_key(struct parsing_data& data) {
						if (quotes::is_hierarchy_qoutes(data.hierarchy) && not array::is_array(data)) {
							data.keys.top().first += data.line[data.i];
							return true;
						}

						return false;
					}
			};

			struct quotes {
					static std::expected<bool, error> handle_quotes(struct parsing_data& data) {
						bool found_qoute = false;
						if (first_quote(data)) {
							if (data.hierarchy.top().first == json_syntax::quotes_1) {
								if (not data.keys.empty() && data.keys.top().first.empty())
									data.keys.top().second = key_type::simple_key;
								data.hierarchy.pop();
								return true;
							}

							found_qoute = true;
						} else if (second_quote(data)) {
							if (data.hierarchy.top().first == json_syntax::quotes_2) {
								if (not data.keys.empty() && data.keys.top().first.empty())
									data.keys.top().second = key_type::simple_key;
								data.hierarchy.pop();
								return true;
							}

							found_qoute = true;
						}

						if (found_qoute) {
							if (not data.hierarchy.empty() &&
							    (data.hierarchy.top().first == json_syntax::column ||
								data.hierarchy.top().first == json_syntax::array)) {
								data.hierarchy.push({json_syntax::string_value, data.line_number});
								data.value.type = ljson::value_type::string;
							} else if (not data.hierarchy.empty() &&
								   data.hierarchy.top().first == json_syntax::string_value) {
								data.hierarchy.pop();
								return end_statement::flush_value(data);
							} else
								data.hierarchy.push({json_syntax::quotes_1, data.line_number});

							return true;
						}

						return false;
					}

					static bool first_quote(const struct parsing_data& data) {
						if (data.line[data.i] == '"' &&
						    (data.hierarchy.empty() || data.hierarchy.top().first != json_syntax::quotes_2))
							return true;
						else
							return false;
					}

					static bool second_quote(const struct parsing_data& data) {
						if (data.line[data.i] == '\'' &&
						    (data.hierarchy.empty() || data.hierarchy.top().first != json_syntax::quotes_1))
							return true;
						else
							return false;
					}

					static bool is_hierarchy_qoutes(const std::stack<std::pair<json_syntax, size_t>>& hierarchy) {
						if (hierarchy.empty())
							return false;
						else if (hierarchy.top().first == json_syntax::quotes_1 ||
							 hierarchy.top().first == json_syntax::quotes_2)
							return true;
						else
							return false;
					}

					static bool is_hierarchy_qoutes(const char ch) {
						if (ch == '"' || ch == '\'')
							return true;
						else
							return false;
					}
			};

			struct array {
					static std::expected<bool, error> handle_array(struct parsing_data& data) {
						if (data.hierarchy.empty() || end_statement::is_end_statement(data))
							return false;

						if (data.line[data.i] == '[') {
							data.hierarchy.push({json_syntax::array, data.line_number});
							data.keys.top().second = key_type::array;

							auto ok = data.json_objs.top()->add_array_to_key(data.keys.top().first);
							if (not ok)
								return std::unexpected(ok.error());
							data.json_objs.push(ok.value());

							if (end_statement::next_char_is_newline(data))
								data.line.pop_back();
							return true;
						} else if (is_array(data) && data.line[data.i] == ']') {
							data.hierarchy.pop();

							if (not data.json_objs.empty())
								data.json_objs.pop();

							data.hierarchy.push({json_syntax::maybe_empty_space_after, data.line_number});

							return true;
						}

						return false;
					}

					static bool is_array(const struct parsing_data& data) {
						if (not data.json_objs.empty() && data.json_objs.top()->type() == value_type::array)
							return true;
						return false;
					}

					static bool is_array_char(const char ch) {
						if (ch == '[' || ch == ']')
							return true;
						return false;
					}

					static bool brackets_at_end_of_line(const struct parsing_data& data) {
						if (data.i == data.line.size() - 1 &&
						    (data.line[data.i] == '[' || data.line[data.i] == ']'))
							return true;
						return false;
					}
			};

			struct column {
					static std::expected<bool, error> handle_column(struct parsing_data& data) {
						if (data.line[data.i] != ':')
							return false;
						else if (column_in_quotes(data.hierarchy))
							return false;

						if (two_consecutive_columns(data.hierarchy)) {
							return std::unexpected(error(error_type::parsing_error,
							    std::format("two consecutive columns at: {}, key: {}, val: {}, line: {}",
								data.line_number, data.keys.top().first, data.value.value, data.line)));
						} else {
							data.hierarchy.push({json_syntax::column, data.line_number});
						}

						return true;
					}

					static bool column_in_quotes(const std::stack<std::pair<json_syntax, size_t>>& hierarchy) {
						if (hierarchy.empty())
							return false;
						else if (hierarchy.top().first == json_syntax::quotes_1 ||
							 hierarchy.top().first == json_syntax::quotes_2 ||
							 hierarchy.top().first == json_syntax::string_value)
							return true;
						return false;
					}

					static bool two_consecutive_columns(const std::stack<std::pair<json_syntax, size_t>>& hierarchy) {
						if (not hierarchy.empty() && hierarchy.top().first == json_syntax::column)
							return true;
						return false;
					}
			};

			struct object {
					static std::expected<bool, error> handle_object(struct parsing_data& data) {

						if (data.hierarchy.empty() || end_statement::is_end_statement(data))
							return false;

						if (data.line[data.i] == '{') {
							if (array::is_array(data)) {
								auto ok = data.json_objs.top()->add_object_to_array();
								if (not ok)
									return std::unexpected(ok.error());
								data.json_objs.push(ok.value());
							} else {
								auto ok = data.json_objs.top()->add_object_to_key(data.keys.top().first);
								if (not ok)
									return std::unexpected(ok.error());
								data.json_objs.push(ok.value());
							}

							data.keys.push({"", key_type::simple_key});
							data.hierarchy.push({json_syntax::object, data.line_number});

							return true;
						} else if (is_object(data) && data.line[data.i] == '}') {
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
						} else if (array::is_array(data) && data.line[data.i] == '}') {
							data.hierarchy.pop();
							data.keys.pop();
							if (not data.json_objs.empty())
								data.json_objs.pop();
							data.hierarchy.push({json_syntax::maybe_empty_space_after, data.line_number});
							return true;
						}

						return false;
					}

					static bool is_object(const struct parsing_data& data) {
						if (not data.json_objs.empty() && data.json_objs.top()->type() == value_type::object)
							return true;
						return false;
					}

					static bool is_object_char(const char ch) {
						if (ch == '{' || ch == '}')
							return true;
						return false;
					}

					static bool brackets_at_end_of_line(const struct parsing_data& data) {
						if (data.i == data.line.size() - 1 &&
						    (data.line[data.i] == '{' || data.line[data.i] == '}'))
							return true;
						return false;
					}
			};

			struct escape {
					static std::expected<bool, error> handle_escape_char(struct parsing_data& data) {
						if (data.value.type != value_type::temp_escape_type)
							return false;

						if (not is_next_char_correct(data.line[data.i])) {
							std::string err = std::format("escape sequence is incorrect. expected [\", \\, "
										      "t, b, f, n, r, u, /] found: {}\nline: {}",
							    data.line[data.i], data.line);
							return std::unexpected(error(error_type::parsing_error, err));
						}

						return true;
					}

					static bool is_escape_char(const struct parsing_data& data) {
						if (data.value.type == value_type::string && data.line[data.i] == '\\') {
							return true;
						} else
							return false;
					}

					static bool is_next_char_correct(const char ch) {
						std::unordered_set<char> chars = {'"', '\\', 't', 'b', 'f', 'n', 'r', 'u', '/'};
						// TODO: handle the unicode sequence
						if (auto itr = chars.find(ch); itr != chars.end())
							return true;
						else
							return false;
					}
			};

			struct value {
					static std::expected<bool, error> handle_value(struct parsing_data& data) {
						if (data.hierarchy.empty())
							return false;
						else if (is_not_value(data))
							return false;
						else if (object::is_object_char(data.line[data.i]))
							return false;
						else if (array::is_array_char(data.line[data.i]))
							return false;
						else if (end_statement::is_end_statement(data))
							return false;

						if (empty_value_in_non_string(data))
							return end_statement::flush_value(data);

						if (not array::is_array(data)) {
							data.keys.top().second = key_type::simple_key;
						}

						if (escape::is_escape_char(data)) {
							data.value.type = value_type::temp_escape_type;
						} else if (auto ok = escape::handle_escape_char(data); (ok && ok.value()) || not ok) {
							if (not ok)
								return ok;
							data.value.type = value_type::string;
						} else {
							if (data.hierarchy.top().first == json_syntax::string_value) {
								data.value.type = value_type::string;
							}
						}

						data.value.value += data.line[data.i];

						return true;
					}

					static bool empty_value_in_non_string(const struct parsing_data& data) {
						if (data.value.value.empty())
							return false;
						else if (data.value.type != ljson::value_type::string &&
							 (data.value.value.back() == ' ' || data.value.value.back() == '\t'))
							return true;
						else
							return false;
					}

					static bool is_not_value(const struct parsing_data& data) {
						if (not data.json_objs.empty() && data.json_objs.top()->type() == value_type::array) {
							return false;
						} else if (data.hierarchy.top().first != json_syntax::column &&
							   data.hierarchy.top().first != json_syntax::string_value)
							return true;

						return false;
					}
			};

			struct end_statement {
					static bool next_char_is_newline(const struct parsing_data& data) {
						if (data.i == data.line.size() - 2 && data.line[data.i + 1] == '\n')
							return true;
						else
							return false;
					}

					static bool end_of_line_after_bracket(const struct parsing_data& data) {
						if (data.hierarchy.empty())
							return false;
						else if (data.hierarchy.top().first == json_syntax::column &&
							 data.i == data.line.size() - 1)
							return true;
						return false;
					}

					static std::expected<bool, error> flush_value(struct parsing_data& data) {
						data.hierarchy.push({json_syntax::flush_value, data.line_number});
						return handle_end_statement(data);
					}

					static bool pop_flush_element(const struct parsing_data& data) {
						if (not data.hierarchy.empty() && data.hierarchy.top().first == json_syntax::flush_value) {
							return true;
						}

						return false;
					}

					static bool empty_space_in_number(struct parsing_data& data) {
						bool found_empty = false;

						auto empty_char = [](char ch) {
							if (ch == ' ' || ch == '\t')
								return true;
							return false;
						};

						for (size_t& i = data.i; i < data.line.size(); i++) {
							if (empty_char(data.line[i]))
								found_empty = true;
							else if (data.line[i] == ',' || data.line[i] == '\n')
								return false;
							else if (found_empty && not empty_char(data.line[i]))
								return true;
						}

						return false;
					}

					static std::expected<bool, error> handle_end_statement(struct parsing_data& data) {
						if (not is_end_statement(data)) {
							return false;
						} else if (not there_is_a_value(data))
							return false;

						if (next_char_is_newline(data))
							data.line.pop_back();

						if (pop_flush_element(data))
							data.hierarchy.pop();

						if (data.value.value == "null") {
							data.value.type = value_type::null;
						} else if (data.value.value == "true" || data.value.value == "false") {
							data.value.type = value_type::boolean;
						} else if (is_num_decimal(data.value.value)) {
							data.value.type = value_type::number;
							if (empty_space_in_number(data)) {
								return std::unexpected(error(error_type::parsing_error_wrong_type,
								    std::format(
									"type error: '{}', in line: '{}'", data.value.value, data.line)));
							}
						} else if (data.value.type == ljson::value_type::none && data.value.value.empty()) {
							return true;
						} else if (data.value.type != ljson::value_type::string) {
							data.value.type = value_type::unknown;
							return std::unexpected(error(error_type::parsing_error_wrong_type,
							    std::format("unknown type: '{}', in line: '{}'", data.value.value, data.line)));
						}

						if (data.keys.top().second == key_type::simple_key) {
							auto ok = data.json_objs.top()->add_value_to_key(data.keys.top().first, data.value);
							if (not ok) {
								return std::unexpected(error(error_type::parsing_error,
								    std::format(
									"{}", ljson::log("internal parsing error: [adding value to simple_key]"))));
							}
						} else if (object::is_object(data)) {
							auto ok = fill_object_data(data);
							if (not ok)
								return std::unexpected(ok.error());
						} else if (array::is_array(data)) {
							auto ok = fill_array_data(data);
							if (not ok)
								return std::unexpected(ok.error());
							data.value.value.clear();
							data.value.type = ljson::value_type::none;
							return true;
						}

						data.value.value.clear();
						data.value.type = ljson::value_type::none;
						data.keys.top().first.clear();
						data.keys.top().second = key_type::none;
						data.hierarchy.pop();
						return true;
					}

					static bool is_end_statement(const struct parsing_data& data) {
						if (data.line[data.i] == ',')
							return true;
						else if (data.i == data.line.size() - 1 && not object::brackets_at_end_of_line(data) &&
							 not array::brackets_at_end_of_line(data))
							return true;
						else if (not data.hierarchy.empty() &&
							 data.hierarchy.top().first == json_syntax::flush_value) {
							return true;
						} else
							return false;
					}

					static bool there_is_a_value(const struct parsing_data& data) {
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

					static std::expected<std::monostate, error> fill_object_data(struct parsing_data& data) {
						if (data.keys.top().second == key_type::simple_key) {
							auto ok = data.json_objs.top()->add_value_to_key(data.keys.top().first, data.value);
							if (not ok) {
								return std::unexpected(error(error_type::parsing_error,
								    "internal parsing error\n[adding value to object]"));
							}
						}

						return std::monostate();
					}

					static std::expected<std::monostate, error> fill_array_data(struct parsing_data& data) {
						auto ok = data.json_objs.top()->add_value_to_array(data.value);
						if (not ok) {
							return std::unexpected(error(
							    error_type::parsing_error, "internal parsing error\n[adding value to array]"));
						}

						return std::monostate();
					}

					static bool run_till_end_of_statement(struct parsing_data& data) {
						if (data.hierarchy.empty())
							return false;
						else if (data.hierarchy.top().first != json_syntax::maybe_empty_space_after)
							return false;
						else if (data.line[data.i] == ',' || data.line[data.i] == '\n') {
							data.hierarchy.pop();
							if (not data.hierarchy.empty() && data.hierarchy.top().first == json_syntax::column)
								data.hierarchy.pop();

							if (not data.keys.empty()) {
								data.keys.top().first.clear();
								data.keys.top().second = key_type::none;
							}
							return true;
						} else
							return false;
					}
			};

			struct closing_bracket {
					static std::expected<bool, error> handle_closing_bracket(struct parsing_data& data) {
						if (data.line[data.i] != '}')
							return false;

						if (not data.hierarchy.empty() &&
						    data.hierarchy.top().first == json_syntax::opening_bracket)
							data.hierarchy.pop();
						else {
							if (not data.hierarchy.empty())
								return std::unexpected(error(error_type::parsing_error,
								    std::format("error at: {}, hierarchy.size: {}: line_num: {}, line: {}",
									data.line[data.i], data.hierarchy.size(),
									data.hierarchy.top().second, data.line)));
							else
								return std::unexpected(error(error_type::parsing_error,
								    std::format("extra closing bracket at line: {}", data.line_number)));
						}

						return true;
					}
			};

			struct syntax_error {
					static std::expected<bool, error> handle_syntax_error(struct parsing_data& data) {
						if (end_statement::is_end_statement(data))
							return false;
						return std::unexpected(
						    error(error_type::parsing_error, std::format("syntax error: line: '{}'\n[error]: {}",
											 data.line, expected_x_but_found_y(data))));
					}

					static std::string expected_x_but_found_y(const struct parsing_data& data) {
						if (data.hierarchy.empty())
							return std::format("expected '{{' but found '{}'", data.line[data.i]);

						switch (data.hierarchy.top().first) {
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

	parser::parser(const std::filesystem::path& path) : file(std::make_unique<std::ifstream>(path)) {
		if (not file->is_open())
			throw ljson::error(
			    error_type::filesystem_error, std::format("couldn't open '{}', {}", path.string(), std::strerror(errno)));
	}

	parser::~parser() {
		if (file->is_open())
			file->close();
	}

	std::shared_ptr<json> parser::parse() {
		struct parsing_data data;

		data.json_objs.push(this->json_data);
		data.keys.push({"", key_type::simple_key});

		auto done_or_not_ok = [](const std::expected<bool, error>& ok) -> bool {
			if ((ok && ok.value()) || not ok)
				return true;
			else
				return false;
		};

		auto throw_error_if_not_ok = [](const std::expected<bool, error>& ok) -> void {
			if (not ok)
				throw ok.error();
		};

		std::expected<bool, error> ok;

		while (std::getline(*file, data.line)) {
			data.line += "\n";
			for (data.i = 0; data.i < data.line.size(); data.i++) {
				if (parser_syntax::end_statement::run_till_end_of_statement(data))
					continue;

				if (ok = parser_syntax::empty::handle_empty(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::quotes::handle_quotes(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::key::handle_key(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::column::handle_column(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::value::handle_value(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::object::handle_object(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::array::handle_array(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::end_statement::handle_end_statement(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::open_bracket::handle_open_bracket(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::closing_bracket::handle_closing_bracket(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				} else if (ok = parser_syntax::syntax_error::handle_syntax_error(data); done_or_not_ok(ok)) {
					throw_error_if_not_ok(ok);
				}
			}

			data.line.clear();

			data.line_number++;
		}

		return this->json_data;
	}

	error::error(error_type err, const std::string& message) : err_type(err), msg(message) {
	}

	const char* error::what() const noexcept {
		return msg.c_str();
	}

	const std::string& error::message() const noexcept {
		return msg;
	}

	error_type error::value() {
		return err_type;
	}
}

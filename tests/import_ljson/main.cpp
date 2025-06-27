#include <memory>
#include <filesystem>
#include <format>
#include <iostream>

import ljson;


template<typename... args_t>
void println(std::format_string<args_t...> fmt, args_t&&... args) {
        std::string output = std::format(fmt, std::forward<args_t>(args)...);
        std::cout << output << "\n";
}

void println() {
        std::cout << "\n";
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		return 2;
	}
	std::filesystem::path file = argv[1];

	ljson::parser parser;
	ljson::node   j2 = {
	      {"meow_key1", "meow_value"},
	      {"meow_key2", ljson::node({
				 "arr_key1",
				 "arr_key2",
				 "arr_key3",
				 "arr_key4",
				 "arr_key5",
				 })
	      },

	      {"meow_key3", ljson::node({
			      {"nested_obj_key1", "value1"},
			      {"nested_obj_key2", "value2"},
			      {"nested_obj_key3", "value3"},
			      })
	      },
	};

	try
	{
		ljson::node node = parser.parse(file);
		node.add_node_to_key("key", j2);
		node.at("key").set(ljson::null);
		node.at("key").set(std::string("string value"));

		node.at("key") = std::string("new_value");

		auto obj = node.at("obj").as_object();
		for (auto [key, value] : *obj)
		{
			if (value.is_value())
				println("key: {}, value: {}", key, value.as_value()->stringify());
		}

		if (node.at("obj").contains("arr"))
			println("TRUE if 'obj' contains 'arr'");

		node.dump_to_stdout();

		ljson::node n = node.at("obj").at("arr");
		if (n.is_array())
		{
			std::shared_ptr<ljson::array> arr = n.as_array();
			for (auto& i : *arr)
			{
				println("array element: {}", i.as_value()->stringify());
			}
		}

		n = node.at("obj").at("nested_object");
		if (n.is_object())
		{
			std::shared_ptr<ljson::object> object = n.as_object();
			for (auto& [key, value] : *object)
			{
				if (value.is_value())
					println("object key: {}: {}", key, value.as_value()->stringify(), value.as_value()->type_name());
			}
		}
		node.dump_to_stdout();
	}
	catch (const ljson::error& error)
	{
		println("err: {}", error.what());
	}
}


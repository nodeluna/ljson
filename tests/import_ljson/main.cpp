#include <print>
#include <memory>
#include <filesystem>

import ljson;

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
		auto ok		   = node.at("key").set(ljson::null_value());
		ok		   = node.at("key").set("string value");
		if (not ok)
			std::println("err: {}", ok.error().message());

		node.at("key") = {.value = "new_value", .type = ljson::value_type::string};

		auto obj = node.at("obj").as_object();
		for (auto [key, value] : *obj)
		{
			if (value.is_value())
				std::println("key: {}, value: {}", key, value.as_value()->value);
		}

		if (node.at("obj").contains("arr"))
			std::println("TRUE if 'obj' contains 'arr'");

		node.dump_to_stdout();

		ljson::node n = node.at("obj").at("arr");
		if (n.is_array())
		{
			std::shared_ptr<ljson::array> arr = n.as_array();
			for (auto& i : *arr)
			{
				std::println("array element: {}", i.as_value()->value);
			}
		}

		n = node.at("obj").at("nested_object");
		if (n.is_object())
		{
			std::shared_ptr<ljson::object> object = n.as_object();
			for (auto& [key, value] : *object)
			{
				if (value.is_value())
					std::println("object key: {}: {}", key, value.as_value()->value, value.as_value()->type_name());
			}
		}
		node.dump_to_stdout();
	}
	catch (const ljson::error& error)
	{
		std::println("err: {}", error.what());
	}
}


#include <initializer_list>
#include <iostream>
#include <string>
#include <array>
#include <ljson.hpp>
#include <gtest/gtest.h>

template<typename... args_t>
void println(std::format_string<args_t...> fmt, args_t&&... args) {
        std::string output = std::format(fmt, std::forward<args_t>(args)...);
        std::cout << output << "\n";
}

void println() {
        std::cout << "\n";
}

class ljson_test : public ::testing::Test {
	protected:
		void SetUp() override
		{
		}

		void TearDown() override
		{
		}
};

TEST_F(ljson_test, parsing_simple_json)
{
	std::string   raw_json = R"""({"name": "cat", "age": 5, "smol": true})""";
	ljson::parser parser;

	try
	{
		ljson::node result = parser.parse(raw_json);

		EXPECT_TRUE(result.is_object());

		EXPECT_TRUE(result.at("name").is_value());
		EXPECT_TRUE(result.at("age").is_value());
		EXPECT_TRUE(result.at("smol").is_value());

		EXPECT_TRUE(result.at("name").as_value()->is_string());
		EXPECT_EQ(result.at("name").as_value()->as_string(), "cat");

		EXPECT_TRUE(result.at("age").as_value()->is_number());
		EXPECT_EQ(result.at("age").as_value()->as_number(), 5);

		EXPECT_TRUE(result.at("smol").as_value()->is_boolean());
		EXPECT_EQ(result.at("smol").as_value()->as_boolean(), true);

		result.at("name") = "new_cat";
		EXPECT_EQ(result.at("name").as_value()->as_string(), "new_cat");
		EXPECT_TRUE(result.at("name").as_value()->is_string());

		result.at("age") = 8;
		EXPECT_TRUE(result.at("age").as_value()->is_number());
		EXPECT_EQ(result.at("age").as_value()->as_number(), 8);

		result.at("smol") = false;
		EXPECT_TRUE(result.at("smol").as_value()->is_boolean());
		EXPECT_EQ(result.at("smol").as_value()->as_boolean(), false);

		result.at("smol") = ljson::null;
		EXPECT_TRUE(result.at("smol").as_value()->is_null());
		EXPECT_EQ(result.at("smol").as_value()->as_null(), ljson::null);
	}
	catch (ljson::error& error)
	{
		println("{}", error.what());
	}
}

TEST_F(ljson_test, object_iteration)
{
	std::string raw_json = R"""({"name": "cat", "age": 5, "smol": true})""";

	std::map<std::string, std::pair<std::string, ljson::value_type>> obj = {
	    {"name",   {"cat", ljson::value_type::string}},
	    { "age",	    {"5", ljson::value_type::number}},
	    {"smol", {"true", ljson::value_type::boolean}},
	};

	ljson::parser parser;

	try
	{
		ljson::node result = parser.parse(raw_json);
		EXPECT_TRUE(result.is_object());

		for (const auto& [key, value] : *result.as_object())
		{
			auto itr = obj.find(key);
			EXPECT_NE(itr, obj.end());

			EXPECT_TRUE(value.is_value());
			EXPECT_EQ(itr->second.first, value.as_value()->get_value());
			EXPECT_EQ(itr->second.second, value.as_value()->get_type());
		}
	}
	catch (ljson::error& error)
	{
		println("{}", error.what());
	}
}

TEST_F(ljson_test, array_iteration)
{
	std::string raw_json = R"""(
	{
		"array": [
			"meow",
			"hi",
			5,
			5.7,
			true,
			null,
		]
	}
	)""";

	const std::map<std::string, ljson::value_type> arr = {
	    {"meow",  ljson::value_type::string},
	    {  "hi",  ljson::value_type::string},
	    {   "5",  ljson::value_type::number},
	    { "5.7",  ljson::value_type::number},
	    {"true", ljson::value_type::boolean},
	    {"null",    ljson::value_type::null},
	};

	ljson::parser parser;

	try
	{
		ljson::node result = parser.parse(raw_json);
		EXPECT_TRUE(result.is_object());
		EXPECT_TRUE(result.contains("array"));
		ljson::node array_node = result.at("array");

		EXPECT_TRUE(array_node.is_array());

		for (const auto& value : *array_node.as_array())
		{
			EXPECT_TRUE(value.is_value());

			auto itr = arr.find(value.as_value()->get_value());
			EXPECT_NE(itr, arr.end());

			EXPECT_EQ(value.as_value()->get_type(), itr->second);
		}
	}
	catch (ljson::error& error)
	{
		println("{}", error.what());
	}
}

TEST_F(ljson_test, construct_from_initializer_list)
{
	std::set<int> num = {1, 2, 3};

	ljson::node node = {
	    {"key1",		      5},
	    {"key2",		     "value"},
	    {"key3",		     false},
	    {"key4",	     ljson::null},
	    {"key5", ljson::node({1, 2, 3})},
	};

	EXPECT_TRUE(node.is_object());

	EXPECT_TRUE(node.at("key1").is_value());
	EXPECT_TRUE(node.at("key1").as_value()->is_number());
	EXPECT_EQ(node.at("key1").as_value()->as_number(), 5);

	EXPECT_TRUE(node.at("key2").is_value());
	EXPECT_TRUE(node.at("key2").as_value()->is_string());
	EXPECT_EQ(node.at("key2").as_value()->as_string(), "value");

	EXPECT_TRUE(node.at("key3").is_value());
	EXPECT_TRUE(node.at("key3").as_value()->is_boolean());
	EXPECT_EQ(node.at("key3").as_value()->as_boolean(), false);

	EXPECT_TRUE(node.at("key4").is_value());
	EXPECT_TRUE(node.at("key4").as_value()->is_null());
	EXPECT_EQ(node.at("key4").as_value()->as_null(), ljson::null);

	EXPECT_TRUE(node.at("key5").is_array());

	for (const auto& value : *node.at("key5").as_array())
	{
		EXPECT_TRUE(value.is_value());
		EXPECT_TRUE(value.as_value()->is_number());

		auto itr = num.find(value.as_value()->as_number());
		EXPECT_NE(itr, num.end());
	}
}

TEST_F(ljson_test, construct_from_initializer_list_from_array)
{
	std::map<std::string, ljson::value_type> val = {
	    {"1.322300",	 ljson::value_type::number},
	    {	     "2",  ljson::value_type::number},
	    {  "string",  ljson::value_type::string},
	    {    "true", ljson::value_type::boolean},
	    {    "null",    ljson::value_type::null},
	};

	ljson::node node = {
	    1.3223,
	    2,
	    "string",
	    true,
	    ljson::null,
	};

	EXPECT_TRUE(node.is_array());

	EXPECT_EQ(node.as_array()->size(), val.size());

	EXPECT_TRUE(node.as_array()->at(0).is_value());
	EXPECT_TRUE(node.as_array()->at(0).as_value()->is_number());
	EXPECT_EQ(node.as_array()->at(0).as_value()->as_number(), 1.3223);

	EXPECT_TRUE(node.as_array()->at(1).is_value());
	EXPECT_TRUE(node.as_array()->at(1).as_value()->is_number());
	EXPECT_EQ(node.as_array()->at(1).as_value()->as_number(), 2);

	EXPECT_TRUE(node.as_array()->at(2).is_value());
	EXPECT_TRUE(node.as_array()->at(2).as_value()->is_string());
	EXPECT_EQ(node.as_array()->at(2).as_value()->as_string(), "string");

	EXPECT_TRUE(node.as_array()->at(3).is_value());
	EXPECT_TRUE(node.as_array()->at(3).as_value()->is_boolean());
	EXPECT_EQ(node.as_array()->at(3).as_value()->as_boolean(), true);

	EXPECT_TRUE(node.as_array()->at(4).is_value());
	EXPECT_TRUE(node.as_array()->at(4).as_value()->is_null());
	EXPECT_EQ(node.as_array()->at(4).as_value()->as_null(), ljson::null);

	for (const auto& value : *node.as_array())
	{
		EXPECT_TRUE(value.is_value());

		auto itr = val.find(value.as_value()->get_value());
		EXPECT_NE(itr, val.end());

		EXPECT_EQ(itr->second, value.as_value()->get_type());
	}
}

TEST_F(ljson_test, default_node_type)
{
	ljson::node node;
	EXPECT_TRUE(node.is_object());
	EXPECT_TRUE(not node.is_array());
	EXPECT_TRUE(not node.is_value());

	EXPECT_NO_THROW(node.as_object());
	EXPECT_THROW(node.as_array(), ljson::error);
	EXPECT_THROW(node.as_value(), ljson::error);

	ljson::node node2(ljson::value_type::array);
	EXPECT_NO_THROW(node2.as_array());

	ljson::node node3(ljson::value_type::number);
	EXPECT_NO_THROW(node3.as_value());

	ljson::node node4(ljson::value_type::string);
	EXPECT_NO_THROW(node4.as_value());

	ljson::node node5(ljson::value_type::boolean);
	EXPECT_NO_THROW(node5.as_value());

	ljson::node node6(ljson::value_type::null);
	EXPECT_NO_THROW(node6.as_value());
}

TEST_F(ljson_test, invalid_json)
{
	ljson::parser parser;
	EXPECT_THROW(parser.parse("{invalid}"), ljson::error);
	EXPECT_THROW(parser.parse("{{}"), ljson::error);
	EXPECT_THROW(parser.parse("{\"name\":}"), ljson::error);
}

TEST_F(ljson_test, node_add_object)
{
	ljson::node node = {
	    {"key1", "value1"},
	    {"key2", "value2"},
	};

	node += ljson::object_pairs{
	    {"key3",			      "value3"},
		{"key4",				 "value4"},
	    { "arr", ljson::node({"arr1", "arr2", "arr3"})}
	  };

	EXPECT_TRUE(node.contains("key1"));
	EXPECT_TRUE(node.at("key1").is_value());
	EXPECT_TRUE(node.at("key1").as_value()->is_string());
	EXPECT_EQ(node.at("key1").as_value()->as_string(), "value1");

	EXPECT_TRUE(node.contains("key2"));
	EXPECT_TRUE(node.at("key2").is_value());
	EXPECT_TRUE(node.at("key2").as_value()->is_string());
	EXPECT_EQ(node.at("key2").as_value()->as_string(), "value2");

	EXPECT_TRUE(node.contains("key3"));
	EXPECT_TRUE(node.at("key3").is_value());
	EXPECT_TRUE(node.at("key3").as_value()->is_string());
	EXPECT_EQ(node.at("key3").as_value()->as_string(), "value3");

	EXPECT_TRUE(node.contains("key4"));
	EXPECT_TRUE(node.at("key4").is_value());
	EXPECT_TRUE(node.at("key4").as_value()->is_string());
	EXPECT_EQ(node.at("key4").as_value()->as_string(), "value4");

	EXPECT_TRUE(node.contains("arr"));
	EXPECT_TRUE(node.at("arr").is_array());
}

TEST_F(ljson_test, node_add_array)
{
	ljson::node node(ljson::value_type::array);
	node += ljson::array_values{"value1", "value2", ljson::node({"arr1", "arr2", "arr3"})};

	EXPECT_TRUE(node.is_array());

	EXPECT_EQ(node.at(0).as_value()->as_string(), "value1");
	EXPECT_EQ(node.at(1).as_value()->as_string(), "value2");
	EXPECT_TRUE(node.at(2).is_array());
}

TEST_F(ljson_test, node_plus_node_objects)
{
	ljson::node object_node1 = {
	    {"key1", "value1"},
	    {"key2", "value2"},
	};

	ljson::node object_node2 = {
	    {"key3", "value3"},
	    {"key4", "value4"},
	};

	ljson::node new_node = object_node1 + object_node2;

	EXPECT_TRUE(new_node.is_object());

	EXPECT_TRUE(new_node.contains("key1"));
	EXPECT_TRUE(new_node.contains("key2"));
	EXPECT_TRUE(new_node.contains("key3"));
	EXPECT_TRUE(new_node.contains("key4"));

	EXPECT_TRUE(new_node.at("key1").is_value());
	EXPECT_TRUE(new_node.at("key2").is_value());
	EXPECT_TRUE(new_node.at("key3").is_value());
	EXPECT_TRUE(new_node.at("key4").is_value());

	EXPECT_TRUE(new_node.at("key1").as_value()->is_string());
	EXPECT_TRUE(new_node.at("key2").as_value()->is_string());
	EXPECT_TRUE(new_node.at("key3").as_value()->is_string());
	EXPECT_TRUE(new_node.at("key4").as_value()->is_string());

	EXPECT_EQ(new_node.at("key1").as_value()->as_string(), "value1");
	EXPECT_EQ(new_node.at("key2").as_value()->as_string(), "value2");
	EXPECT_EQ(new_node.at("key3").as_value()->as_string(), "value3");
	EXPECT_EQ(new_node.at("key4").as_value()->as_string(), "value4");

	ljson::node array_node = {
	    1.3223,
	    2,
	    "string",
	    true,
	    ljson::null,
	};

	EXPECT_THROW(array_node + object_node2, ljson::error);
}

TEST_F(ljson_test, node_plus_node_arrays)
{
	ljson::node array_node1 = {
	    1.3223,
	    2,
	    "string",
	    true,
	    ljson::null,
	};

	ljson::node array_node2 = {
	    4,
	    5,
	    "string2",
	    false,
	    ljson::null,
	};

	ljson::node new_node = array_node1 + array_node2;

	EXPECT_TRUE(new_node.is_array());
	EXPECT_EQ(new_node.as_array()->size(), array_node1.as_array()->size() + array_node2.as_array()->size());
}

TEST_F(ljson_test, insert_into_object)
{
	ljson::node node = {
	    {"key1", "value1"},
	    {"key2", "value2"},
	};

	std::map<std::string, int> object = {
		{"key1", 1},
		{"key2", 2},
	};

	std::set<std::string> array = {"arr1", "arr2"};

	node.insert("key3", (const char*) "value3");
	node.insert("key4", (const char*) "value4");
	node.insert("arr", array);
	node.insert("obj", object);

	EXPECT_TRUE(node.contains("key1"));
	EXPECT_TRUE(node.at("key1").is_value());
	EXPECT_TRUE(node.at("key1").as_value()->is_string());
	EXPECT_EQ(node.at("key1").as_value()->as_string(), "value1");

	EXPECT_TRUE(node.contains("key2"));
	EXPECT_TRUE(node.at("key2").is_value());
	EXPECT_TRUE(node.at("key2").as_value()->is_string());
	EXPECT_EQ(node.at("key2").as_value()->as_string(), "value2");

	EXPECT_TRUE(node.contains("key3"));
	EXPECT_TRUE(node.at("key3").is_value());
	EXPECT_TRUE(node.at("key3").as_value()->is_string());
	EXPECT_EQ(node.at("key3").as_value()->as_string(), "value3");

	EXPECT_TRUE(node.contains("key4"));
	EXPECT_TRUE(node.at("key4").is_value());
	EXPECT_TRUE(node.at("key4").as_value()->is_string());
	EXPECT_EQ(node.at("key4").as_value()->as_string(), "value4");

	EXPECT_TRUE(node.contains("arr"));
	EXPECT_TRUE(node.at("arr").is_array());

	EXPECT_TRUE(node.at("arr").at(0).is_value());
	EXPECT_TRUE(node.at("arr").at(0).as_value()->is_string());
	EXPECT_EQ(node.at("arr").at(0).as_value()->as_string(), "arr1");

	EXPECT_TRUE(node.at("arr").at(1).is_value());
	EXPECT_TRUE(node.at("arr").at(1).as_value()->is_string());
	EXPECT_EQ(node.at("arr").at(1).as_value()->as_string(), "arr2");

	EXPECT_TRUE(node.contains("obj"));
	EXPECT_TRUE(node.at("obj").is_object());

	EXPECT_TRUE(node.at("obj").contains("key1"));
	EXPECT_TRUE(node.at("obj").at("key1").is_value());
	EXPECT_TRUE(node.at("obj").at("key1").as_value()->is_number());
	EXPECT_EQ(node.at("obj").at("key1").as_value()->as_number(), 1);

	EXPECT_TRUE(node.at("obj").contains("key2"));
	EXPECT_TRUE(node.at("obj").at("key2").is_value());
	EXPECT_TRUE(node.at("obj").at("key2").as_value()->is_number());
	EXPECT_EQ(node.at("obj").at("key2").as_value()->as_number(), 2);
}

TEST_F(ljson_test, push_back_into_array)
{
	ljson::node node(ljson::value_type::array);

	std::map<std::string, int> object = {
		{"key1", 1},
		{"key2", 2},
	};

	std::set<std::string> array = {"arr1", "arr2", "arr3"};

	node.push_back((const char*) "value1");
	node.push_back((const char*) "value2");
	node.push_back(array);
	node.push_back(object);

	EXPECT_TRUE(node.at(0).is_value());
	EXPECT_TRUE(node.at(0).as_value()->is_string());
	EXPECT_EQ(node.at(0).as_value()->as_string(), "value1");

	EXPECT_TRUE(node.at(1).is_value());
	EXPECT_TRUE(node.at(1).as_value()->is_string());
	EXPECT_EQ(node.at(1).as_value()->as_string(), "value2");

	EXPECT_TRUE(node.at(2).is_array());

	EXPECT_TRUE(node.at(2).as_array()->at(0).is_value());
	EXPECT_TRUE(node.at(2).as_array()->at(0).as_value()->is_string());
	EXPECT_EQ(node.at(2).as_array()->at(0).as_value()->as_string(), "arr1");

	EXPECT_TRUE(node.at(2).as_array()->at(1).is_value());
	EXPECT_TRUE(node.at(2).as_array()->at(1).as_value()->is_string());
	EXPECT_EQ(node.at(2).as_array()->at(1).as_value()->as_string(), "arr2");

	EXPECT_TRUE(node.at(2).as_array()->at(2).is_value());
	EXPECT_TRUE(node.at(2).as_array()->at(2).as_value()->is_string());
	EXPECT_EQ(node.at(2).as_array()->at(2).as_value()->as_string(), "arr3");

	EXPECT_TRUE(node.at(3).is_object());

	EXPECT_TRUE(node.at(3).contains("key1"));
	EXPECT_TRUE(node.at(3).at("key1").is_value());
	EXPECT_TRUE(node.at(3).at("key1").as_value()->is_number());
	EXPECT_EQ(node.at(3).as_object()->at("key1").as_value()->as_number(), 1);

	EXPECT_TRUE(node.at(3).contains("key2"));
	EXPECT_TRUE(node.at(3).at("key2").is_value());
	EXPECT_TRUE(node.at(3).at("key2").as_value()->is_number());
	EXPECT_EQ(node.at(3).as_object()->at("key2").as_value()->as_number(), 2);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	println("[=] running unit tests");
	return RUN_ALL_TESTS();
}

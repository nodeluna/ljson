#include <iostream>
#include <string>
#include <array>
#include <print>
#include <ljson.hpp>
#include <gtest/gtest.h>
#include <unistd.h>

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
		std::println("{}", error.what());
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
			EXPECT_EQ(itr->second.first, value.as_value()->value);
			EXPECT_EQ(itr->second.second, value.as_value()->type);
		}
	}
	catch (ljson::error& error)
	{
		std::println("{}", error.what());
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

			auto itr = arr.find(value.as_value()->value);
			EXPECT_NE(itr, arr.end());

			EXPECT_EQ(value.as_value()->type, itr->second);
		}
	}
	catch (ljson::error& error)
	{
		std::println("{}", error.what());
	}
}

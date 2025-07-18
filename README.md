ljson is an easy to use header only json library for c++

# requirements
- compiler that supports c++20
- gcc 13 or clang 17

# usage

## header-only

- clone the repo
- add the include/ directory to your build system
- include <ljson.hpp>

## c++20 modules

- clone the repo
- add the mod/ and include/ directory to your build system
- import ljson;

check out the example at test/import_ljson/ to see how to use the library as a module in cmake

# tutorial

### reading and writing to files
```cpp
#include <ljson.hpp>
#include <exception>

int main() {
	std::filesystem::path path_to_file = "meow";

	try {
		ljson::node node = ljson::parser::parse(path_to_file);
		std::pair<char, int> indent_config = {'\t', 2}; // you can specifiy tab/space here and it's count
		node.dump_to_stdout(indent_config);
		node.dump_to_stdout(); // not specifiying defaults to {' ', 4}
		node.dump_to_file("new_file.json" /*, indent_config */);


	} catch (const ljson::error& error) {
		// parsing error, JSON syntax error
		// handle error
	}
  
}

```
### exception free parsing
```cpp
#include <ljson.hpp>
#include <print>

int main() {
	std::filesystem::path path_to_file = "meow";
	ljson::expected<ljson::node, ljson::error> node = ljson::parser::try_parse(path_to_file);
	if (not node)
	{
		// handle error
		std::println("{}", node.error().message());
	}
	else
	{
		// parsed successfully
	}
  
}

```


### accessing and changing/setting values

```cpp

#include <ljson.hpp>
#include <exception>

int main() {
	std::filesystem::path path_to_file = "meow";

	// making a json object
	ljson::node j2 = {
		 {"simple_key", "meow_value"},
		 {"array_key", ljson::node({ // ljson::node() can hold an object, array's values or a simple value
				 "arr_key1",
				 "arr_key2",
				 "arr_key3",
				 "arr_key4",
				 "arr_key5",
				 })},
		 {"object_key", ljson::node({
				 {"obj_key1", "value1"},
				 {"obj_key2", "value2"},
				 {"obj_key3", "value3"},
				 })},
	};

	try {
		ljson::node node = ljson::parser::parse(path_to_file);

		// this function adds an object to a key
		node.insert("new_object", j2);

		ljson::node new_node = node.at("object_key").at("nested_key"); // getting the value of nested_key
												 // this function can throw if the key doesn't exist

		// to check if the key exists or not
		if (node.contains("object_key")) {
			new_node = node.at("object_key");
			if (new_node.contains("nested_key")) {
				new_node = new_node.at("nested_key");

				// to change the value, it can be done this way
				new_node.set("new_value_for_nested_key");
				// or this way
				new_node = "new_value_for_nested_key";
			}
		}

		// this function adds an object to an array
		node.at("object").at("array_key").push_back(j2);

		// this function adds a value to an array
		node.at("object").at("array_key").push_back("new_array_value");

		// to access an index inside an array pass a size_t to the .at(index) function
		new_node = node.at("object").at("array_key").at(0); 

		// to chage its value
		std::expected<std::monostate, ljson::error> ok = new_node.set("changed_value");
		if (not ok) {
			std::println("err: {}", ok.error().message()); // ok.error().value() == ljson::error_type::wrong_type
		}
		new_node.set(true);
		new_node.set(12);
		new_node.set(ljson::null); // set its value to 'null'

		node.dump_to_file("new_file.json"); // write the new changes


	} catch (const ljson::error& error) {
		// parsing error, JSON syntax error
		// handle error
	}
}

```



### exception free key access

```cpp

#include <ljson.hpp>
#include <exception>

int main() {
	// making a json object
	ljson::node j2 = {
		 {"simple_key", "meow_value"},
		 {"array_key", ljson::node({ // ljson::node() can hold an object, array's values or a simple value
				 "arr_key1",
				 "arr_key2",
				 "arr_key3",
				 "arr_key4",
				 "arr_key5",
				 })},
		 {"object_key", ljson::node({
				 {"obj_key1", "value1"},
				 {"obj_key2", "value2"},
				 {"obj_key3", "value3"},
				 })},
	};

	// this function adds an object to a key
	node.insert("new_object", j2);

	ljson::expected<std::reference_wrapper<ljson::node>, ljson::error> simple_node = node.try_at("simple_key");
	if (simple_node)
	{
		ljson::node& node_ref = simple_node.value().get();
		ljson::expected<std::string, ljson::error> maybe_string = node_ref.try_as_string();
		if (maybe_string)
		{
			std::println("{}", maybe_value.value()); // prints: meow_value
		}
	}
}

```




### casting to a json array and iteration
```cpp
#include <ljson.hpp>
#include <exception>

int main() {
	std::filesystem::path path_to_file = "meow";

	try {
		// parse the file
		ljson::node new_node = node.at("key").at("array");
		if (new_node.is_array()) {
			// this function can throw if the internal type isn't an json array
			std::shared_ptr<ljson::array> array = new_node.as_array();

			for (ljson::node& element : *array) {
				if (element.is_value()) {
					std::println("array element: {}, type name: {}",
						element->as_value().stringify(), element->as_value().type_name());
				}
			}
		}


	} catch (const ljson::error& error) {
		// parsing error, JSON syntax error
		// handle error
	}
  
}

```

### casting to a json object and iteration
```cpp
#include <ljson.hpp>
#include <exception>

int main() {
	std::filesystem::path path_to_file = "meow";

	try {
		// parse the file
		ljson::node new_node = node.at("key").at("object");
		if (new_node.is_object()) {
			// this function can throw if the internal type isn't a json object
			std::shared_ptr<ljson::object> object = new_node.as_object();

			for (auto& [key, node] : *object) {
				if (node.is_value()) {
					std::println("object key: {} element: {}, type name: {}", key
						node->as_value().stringify(), node->as_value().type_name());
				}
			}
		}


	} catch (const ljson::error& error) {
		// parsing error, JSON syntax error
		// handle error
	}
  
}

```

### exception free casting
```cpp
#include <ljson.hpp>

// add "try_" in front of any casting method that throws

int main()
{
	ljson::node node;

	// throws
	int number = node.as_integer();
	std::string number = node.as_string();


	// doesn't throw
	ljson::expected<int, ljson::error> number = node.try_as_integer();
	ljson::expected<std::string, ljson::error> number = node.try_as_string();
}

```


### inserting std library containers into a node
```cpp
#include <ljson.hpp>
#include <exception>

int main() {
	std::filesystem::path path_to_file = "meow";

	try {
		// parse the file

		std::map<std::string, std::string> object = {
				{"meow_key", "value1"},
				{"meow_key2", "value2"},
		};
		std::list<std::string> array = {"meow", "meoow"};


		if (node.is_object)
		{
			node.insert("array", array); // inserts an array to the value name "array"
			node.insert("object", object); // inserts an object to the value name "object"
		}
		else if (node.is_array())
		{
			node.push_back(array); // pushes back an array at the end of the array
			node.push_back(object); // pushes back an object at the end of the array
		}

	} catch (const ljson::error& error) {
		// parsing error, JSON syntax error
		// handle error
	}
  
}
```


# author

nodeluna - nodeluna@proton.me

# license
    Copyright 2025 nodeluna

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

# special thanks to

- **[tomlplusplus]** - for showing how to make a nice API

[tomlplusplus]: https://github.com/marzer/tomlplusplus

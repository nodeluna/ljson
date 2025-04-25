This is a header-only json library


# reading and writing to files
```cpp
#include <ljson>
#include <exception>

int main() {
	std::string path_to_file = "meow";
	ljson::parser parser = ljson::parser(path_to_file);

	try {
		std::shared_ptr<ljson::json> json = parser.parse();
		std::pair<char, int> indent_config = {'\t', 2}; // you can specifiy tab/space here and it's count
		json->dump_to_stdout(indent_config);
		json->dump_to_stdout(); // not specifiying defaults to {' ', 4}
		json->write_to_file("new_file.json"); // same with writing to files


	} catch (const std::exception& error) {
		// parsing error, JSON syntax error
		// handle error
	}
  
}

```
# accessing and changing/setting values

```cpp

#include <ljson>
#include <exception>

int main() {
	std::string path_to_file = "meow";
	ljson::parser parser = ljson::parser(path_to_file);

	// making a json object
	ljson::json j2 = {
		 {"simple_key", "meow_value"},
		 {"array_key", ljson::json({ // ljson::json() can hold an object or array's values
				 "arr_key1",
				 "arr_key2",
				 "arr_key3",
				 "arr_key4",
				 "arr_key5",
				 })},
		 {"object_key", ljson::json({
				 {"obj_key1", "value1"},
				 {"obj_key2", "value2"},
				 {"obj_key3", "value3"},
				 })},
	};

	try {
		std::shared_ptr<ljson::json> json = parser.parse();

		// this function adds an object to a simple key
		json->add_object_to_key("new_object", j2);

		std::shared_ptr<ljson::json> json_data = json->at("object_key")->("nested_key"); // getting the value of nested_key
												 // this function can throw if the key doesn't exist

		// to check if the key exists or not
		if (json->contains("object_key")) {
			json_data = json->at("object_key");
			if (json_data->contains("nested_key")) {
				json_data = json_data->at("nested_key");

				// to change the value, it can be done this way
				json_data->set("new_value_for_nested_key");
				// or this way
				*json_data = "new_value_for_nested_key";
			}
		}

		// this function adds an object to an array
		json->at("new_object")->at("array_key")->add_object_to_array("object_inside_array", j2);

		// this function adds a value to an array
		json->at("new_object")->at("array_key")->add_value_to_array("new_array_value");

		// to access an index inside an array pass a size_t to the ->at(index) function
		json_data = json->at("new_object")->at("array_key")->at(0); 

		// to chage its value
		json_data->set("changed_value");
		json_data->set(true);
		json_data->set(12);

		json->write_to_file("new_file.json"); // write the new changes


	} catch (const std::exception& error) {
		// parsing error, JSON syntax error
		// handle error
	}
}

```


/*
 * Copyright: 2025 nodeluna
 * SPDX-License-Identifier: Apache-2.0
 * repository: https://github.com/nodeluna/ljson
 */

module;

#include <ljson.hpp>

export module ljson;

export namespace ljson {
	using ljson::array;
	using ljson::error;
	using ljson::error_type;
	using ljson::json;
	using ljson::node;
	using ljson::null_type;
	using ljson::null;
	using ljson::object;
	using ljson::parser;
	using ljson::value;
	using ljson::value_type;
	using ljson::object_pairs;
	using ljson::array_values;
	using ljson::expected;
	using ljson::unexpected;
	using ljson::monostate;
}

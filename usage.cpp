#include "cppjson.h"
#include <iostream>
#include <memory>
#include <string>

using namespace std;

int main() {
  JSON json{std::make_unique<JSON::Object>()};
  auto &root = json->cast<JSON::Object>();
  root["123"] = 456;
  root["\n\n"] = "hello";
  root["\b\t"] = 114.514;
  root["true"] = false;
  root["null"] = nullptr;
  root["array"] =
      JSON::Array::makeArray(1, true, 11514.1919, -2147483648, "miao", nullptr);
  root["array2"] = JSON::Array::makeArray(
      1,
      JSON::Array::makeArray(1, JSON::Array::makeArray(4),
                             JSON::Array::makeArray(JSON::Array::makeArray(5)),
                             JSON::Array::makeArray(1)),
      4, JSON::Array::makeArray());

  std::cout << json->dump();
}
# cpp-json-learn

A C++ JSON parser and serializer for learning. **Do not use in production!**

90% test consistency with Javascript's `JSON`

### Parsing JSON text

```cpp
auto json = JSON::parse(json_str);
```

### Make JSON Object

```cpp
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
// {"array2":[1,[1,[4],[[5]],[1]],4,[]],"array":[1,true,11514.191900,-2147483648,"miao",null],"null":null,"\b\t":114.514000,"true":false,"\n\n":"hello","123":456}
```

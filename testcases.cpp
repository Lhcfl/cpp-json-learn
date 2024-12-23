#include "cppjson.h"
#include "nlohmann-json/json.hpp"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct Result {
  std::string test_name;
  int pass;
  int fail;
  void print() const {
    std::cout << "\n\n=========== RESULT " << test_name << " ============\n";
    std::cout << "total = " << pass + fail << std::endl;
    std::cout << "pass = " << pass << std::endl;
    std::cout << "fail = " << fail << std::endl;
    std::cout << "success = " << pass * 1.0 / (pass + fail) << std::endl;
  }
};

std::string readfile(const std::string &path) {
  std::ifstream fin(path, std::ios::ate);
  size_t size = fin.tellg();
  fin.seekg(0);
  std::string json_str;
  json_str.resize(size);
  fin.read(json_str.data(), size);
  fin.close();
  return json_str;
}

bool show_detailed = false;

Result
test(std::string test_name,
     std::function<void(const std::string &filename, const std::string &s)>
         tester) {
  std::filesystem::directory_iterator testcases("JSONTestSuite/test_parsing");

  int pass = 0, fail = 0;

  for (const auto &testfile : testcases) {
    auto path = testfile.path().string();

    std::string json_str = readfile(path);

    bool me_throwed = false;
    bool cg_throwed = false;
    std::string my_exception;
    std::string cg_exception;
    std::string my_dump;

    try {
      tester(testfile.path().filename().string(), json_str);
    } catch (const std::exception &e) {
      cg_throwed = true;
      cg_exception = (&e)->what();
    }

    try {
      auto json = JSON::parse(json_str);
      my_dump = json->dump();
    } catch (const std::exception &e) {
      me_throwed = true;
      my_exception = (&e)->what();
    }

    if (show_detailed || me_throwed != cg_throwed) {
      std::cout << "\n==============================\n";
      std::cout << "Testing " << test_name << ": " << path << std::endl;
      std::cout << "[TEST]: " << std::string_view(json_str).substr(0, 100)
                << std::endl;
    }
    if (me_throwed == cg_throwed) {
      pass++;
      if (show_detailed) {
        std::cout << "\033[32m[PASS]:\033[0m "
                  << std::string_view(me_throwed ? my_exception : my_dump)
                         .substr(0, 100)
                  << std::endl;
      }
    } else {
      fail++;
      std::cout << "\033[31m[FAIL]: "
                << std::string_view(me_throwed ? my_exception : my_dump)
                       .substr(0, 100)
                << "\033[0m" << std::endl;
      if (cg_throwed)
        std::cout << "\033[33m[Control Group]: "
                  << std::string_view(cg_exception).substr(0, 100) << "\033[0m"
                  << std::endl;
    }
  }

  return {
      std::move(test_name),
      pass,
      fail,
  };
}

int main(int argc, char **argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    args.emplace_back(argv[i]);
    std::cout << argv[i] << "\n";
  }
  if (std::ranges::any_of(args,
                          [](const auto &s) { return s == "--detailed"; })) {
    show_detailed = true;
  }

  std::vector<Result> result;

  result.push_back(test("nlohmann", [](auto, const std::string &s) {
    nlohmann::json::parse(s).dump();
  }));
  result.push_back(test("javascript", [](const std::string &filename, auto) {
    auto js_result = readfile("js-results/" + filename);
    if (js_result.starts_with("ok"))
      return std::string_view(js_result).substr(4, 999);
    else
      throw std::runtime_error(js_result.substr(4, 999));
  }));

  for (const auto &r : result) {
    r.print();
  }
}
#pragma once

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <format>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

class JSON;

class JSON {
public:
  static constexpr int MAX_RECURSE_DEPTH = 1000;
  static constexpr bool ENABLE_DUMPLICATED_KEY_DETECT = false;
  static constexpr bool ENABLE_TRAILING_COMMA = false;

  class JSONException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };
  class JSONParseException : public JSONException {
  public:
    using JSONException::JSONException;
  };

  enum class NodeType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object,
  };

  class Node;
  class Null;
  class Boolean;
  class Number;
  class String;
  class Array;
  class Object;

private:
  std::unique_ptr<Node> _uptr;

public:
  JSON() : _uptr(std::make_unique<Null>()) {};
  JSON(JSON &&) = default;

  template <typename T>
    requires std::is_base_of_v<Node, T>
  explicit JSON(std::unique_ptr<T> &&uptr) : _uptr(std::move(uptr)) {}

  JSON &operator=(JSON &&) = default;
  JSON &operator=(const JSON &) = delete;

  inline JSON &operator=(std::unique_ptr<Node> &&uptr) {
    _uptr = std::move(uptr);
    return *this;
  }

  template <typename T>
    requires std::is_base_of_v<Node, T>
  explicit JSON(T &&node) : _uptr(std::make_unique<T>(std::forward<T>(node))) {}

  template <typename T>
    requires std::is_base_of_v<Node, T>
  inline JSON &operator=(T &&node) {
    _uptr = std::make_unique<T>(std::forward<T>(node));
    return *this;
  }

  explicit JSON(std::nullptr_t) : JSON() {}
  inline JSON &operator=(std::nullptr_t) {
    _uptr = std::make_unique<Null>();
    return *this;
  }

  explicit JSON(bool boolean) : _uptr(std::make_unique<Boolean>(boolean)) {}
  inline JSON &operator=(bool boolean) {
    _uptr = std::make_unique<Boolean>(boolean);
    return *this;
  }

  template <typename IntN>
    requires std::numeric_limits<IntN>::is_integer
  explicit JSON(IntN integer)
      : _uptr(std::make_unique<Number>(static_cast<int64_t>(integer))) {}
  template <typename IntN>
    requires std::numeric_limits<IntN>::is_integer
  inline JSON &operator=(IntN integer) {
    _uptr = std::make_unique<Number>(static_cast<int64_t>(integer));
    return *this;
  }

  explicit JSON(double float_number)
      : _uptr(std::make_unique<Number>(float_number)) {}
  inline JSON &operator=(double float_number) {
    _uptr = std::make_unique<Number>(float_number);
    return *this;
  }

  explicit JSON(std::string str)
      : _uptr(std::make_unique<String>(std::move(str))) {}
  inline JSON &operator=(std::string str) {
    _uptr = std::make_unique<String>(std::move(str));
    return *this;
  }

  explicit JSON(const char *c_str) : _uptr(std::make_unique<String>(c_str)) {}
  inline JSON &operator=(const char *c_str) {
    _uptr = std::make_unique<String>(c_str);
    return *this;
  }

  std::unique_ptr<Node> &operator->() { return _uptr; }
  const std::unique_ptr<Node> &operator->() const { return _uptr; }

private:
  inline static auto getJSONParseError(std::string_view sv,
                                       const char *excepted) {
    auto bkg = sv.substr(0, 30);
    auto unexpected_token =
        sv[0] == 0 ? std::string("EOF") : std::format("`{}`", sv[0]);
    return JSONParseException(std::format(
        "Unexpected token {} at `{}{}` (excepted {})", unexpected_token, bkg,
        bkg.size() < 30 ? "" : "...", excepted));
  }

  inline static void removeWhiteSpaces(std::string_view &v) {
    auto ptr = std::ranges::find_if_not(v, [](char c) {
      return c == ' ' || c == '\n' || c == '\r' || c == '\t';
    });
    auto n = static_cast<size_t>(ptr - v.begin());
    v.remove_prefix(n);
  }

  static void assert_depth(std::string_view &sv, int current_depth) {
    if (current_depth > MAX_RECURSE_DEPTH) {
      throw getJSONParseError(sv, ".., max rescurse depth exceeded");
    }
  }

public:
  class Node {
  public:
    inline static std::unique_ptr<Node> parse(std::string_view &sv, int dep) {
      assert_depth(sv, dep);
      removeWhiteSpaces(sv);
      switch (sv[0]) {
      case 'n':
        return Null::parse(sv);
      case 't':
      case 'f':
        return Boolean::parse(sv);
      case '{':
        return Object::parse(sv, dep + 1);
      case '[':
        return Array::parse(sv, dep + 1);
      case '"':
        return String::parse(sv);
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return Number::parse(sv);
      default:
        throw getJSONParseError(sv, "any JSON value");
      }
    };

    virtual inline NodeType getType() const noexcept = 0;

    template <class T>
      requires std::is_base_of_v<Node, T>
    inline T &cast() noexcept {
      return *(static_cast<T *>(this));
    }
    template <class T>
      requires std::is_base_of_v<Node, T>
    inline const T &cast() const noexcept {
      return *(static_cast<const T *>(this));
    }
    virtual inline std::string dump() const {
      switch (this->getType()) {
      case NodeType::Null:
        return this->cast<Null>().dump();
      case NodeType::Boolean:
        return this->cast<Boolean>().dump();
      case NodeType::Number:
        return this->cast<Number>().dump();
      case NodeType::String:
        return this->cast<String>().dump();
      case NodeType::Array:
        return this->cast<Array>().dump();
      case NodeType::Object:
        return this->cast<Object>().dump();
      }
      throw JSONException("unreachable: a JSON::Node has no nodetype");
    };

    virtual ~Node() {};
  };

  class Null : public Node {
  public:
    inline static std::unique_ptr<Null> parse(std::string_view &sv) {
      removeWhiteSpaces(sv);
      if (sv.starts_with("null")) {
        sv.remove_prefix(4);
        return std::make_unique<Null>();
      }
      throw getJSONParseError(sv, "`null`");
    }
    inline NodeType getType() const noexcept override { return NodeType::Null; }
    inline std::string dump() const noexcept override { return "null"; }
  };

  class Boolean : public Node {
    bool value_;

  public:
    explicit Boolean(bool v) : value_(v) {};
    Boolean(Boolean &&v) = default;
    Boolean(const Boolean &v) = default;
    Boolean &operator=(Boolean &&v) = default;
    Boolean &operator=(const Boolean &v) = default;

    inline static std::unique_ptr<Boolean> parse(std::string_view &sv) {
      removeWhiteSpaces(sv);
      try {
        if (sv.starts_with("true")) {
          sv.remove_prefix(4);
          return std::make_unique<Boolean>(true);
        }
        if (sv.starts_with("false")) {
          sv.remove_prefix(5);
          return std::make_unique<Boolean>(false);
        }
      } catch (const std::out_of_range &e) {
        throw getJSONParseError(sv, "number, but get out of range");
      }
      throw getJSONParseError(sv, "`true` or `false`");
    }

    inline NodeType getType() const noexcept override {
      return NodeType::Boolean;
    }
    bool value() const { return value_; }
    inline std::string dump() const noexcept override {
      return value_ ? "true" : "false";
    }
  };

  class Number : public Node {
    int64_t value_int_{};
    double value_double_{};
    bool is_double_{};
    std::string str_raw_{};

  public:
    explicit Number(std::string str_raw, bool is_double)
        : is_double_(is_double), str_raw_(std::move(str_raw)) {
      if (is_double) {
        value_double_ = std::stod(str_raw_);
        value_int_ = value_double_;
      } else {
        value_int_ = std::stoll(str_raw_);
        value_double_ = value_int_;
      }
    }
    explicit Number(int64_t integer)
        : value_int_(integer), value_double_(integer), is_double_(false) {};
    explicit Number(double float_num)
        : value_int_(float_num), value_double_(float_num), is_double_(true) {};
    Number(Number &&) = default;
    Number(const Number &) = default;
    Number &operator=(Number &&) = default;
    Number &operator=(const Number &) = default;

    inline static std::unique_ptr<Number> parse(std::string_view &sv) {
      removeWhiteSpaces(sv);
      bool is_double = false;
      auto ptr = std::ranges::find_if_not(sv, [&is_double](char c) {
        if (c == '.') [[unlikely]] {
          return is_double = true;
        }
        return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == 'e' ||
               c == 'E';
      });
      auto n = static_cast<size_t>(ptr - sv.begin());
      std::string numsv(sv.substr(0, n));
      sv.remove_prefix(std::min(n, sv.size()));

      try {
        return std::make_unique<Number>(std::move(numsv), is_double);
      } catch (const std::exception &e) {
        if (!is_double) {
          try {
            return std::make_unique<Number>(std::string(sv.substr(0, n)), true);
          } catch (const std::exception &e) {
            throw getJSONParseError(sv, "but got out of range");
          }
        }
        throw getJSONParseError(sv, "but got out of range");
      }
    }

    int64_t value_int() const { return value_int_; }
    double value_double() const { return value_double_; }
    bool is_double() const { return is_double_; }

    inline void set(int64_t x) {
      is_double_ = false;
      str_raw_.clear();
      value_int_ = x;
    }
    inline void set(double d) {
      is_double_ = true;
      str_raw_.clear();
      value_double_ = d;
    }

    inline NodeType getType() const noexcept override {
      return NodeType::Number;
    }
    inline std::string dump() const noexcept override {
      return str_raw_.empty() ? is_double_ ? std::to_string(value_double_)
                                           : std::to_string(value_int_)
                              : str_raw_;
    }
  };

  class String : public Node {
    std::string value_;

    static void pushHexToUtf8(std::string &utf8, std::string_view hexString) {
      uint32_t codepoint;
      std::stringstream ss;
      ss << std::hex << hexString;
      ss >> codepoint;
      if (codepoint <= 0x7F) {
        utf8.push_back(static_cast<char>(codepoint));
      } else if (codepoint <= 0x7FF) {
        utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      } else if (codepoint <= 0xFFFF) {
        utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      } else if (codepoint <= 0x10FFFF) {
        utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
      }
    }

  public:
    explicit String(std::string str) : value_(std::move(str)) {};
    String(String &&) = default;
    String(const String &) = default;
    String &operator=(String &&) = default;
    String &operator=(const String &) = default;

    inline static std::unique_ptr<String> parse(std::string_view &sv) {
      removeWhiteSpaces(sv);
      if (sv[0] != '"')
        throw getJSONParseError(sv, "string start `\"`");
      sv.remove_prefix(1);
      std::string res;
      size_t n = 1;
      while (true) {
        auto ptr = std::ranges::find_if(
            sv, [](char c) { return c == '\\' || c == '"'; });
        n = ptr - sv.begin();

        for (auto c : sv.substr(0, n)) {
          switch (c) {
          case '\n':
            throw getJSONParseError(sv, "string end `\"`");
          case '\t':
          case '\0':
            throw getJSONParseError(sv, "no control char in string");
          }
        }

        res += sv.substr(0, n);
        sv.remove_prefix(n);

        if (sv.empty()) {
          throw getJSONParseError(sv, "string end `\"`");
        } else if (sv[0] == '"') {
          sv.remove_prefix(1);
          return std::make_unique<String>(std::move(res));
        } else {
          sv.remove_prefix(1);
          switch (sv[0]) {
          case '\\':
            sv.remove_prefix(1);
            res += '\\';
            break;
          case '"':
            sv.remove_prefix(1);
            res += '"';
            break;
          case '/':
            sv.remove_prefix(1);
            res += '/';
            break;
          case 'b':
            sv.remove_prefix(1);
            res += '\b';
            break;
          case 'f':
            sv.remove_prefix(1);
            res += '\f';
            break;
          case 'n':
            sv.remove_prefix(1);
            res += '\n';
            break;
          case 'r':
            sv.remove_prefix(1);
            res += '\r';
            break;
          case 't':
            sv.remove_prefix(1);
            res += '\t';
            break;
          case 'u': {
            sv.remove_prefix(1);
            auto hexs = sv.substr(0, 4);
            sv.remove_prefix(4);
            for (auto c : hexs) {
              if (not((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
                      (c >= '0' && c <= '9'))) {
                throw getJSONParseError(
                    sv, "[0-9a-fA-F] but got bad Unicode escape");
              }
            }
            pushHexToUtf8(res, hexs);
            break;
          }
          default:
            throw getJSONParseError(sv, "escape character");
          }
        }
      }
    }

    static std::string toJSONString(const std::string &s) {
      std::string res = "\"";
      for (const auto c : s) {
        switch (c) {
        case '\\':
          res += "\\\\";
          break;
        case '\n':
          res += "\\n";
          break;
        case '\b':
          res += "\\b";
          break;
        case '\f':
          res += "\\f";
          break;
        case '\r':
          res += "\\r";
          break;
        case '\t':
          res += "\\t";
          break;
        case '"':
          res += "\\\"";
          break;
        default:
          res += c;
          break;
        }
      }
      res += '"';
      return res;
    }

    const std::string &value() const { return value_; }
    std::string take() { return std::move(value_); }

    template <typename T> void set(T &&v) { value_ = std::forward<T>(v); }

    inline NodeType getType() const noexcept override {
      return NodeType::String;
    }
    inline std::string dump() const override { return toJSONString(value_); }
  };

  class Array : public Node {
    using ArrayVT = std::vector<JSON>;
    ArrayVT value_{};

    static void pushArray_(Array &) {}
    template <typename T> static void pushArray_(Array &arr, T &&t) {
      arr.value_.emplace_back(JSON(std::forward<T>(t)));
    }
    template <typename T, typename... Args>
    static void pushArray_(Array &arr, T &&t, Args &&...args) {
      arr.value_.emplace_back(JSON(std::forward<T>(t)));
      pushArray_(arr, std::forward<Args>(args)...);
    }

  public:
    Array() {};
    explicit Array(ArrayVT &&val) : value_(std::move(val)) {};
    Array(Array &&) = default;
    Array(const Array &) = delete;
    Array &operator=(Array &&) = default;
    Array &operator=(const Array &) = delete;

    template <typename... Args> static Array makeArray(Args &&...args) {
      Array res;
      pushArray_(res, std::forward<Args>(args)...);
      return res;
    }

    inline static std::unique_ptr<Array> parse(std::string_view &sv, int dep) {
      assert_depth(sv, dep);
      removeWhiteSpaces(sv);
      if (sv[0] != '[')
        throw getJSONParseError(sv, "array start `[`");
      sv.remove_prefix(1);
      removeWhiteSpaces(sv);

      ArrayVT val;
      bool isTComma = false;

      while (sv[0] != ']') {
        val.emplace_back(Node::parse(sv, dep + 1));
        removeWhiteSpaces(sv);
        switch (sv[0]) {
        case ']':
          sv.remove_prefix(1);
          return std::make_unique<Array>(std::move(val));
        case ',':
          isTComma = true;
          sv.remove_prefix(1);
          continue;
        default:
          throw getJSONParseError(sv, "array spliter `,` or array end `]`");
        }
      }

      if (isTComma && !ENABLE_TRAILING_COMMA)
        throw getJSONParseError(sv, "next json value");

      sv.remove_prefix(1);
      return std::make_unique<Array>(std::move(val));
    }

    inline NodeType getType() const noexcept override {
      return NodeType::Array;
    }
    inline std::string dump() const override {
      std::string s = "[";
      for (const auto &v : value_) {
        s += v->dump();
        s += ",";
      }
      if (!value_.empty())
        s.pop_back();
      s += ']';
      return s;
    }

    JSON &operator[](size_t idx) { return value_.at(idx); }
  };

  class Object : public Node {
    using ObjectVT = std::unordered_map<std::string, JSON>;
    ObjectVT value_{};

  public:
    Object() {};
    explicit Object(ObjectVT &&val) : value_(std::move(val)) {};
    Object(Object &&) = default;
    Object(const Object &) = delete;
    Object &operator=(Object &&) = default;
    Object &operator=(const Object &) = delete;

    inline static std::unique_ptr<Object> parse(std::string_view &sv, int dep) {
      assert_depth(sv, dep);
      removeWhiteSpaces(sv);
      if (sv[0] != '{')
        throw getJSONParseError(sv, "object start `{`");
      sv.remove_prefix(1);
      removeWhiteSpaces(sv);
      ObjectVT val;
      bool isTComma = false;
      while (sv[0] != '}') {
        auto key = String::parse(sv);
        if (ENABLE_DUMPLICATED_KEY_DETECT && val.contains(key->value())) {
          throw getJSONParseError(
              sv, ("unique key, but got dumplicated key `" + key->value() + "`")
                      .c_str());
        } else {
          removeWhiteSpaces(sv);
          if (sv[0] != ':')
            throw getJSONParseError(sv, "object spliter `:`");
          sv.remove_prefix(1);
          val.insert({key->take(), JSON(Node::parse(sv, dep + 1))});
          removeWhiteSpaces(sv);
          switch (sv[0]) {
          case '}':
            sv.remove_prefix(1);
            return std::make_unique<Object>(std::move(val));
          case ',':
            sv.remove_prefix(1);
            isTComma = true;
            continue;
          default:
            throw getJSONParseError(sv, "object spliter `,` or object end `}`");
          }
        }
      }
      if (isTComma && !ENABLE_TRAILING_COMMA)
        throw getJSONParseError(sv, "next json value");
      sv.remove_prefix(1);
      return std::make_unique<Object>(std::move(val));
    }

    inline NodeType getType() const noexcept override {
      return NodeType::Object;
    }
    inline std::string dump() const override {
      std::string s = "{";
      for (const auto &[key, val] : value_) {
        s += String::toJSONString(key);
        s += ":";
        s += val->dump();
        s += ",";
      }
      if (!value_.empty())
        s.pop_back();
      s += '}';
      return s;
    }

    JSON &operator[](const std::string &s) { return value_[s]; }
  };

public:
  static JSON parse(std::string_view sv) {
    auto res = Node::parse(sv, 0);
    removeWhiteSpaces(sv);
    if (!sv.empty()) {
      throw getJSONParseError(sv, "EOF");
    }
    return JSON(std::move(res));
  }
};
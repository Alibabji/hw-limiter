#pragma once

#include <cctype>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jsonlite {

enum class Type {
    Null,
    Bool,
    Number,
    String,
    Object,
    Array
};

struct Value {
    Type type = Type::Null;
    double number = 0.0;
    bool boolean = false;
    std::string string;
    std::map<std::string, Value> object;
    std::vector<Value> array;

    bool IsNull() const { return type == Type::Null; }
    bool IsBool() const { return type == Type::Bool; }
    bool IsNumber() const { return type == Type::Number; }
    bool IsString() const { return type == Type::String; }
    bool IsObject() const { return type == Type::Object; }
    bool IsArray() const { return type == Type::Array; }

    const Value& operator[](const std::string& key) const {
        static Value nullValue;
        auto it = object.find(key);
        if (it == object.end()) {
            return nullValue;
        }
        return it->second;
    }

    const Value& operator[](size_t index) const {
        static Value nullValue;
        if (index >= array.size()) {
            return nullValue;
        }
        return array[index];
    }

    std::string GetString(const std::string& fallback = {}) const {
        return IsString() ? string : fallback;
    }

    double GetNumber(double fallback = 0.0) const {
        return IsNumber() ? number : fallback;
    }

    bool GetBool(bool fallback = false) const {
        return IsBool() ? boolean : fallback;
    }
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
public:
    explicit Parser(std::string_view input) : source_(input) {}

    Value Parse() {
        SkipWhitespace();
        auto value = ParseValue();
        SkipWhitespace();
        if (pos_ != source_.size()) {
            throw ParseError("Unexpected trailing characters in JSON");
        }
        return value;
    }

private:
    char Peek() const {
        return pos_ < source_.size() ? source_[pos_] : '\0';
    }

    char Get() {
        return pos_ < source_.size() ? source_[pos_++] : '\0';
    }

    bool Match(char expected) {
        if (Peek() == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    void SkipWhitespace() {
        while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[pos_]))) {
            ++pos_;
        }
    }

    Value ParseValue() {
        SkipWhitespace();
        char c = Peek();
        if (c == '\0') {
            throw ParseError("Unexpected end of JSON input");
        }
        if (c == '"') {
            Value v;
            v.type = Type::String;
            v.string = ParseString();
            return v;
        }
        if (c == '{') {
            return ParseObject();
        }
        if (c == '[') {
            return ParseArray();
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            Value v;
            v.type = Type::Number;
            v.number = ParseNumber();
            return v;
        }
        if (c == 't') {
            ExpectLiteral("true");
            Value v;
            v.type = Type::Bool;
            v.boolean = true;
            return v;
        }
        if (c == 'f') {
            ExpectLiteral("false");
            Value v;
            v.type = Type::Bool;
            v.boolean = false;
            return v;
        }
        if (c == 'n') {
            ExpectLiteral("null");
            return Value{};
        }
        throw ParseError("Unrecognized value in JSON");
    }

    void ExpectLiteral(std::string_view literal) {
        for (char expected : literal) {
            if (Get() != expected) {
                throw ParseError("Malformed literal in JSON");
            }
        }
    }

    Value ParseObject() {
        Value objectValue;
        objectValue.type = Type::Object;
        objectValue.object.clear();
        Get(); // consume '{'
        SkipWhitespace();
        if (Match('}')) {
            return objectValue;
        }
        while (true) {
            SkipWhitespace();
            if (Peek() != '"') {
                throw ParseError("Expected string key in JSON object");
            }
            std::string key = ParseString();
            SkipWhitespace();
            if (!Match(':')) {
                throw ParseError("Expected ':' after object key");
            }
            SkipWhitespace();
            Value value = ParseValue();
            objectValue.object.emplace(std::move(key), std::move(value));
            SkipWhitespace();
            if (Match('}')) {
                break;
            }
            if (!Match(',')) {
                throw ParseError("Expected ',' between object members");
            }
        }
        return objectValue;
    }

    Value ParseArray() {
        Value arrayValue;
        arrayValue.type = Type::Array;
        arrayValue.array.clear();
        Get(); // consume '['
        SkipWhitespace();
        if (Match(']')) {
            return arrayValue;
        }
        while (true) {
            Value entry = ParseValue();
            arrayValue.array.emplace_back(std::move(entry));
            SkipWhitespace();
            if (Match(']')) {
                break;
            }
            if (!Match(',')) {
                throw ParseError("Expected ',' between array elements");
            }
        }
        return arrayValue;
    }

    std::string ParseString() {
        if (!Match('"')) {
            throw ParseError("Expected beginning of string");
        }
        std::string result;
        while (true) {
            if (pos_ >= source_.size()) {
                throw ParseError("Unterminated string literal");
            }
            char c = Get();
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (pos_ >= source_.size()) {
                    throw ParseError("Bad escape sequence in string");
                }
                char esc = Get();
                switch (esc) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u': {
                        result.push_back(ParseUnicodeEscape());
                        break;
                    }
                    default:
                        throw ParseError("Invalid escape character in string");
                }
            } else {
                result.push_back(c);
            }
        }
        return result;
    }

    char ParseUnicodeEscape() {
        if (pos_ + 4 > source_.size()) {
            throw ParseError("Invalid unicode escape");
        }
        int value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = Get();
            value <<= 4;
            if (c >= '0' && c <= '9') {
                value += c - '0';
            } else if (c >= 'a' && c <= 'f') {
                value += c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                value += c - 'A' + 10;
            } else {
                throw ParseError("Invalid hex digit in unicode escape");
            }
        }
        if (value <= 0x7F) {
            return static_cast<char>(value);
        }
        // For simplicity we only handle BMP characters <= 0x7F in this lightweight parser.
        return '?';
    }

    double ParseNumber() {
        size_t start = pos_;
        if (Match('-')) {}
        while (std::isdigit(static_cast<unsigned char>(Peek()))) {
            ++pos_;
        }
        if (Match('.')) {
            while (std::isdigit(static_cast<unsigned char>(Peek()))) {
                ++pos_;
            }
        }
        if (Peek() == 'e' || Peek() == 'E') {
            ++pos_;
            if (Peek() == '+' || Peek() == '-') {
                ++pos_;
            }
            while (std::isdigit(static_cast<unsigned char>(Peek()))) {
                ++pos_;
            }
        }
        auto numberStr = std::string(source_.substr(start, pos_ - start));
        try {
            return std::stod(numberStr);
        } catch (const std::exception&) {
            throw ParseError("Invalid numeric literal in JSON");
        }
    }

    std::string_view source_;
    size_t pos_ = 0;
};

inline Value Parse(std::string_view text) {
    Parser parser(text);
    return parser.Parse();
}

}  // namespace jsonlite

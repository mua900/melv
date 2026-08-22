#ifndef VALUE_HPP
#define VALUE_HPP

#include "math_util.hpp"

namespace melv {

enum class ValueType {
    ZERO = 0,
    // STRING,
    INTEGER,
    REAL,
    BOOLEAN,
    VECTOR2,
};

struct Value
{
    ValueType type = {};

    union {
        // @todo who stores this?
        // String string;
        bool boolean;
        s64 integer;
        double real;
        vec2 v2;
    } data = {};

    Value() {}
    Value(s64 s) : type(ValueType::INTEGER) {
        data.integer = s;
    }
    Value(double r) : type(ValueType::REAL) {
        data.real = r;
    }
    Value(bool b) : type(ValueType::BOOLEAN) {
        data.boolean = b;
    }
    Value(vec2 v) : type(ValueType::VECTOR2) {
        data.v2 = v;
    }
};

} // namespace

#endif // VALUE_HPP
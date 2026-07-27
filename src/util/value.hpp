#ifndef VALUE_HPP
#define VALUE_HPP

namespace melv {

struct Value
{
    enum {
        STRING,
        INTEGER,
        REAL
    } type;

    union {
        // @todo who stores this?
        // String string;
        s64 integer;
        double real;
    } data = {};

    Value() {}
    Value(s64 s) : type(INTEGER) {
        data.integer = s;
    }
    Value(double r) : type(REAL) {
        data.real = r;
    }
};

} // namespace

#endif // VALUE_HPP
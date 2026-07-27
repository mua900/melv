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
        String string;
        s64 integer;
        double real;
    } data = {};
};

} // namespace

#endif // VALUE_HPP
#ifndef ACTION_SET_HPP
#define ACTION_SET_HPP

#include "input.hpp"
#include <util/hash_table.hpp>
#include <util/value.hpp>

namespace melv {

    #define ACTION_MAX_INPUT 4

    struct Action
    {
        Input input[ACTION_MAX_INPUT] = {};
        int num_input = 0;

        void add_input(Input& input);
        bool read_value(InputState& state, Value& out);
    };

    struct ActionSet
    {
        HashTable<Action> action_set = {};

        void add_action(const char* name, Input& input);
        bool read_action(InputState& input, const char* name, Value& out);
    };

} // namespace

#endif // ACTION_SET_HPP
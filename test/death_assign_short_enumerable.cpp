#include "common.hpp"

struct finite_enumerable {
    int count;

    struct cursor {
        int index = 0, count = 0;
        bool ok() const { return index < count; }
        int val() const { return index; }
        int idx() const { return index; }
        void next() { ++index; }
    };

    cursor enumerate() const { return {0, count}; }
};

int main() {
    nvector<int> destination(3);
    nassign(destination, finite_enumerable{2});
}

#include "common.hpp"

int main() {
    nmaxflow<unsigned char> flow(3);
    flow.add(0, 1, 200);
    flow.add(0, 1, 100);
    return flow.flow(0, 2);
}

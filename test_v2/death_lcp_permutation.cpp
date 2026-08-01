#include "common.hpp"

int main() {
    string text = "ab";
    return nlcp_array(text, nvector<int>{0, 0}).len();
}

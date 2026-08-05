#include "common.hpp"

int main() {
    vector<int> values;
    int count_calls = 0;
    nrep(i, (++count_calls, 4)) values.push_back(i);
    ntest(count_calls == 1);
    ntest((values == vector<int>{0, 1, 2, 3}));

    values.clear();
    nrrep(i, (++count_calls, 4)) values.push_back(i);
    ntest(count_calls == 2);
    ntest((values == vector<int>{3, 2, 1, 0}));

    int repeat_break = 0;
    nrep(i, 10) {
        ++repeat_break;
        if (i == 2)
            break;
    }
    ntest(repeat_break == 3);

    int reverse_break = 0;
    nrrep(i, 10) {
        ++reverse_break;
        if (i == 7)
            break;
    }
    ntest(reverse_break == 3);

    int negative_repeat = 0;
    nrep(i, -3) negative_repeat += i;
    nrrep(i, -3) negative_repeat += i;
    ntest(negative_repeat == 0);

    values.clear();
    nfor(x, nrange(2, 9, 2)) values.push_back(x);
    ntest((values == vector<int>{2, 4, 6, 8}));

    values.clear();
    nfor(x, nrange(8, 1, -2)) values.push_back(x);
    ntest((values == vector<int>{8, 6, 4, 2}));

    nvector<int> a{4, 7, 9};
    int encoded = 0;
    nfori(i, x, a) encoded += (i + 1) * x;
    ntest(encoded == 45);

    int visited = 0;
    nfor(x, a) {
        ++visited;
        if (x == 7)
            break;
    }
    ntest(visited == 2);

    int indexed_visited = 0;
    nfori(i, x, a) {
        (void)x;
        ++indexed_visited;
        if (i == 1)
            break;
    }
    ntest(indexed_visited == 2);

    int even_sum = 0;
    nfor(x, nrange(7)) {
        if (x % 2)
            continue;
        even_sum += x;
    }
    ntest(even_sum == 12);

    int nested_visits = 0;
    nfor(row, nrange(3)) {
        (void)row;
        nfor(column, nrange(4)) {
            (void)column;
            ++nested_visits;
            break;
        }
    }
    ntest(nested_visits == 3);

    nvector<pair<int, int>> buffer{{1, 4}, {2, 5}, {3, 6}};
    int scanned = 0, chosen = -1, weight = 0;
    nfor(edge, buffer) {
        auto [vertex, edge_weight] = edge;
        ++scanned;
        if (vertex == 2) {
            chosen = vertex;
            weight = edge_weight;
            break;
        }
    }
    ntest(scanned == 2 && chosen == 2 && weight == 5);

    bool else_reached = false;
    if (false)
        nfor(x, a) (void)x;
    else
        else_reached = true;
    ntest(else_reached);

    values.clear();
    nfor(x, nreverse(a)) values.push_back(x);
    ntest((values == vector<int>{9, 7, 4}));

    struct item {
        int key, payload;
    };
    nvector<item> items{{3, 30}, {1, 10}, {2, 20}};
    auto keys = nproject(items, [](item& x) -> int& { return x.key; });
    nsort(keys);
    ntest(items[0].key == 1 && items[1].key == 2 && items[2].key == 3);
    ntest(items[0].payload == 30 && items[1].payload == 10 && items[2].payload == 20);
}

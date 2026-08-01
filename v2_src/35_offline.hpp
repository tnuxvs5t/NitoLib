struct ninterval_query {
    int left, right, id;
};

template <nindexed Q> nvector<int> nmo_order(const Q& queries, int universe) {
    npre(universe >= 0);
    int count = nlen(queries);
    int block = max(1, int(sqrt(double(max(1, universe)))));
    nvector<int> order(count);
    for (int i = 0; i < count; ++i) {
        npre(0 <= queries[i].left && queries[i].left <= queries[i].right && queries[i].right <= universe);
        order[i] = i;
    }
    nsort(order, [&](int x, int y) {
        int block_x = queries[x].left / block;
        int block_y = queries[y].left / block;
        if (block_x != block_y)
            return block_x < block_y;
        return block_x & 1 ? queries[y].right < queries[x].right : queries[x].right < queries[y].right;
    });
    return order;
}

template <nindexed Q, class AddLeft, class AddRight, class RemoveLeft, class RemoveRight, class Answer>
void nrun_mo(const Q& queries, int universe, AddLeft add_left, AddRight add_right, RemoveLeft remove_left,
             RemoveRight remove_right, Answer answer) {
    auto order = nmo_order(queries, universe);
    int left = 0, right = 0;
    for (int position = 0; position < order.len(); ++position) {
        int query_index = order[position];
        const auto& query = queries[query_index];
        while (query.left < left)
            invoke(add_left, --left);
        while (right < query.right)
            invoke(add_right, right++);
        while (left < query.left)
            invoke(remove_left, left++);
        while (query.right < right)
            invoke(remove_right, --right);
        invoke(answer, query.id);
    }
}

template <nindexed Q, class Add, class Remove, class Answer>
void nrun_mo(const Q& queries, int universe, Add add, Remove remove, Answer answer) {
    nrun_mo(queries, universe, add, add, remove, remove, move(answer));
}

enum class nbranch : unsigned char { left, take, right };

template <class S> class nnode {
  public:
    using value_type = typename S::value_type;
    using info_type = typename S::info_type;

  private:
    const S* owner_ = nullptr;
    int handle_ = 0;
    uint64_t epoch_ = 0;

    constexpr nnode(const S* owner, int handle, uint64_t epoch)
        : owner_(owner), handle_(handle), epoch_(epoch) {}
    friend S;

  public:
    constexpr nnode() = default;

    bool current() const { return owner_ && epoch_ == owner_->nnode_epoch(); }
    bool ok() const { return current() && handle_ && owner_->nnode_alive(handle_); }
    explicit operator bool() const { return ok(); }

    const value_type& val() const {
        npre(ok());
        return owner_->nnode_val(handle_);
    }
    int count() const {
        npre(current());
        return owner_->nnode_count(handle_);
    }
    int len() const {
        npre(current());
        return owner_->nnode_len(handle_);
    }
    info_type info() const {
        npre(current());
        return owner_->nnode_info(handle_);
    }
    nnode left() const {
        npre(current());
        return {owner_, owner_->nnode_left(handle_), epoch_};
    }
    nnode right() const {
        npre(current());
        return {owner_, owner_->nnode_right(handle_), epoch_};
    }
    int handle() const noexcept { return handle_; }
};

template <class S>
concept nnode_tree = requires(const S& tree) {
    { tree.root() } -> same_as<nnode<S>>;
};

template <nnode_tree S, class F> nnode<S> nwalk(const S& tree, F&& decide) {
    auto node = tree.root();
    while (node) {
        nbranch branch = invoke(decide, node);
        if (branch == nbranch::left)
            node = node.left();
        else if (branch == nbranch::right)
            node = node.right();
        else {
            npre(branch == nbranch::take);
            return node;
        }
    }
    return node;
}

template <class A, class T>
concept naugment = copyable<typename A::info_type> &&
                   requires(const A& augment, const T& value, int count,
                            typename A::info_type info) {
                       { augment.id() } -> convertible_to<typename A::info_type>;
                       { augment.one(value, count) } -> convertible_to<typename A::info_type>;
                       { augment.op(info, info) } -> convertible_to<typename A::info_type>;
                   };

template <class T> struct nempty_augment {
    using info_type = monostate;
    constexpr info_type id() const { return {}; }
    constexpr info_type one(const T&, int) const { return {}; }
    constexpr info_type op(info_type, info_type) const { return {}; }
};

template <class S>
concept naugmented_tree = nnode_tree<S> && requires(const S& tree) {
    typename S::augment_type;
    { tree.augment() } -> same_as<const typename S::augment_type&>;
};

template <naugmented_tree S, class P> nnode<S> nfirst_prefix(const S& tree, P&& predicate) {
    const auto& augment = tree.augment();
    auto node = tree.root();
    auto prefix = augment.id();
    while (node) {
        auto through_left = augment.op(prefix, node.left().info());
        if (invoke(predicate, through_left)) {
            node = node.left();
            continue;
        }
        auto through_node = augment.op(through_left, augment.one(node.val(), node.count()));
        if (invoke(predicate, through_node))
            return node;
        prefix = move(through_node);
        node = node.right();
    }
    return node;
}

template <naugmented_tree S, class P> nnode<S> nlast_suffix(const S& tree, P&& predicate) {
    const auto& augment = tree.augment();
    auto node = tree.root();
    auto suffix = augment.id();
    while (node) {
        auto through_right = augment.op(node.right().info(), suffix);
        if (invoke(predicate, through_right)) {
            node = node.right();
            continue;
        }
        auto through_node = augment.op(augment.one(node.val(), node.count()), through_right);
        if (invoke(predicate, through_node))
            return node;
        suffix = move(through_node);
        node = node.left();
    }
    return node;
}

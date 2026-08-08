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

    template <class Q = S>
        requires requires(const Q& owner, int handle) {
            typename Q::tag_type;
            { owner.nnode_tag(handle) } -> convertible_to<typename Q::tag_type>;
        }
    typename Q::tag_type tag() const {
        npre(current());
        return owner_->nnode_tag(handle_);
    }
};

template <class S, class F> nnode<S> nwalk(const S& tree, F&& decide) {
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

template <class T> struct nempty_augment {
    using info_type = monostate;
    constexpr info_type id() const { return {}; }
    constexpr info_type one(const T&, int) const { return {}; }
    constexpr info_type op(info_type, info_type) const { return {}; }
};

// Default no-op action used by AST containers.  A tree can opt into lazy tags
// by supplying an action with the same four operations and a `tag_type` alias.
template <class T, class I> struct nempty_tag {
    using tag_type = monostate;
    constexpr tag_type tag_id() const { return {}; }
    constexpr tag_type compose(const tag_type&, const tag_type&) const { return {}; }
    constexpr T apply_value(T value, const tag_type&, int) const { return value; }
    constexpr I apply_info(I info, const tag_type&, int) const { return info; }
};

template <class S, class P> nnode<S> nfirst_prefix(const S& tree, P&& predicate) {
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

template <class S, class P> nnode<S> nlast_suffix(const S& tree, P&& predicate) {
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

// Interval-tree node view.  The view stores the interval bounds because an
// open/dynamic tree does not have to allocate absent children.  An absent child
// contributes the operation identity; a lazy view additionally applies any
// carried ancestor tag without materializing that child.
template <class S> class nseg_node {
  public:
    using aggregate_type = typename S::aggregate_type;
    using state_type = typename S::nseg_state_type;

  private:
    const S* owner_ = nullptr;
    int handle_ = 0;
    uint64_t epoch_ = 0;
    long long left_ = 0, right_ = 0;
    state_type carry_{};

    constexpr nseg_node(const S* owner, int handle, uint64_t epoch, long long left,
                        long long right, state_type carry = {})
        : owner_(owner), handle_(handle), epoch_(epoch), left_(left), right_(right),
          carry_(move(carry)) {}
    friend S;

    static constexpr bool has_lazy_view = requires(const S& owner, int handle,
                                                   aggregate_type aggregate,
                                                   const typename S::tag_type& tag, int length) {
        { owner.nseg_pending(handle) } -> convertible_to<bool>;
        { owner.nseg_tag(handle) } -> convertible_to<typename S::tag_type>;
        { owner.nseg_compose(tag, tag) } -> convertible_to<typename S::tag_type>;
        { owner.nseg_apply(move(aggregate), tag, length) } -> convertible_to<aggregate_type>;
    };

    state_type child_carry() const {
        state_type result = carry_;
        if constexpr (has_lazy_view) {
            using tag_type = typename S::tag_type;
            if (owner_->nseg_pending(handle_)) {
                tag_type local = owner_->nseg_tag(handle_);
                result = result ? state_type(owner_->nseg_compose(*result, local))
                                : state_type(move(local));
            }
        }
        return result;
    }

  public:
    constexpr nseg_node() = default;

    bool current() const { return owner_ && epoch_ == owner_->nseg_epoch(); }
    bool ok() const { return current() && handle_ && owner_->nseg_alive(handle_); }
    explicit operator bool() const { return ok(); }

    aggregate_type aggregate() const {
        npre(current());
        aggregate_type result = owner_->nseg_aggregate(handle_);
        if constexpr (has_lazy_view) {
            if (carry_)
                result = owner_->nseg_apply(move(result), *carry_, ni::nchecked_int(width()));
        }
        return result;
    }
    aggregate_type info() const { return aggregate(); }
    long long left_bound() const {
        npre(current());
        return left_;
    }
    long long right_bound() const {
        npre(current());
        return right_;
    }
    long long width() const {
        npre(current());
        npre(left_ <= right_);
        __int128_t result = __int128_t(right_) - left_;
        npre(result <= __int128_t(LLONG_MAX));
        return static_cast<long long>(result);
    }
    bool leaf() const {
        npre(current());
        return width() == 1;
    }
    nseg_node left() const {
        npre(current());
        npre(width() > 1);
        long long middle = left_ + (__int128_t(right_) - left_) / 2;
        return {owner_, owner_->nseg_left(handle_), epoch_, left_, middle, child_carry()};
    }
    nseg_node right() const {
        npre(current());
        npre(width() > 1);
        long long middle = left_ + (__int128_t(right_) - left_) / 2;
        return {owner_, owner_->nseg_right(handle_), epoch_, middle, right_, child_carry()};
    }
    int handle() const noexcept { return handle_; }

    template <class Q = S>
        requires requires(const Q& owner, int handle) {
            typename Q::tag_type;
            { owner.nseg_tag(handle) } -> convertible_to<typename Q::tag_type>;
        }
    typename Q::tag_type tag() const {
        npre(current());
        return owner_->nseg_tag(handle_);
    }
};

template <class S, class F> nseg_node<S> nseg_walk(nseg_node<S> node, F&& decide) {
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

template <class S, class F> nseg_node<S> nseg_walk(const S& tree, F&& decide) {
    return nseg_walk(tree.root(), forward<F>(decide));
}

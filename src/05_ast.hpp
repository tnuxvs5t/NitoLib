// Stable node identity shared by every owner/view layer.  The domain owns the
// meaning of the handle and generation; this record deliberately carries no tree
// or graph semantics.
struct nnode_identity {
    const void* domain = nullptr;
    int handle = 0;
    uint64_t generation = 0;

    explicit operator bool() const noexcept { return domain && handle && generation; }
    friend bool operator==(const nnode_identity&, const nnode_identity&) = default;
};

enum class nbranch : unsigned char { left, take, right };

/**
 * Non-owning AST node snapshot supplied by a tree owner `S`.
 * `S` provides the private nnode_* accessors and an epoch; topology-changing owner
 * operations invalidate old snapshots.  A zero handle is a valid empty subtree and
 * still supports count/len/info, but not val().
 */
template <class S> class nnode_view {
  public:
    using value_type = typename S::value_type;
    using info_type = typename S::info_type;

  private:
    const S* owner_ = nullptr;
    const void* domain_ = nullptr;
    int handle_ = 0;
    uint64_t epoch_ = 0;

    static const void* domain_token_of(const S* owner) noexcept {
        if (!owner)
            return nullptr;
        if constexpr (requires(const S& value) { value.nnode_domain_token(); })
            return owner->nnode_domain_token();
        return owner;
    }

    nnode_view(const S* owner, int handle, uint64_t epoch)
        : owner_(owner), domain_(domain_token_of(owner)), handle_(handle), epoch_(epoch) {}
    friend S;

  public:
    constexpr nnode_view() = default;

    bool current() const {
        return owner_ && domain_ == domain_token_of(owner_) && epoch_ == owner_->nnode_epoch();
    }
    bool ok() const { return current() && handle_ && owner_->nnode_alive(handle_); }
    explicit operator bool() const { return ok(); }

    const S& owner() const {
        npre(current());
        return *owner_;
    }
    bool same_owner(const nnode_view& other) const noexcept { return owner_ == other.owner_; }
    bool same_domain(const nnode_view& other) const noexcept {
        return domain_ && domain_ == other.domain_;
    }
    nnode_identity identity() const {
        npre(current());
        if constexpr (requires(const S& owner, int handle) {
                          owner.nnode_identity_of(handle);
                      }) {
            return owner_->nnode_identity_of(handle_);
        } else {
            return {domain_, handle_, handle_ ? 1u : 0u};
        }
    }

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
    nnode_view left() const {
        npre(current());
        return {owner_, owner_->nnode_left(handle_), epoch_};
    }
    nnode_view right() const {
        npre(current());
        return {owner_, owner_->nnode_right(handle_), epoch_};
    }
    bool leaf() const {
        npre(current());
        return handle_ && !owner_->nnode_left(handle_) && !owner_->nnode_right(handle_);
    }
    int handle() const noexcept { return handle_; }

    template <class Q = S>
        requires requires(const Q& owner, int handle) {
            { owner.nnode_parent(handle) } -> convertible_to<int>;
        }
    nnode_view parent() const {
        npre(current());
        return {owner_, owner_->nnode_parent(handle_), epoch_};
    }

    template <class Q = S>
        requires requires(const Q& owner, int handle) {
            typename Q::tag_type;
            { owner.nnode_tag(handle) } -> convertible_to<typename Q::tag_type>;
        }
    typename Q::tag_type tag() const {
        npre(current());
        return owner_->nnode_tag(handle_);
    }

    template <class Q = S>
        requires requires(const Q& owner, int handle) {
            typename Q::state_type;
            { owner.nnode_state(handle) } -> convertible_to<typename Q::state_type>;
        }
    typename Q::state_type state() const {
        npre(ok());
        return owner_->nnode_state(handle_);
    }
};

// `nnode<S>` was the original public spelling.  Keep it as a source-compatible
// alias while making the role explicit for new code: identity/lifetime belongs to
// the owner domain, and this type is the borrowed structural view.
template <class S> using nnode = nnode_view<S>;

// Walk may start at an owner root or at any current subtree snapshot.  The decision
// must eventually take a node or descend toward an existing child.
template <class S, class F> nnode<S> nwalk(nnode<S> node, F&& decide) {
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

template <class S, class F> nnode<S> nwalk(const S& tree, F&& decide) {
    return nwalk(tree.root(), forward<F>(decide));
}

// Augmentation objects provide `info_type`, id(), one(value,count), and associative
// ordered op(left,right).  nempty_augment is the no-information implementation.
template <class T> struct nempty_augment {
    using info_type = monostate;
    constexpr info_type id() const { return {}; }
    constexpr info_type one(const T&, int) const { return {}; }
    constexpr info_type op(info_type, info_type) const { return {}; }
};

// Node actions provide tag_type/tag_id/compose/apply_value/apply_info.
// compose(newer, older) means "older, then newer"; it must be associative and
// compatible with both per-key multiplicity and subtree aggregate length.
// nempty_tag is the default no-op implementation.
template <class T, class I> struct nempty_tag {
    using tag_type = monostate;
    constexpr tag_type tag_id() const { return {}; }
    constexpr tag_type compose(const tag_type&, const tag_type&) const { return {}; }
    constexpr T apply_value(T value, const tag_type&, int) const { return value; }
    constexpr I apply_info(I info, const tag_type&, int) const { return info; }
};

// Prefix/suffix descent assumes the predicate is monotone as the ordered aggregate
// grows.  Without that semantic property the returned node is not the first/last hit.
template <class S, class P> nnode<S> nfirst_prefix(nnode<S> node, P&& predicate) {
    const auto& augment = node.owner().augment();
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


template <class S, class P> nnode<S> nfirst_prefix(const S& tree, P&& predicate) {
    return nfirst_prefix(tree.root(), forward<P>(predicate));
}

template <class S, class P> nnode<S> nlast_suffix(nnode<S> node, P&& predicate) {
    const auto& augment = node.owner().augment();
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


template <class S, class P> nnode<S> nlast_suffix(const S& tree, P&& predicate) {
    return nlast_suffix(tree.root(), forward<P>(predicate));
}

/**
 * Non-owning interval-tree snapshot shared by fixed, persistent and dynamic trees.
 * The owner supplies nseg_* accessors plus `nseg_state_type`; monostate means no
 * lazy carry, optional<tag_type> carries ancestor tags.  Missing children contribute
 * the merge identity.  Mutating nonpersistent owners invalidates the epoch; read-only
 * descent never materializes dynamic children.
 */
template <class S> class nseg_node {
  public:
    using aggregate_type = typename S::aggregate_type;
    using state_type = typename S::nseg_state_type;

  private:
    const S* owner_ = nullptr;
    const void* domain_ = nullptr;
    int handle_ = 0;
    uint64_t epoch_ = 0;
    long long left_ = 0, right_ = 0;
    state_type carry_{};

    static const void* domain_token_of(const S* owner) noexcept {
        if (!owner)
            return nullptr;
        if constexpr (requires(const S& value) { value.nseg_domain_token(); })
            return owner->nseg_domain_token();
        return owner;
    }

    nseg_node(const S* owner, int handle, uint64_t epoch, long long left, long long right,
              state_type carry = {})
        : owner_(owner), domain_(domain_token_of(owner)), handle_(handle), epoch_(epoch), left_(left),
          right_(right), carry_(move(carry)) {}
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

    bool current() const {
        return owner_ && domain_ == domain_token_of(owner_) && epoch_ == owner_->nseg_epoch();
    }
    bool ok() const { return current() && handle_ && owner_->nseg_alive(handle_); }
    explicit operator bool() const { return ok(); }

    bool same_owner(const nseg_node& other) const noexcept { return owner_ == other.owner_; }
    bool same_domain(const nseg_node& other) const noexcept {
        return domain_ && domain_ == other.domain_;
    }
    nnode_identity identity() const {
        npre(current());
        if constexpr (requires(const S& owner, int handle) {
                          owner.nseg_identity_of(handle);
                      }) {
            return owner_->nseg_identity_of(handle_);
        } else {
            return {domain_, handle_, handle_ ? 1u : 0u};
        }
    }

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

// `decide` must eventually take a node or descend toward a nonempty child.  Returning
// left/right at a leaf violates the walk contract and is rejected by the node view.
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

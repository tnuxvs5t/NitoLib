namespace ni {
// Internal reusable-handle pool.  Erased handles may be recycled, so node snapshots
// rely on the owning tree epoch as well as handle liveness to reject stale identity.
template <class T> class nslot_pool {
    vector<optional<T>> slots_;
    vector<int> free_;
    int live_ = 0;

  public:
    int len() const noexcept { return live_; }
    bool empty() const noexcept { return live_ == 0; }

    void reserve(int capacity) {
        npre(capacity >= 0);
        slots_.reserve(size_t(capacity));
        free_.reserve(size_t(capacity));
    }

    template <class... A> int make(A&&... args) {
        npre(live_ < INT_MAX);
        int handle;
        if (free_.empty()) {
            npre(slots_.size() < size_t(INT_MAX));
            slots_.emplace_back(in_place, forward<A>(args)...);
            handle = int(slots_.size());
        } else {
            handle = free_.back();
            free_.pop_back();
            slots_[handle - 1].emplace(forward<A>(args)...);
        }
        ++live_;
        return handle;
    }

    bool alive(int handle) const noexcept {
        return 0 < handle && handle <= int(slots_.size()) && slots_[handle - 1].has_value();
    }
    T& operator[](int handle) {
        npre(alive(handle));
        return *slots_[handle - 1];
    }
    const T& operator[](int handle) const {
        npre(alive(handle));
        return *slots_[handle - 1];
    }
    void erase(int handle) {
        npre(alive(handle));
        slots_[handle - 1].reset();
        free_.push_back(handle);
        --live_;
    }
    void clear() noexcept {
        slots_.clear();
        free_.clear();
        live_ = 0;
    }
};

template <class S> bool nordered_equal(const S& left, const S& right) {
    if (left.len() != right.len())
        return false;
    auto a = left.enumerate(), b = right.enumerate();
    while (a.ok()) {
        if (!left.equivalent(a.val(), b.val()))
            return false;
        a.next();
        b.next();
    }
    return true;
}
} // namespace ni

/**
 * Convenience policy for an implicit FHQ whose subtree information is an ordered
 * augmentation and whose lazy operation is a uniform element action.  This adapter is
 * intentionally not the FHQ core: custom policies may use arbitrary state, accept
 * several tag types, exchange children, and derive different child tags in push().
 */
template <class T, class A = nempty_augment<T>,
          class L = nempty_tag<T, typename A::info_type>>
struct nfhq_policy {
    using info_type = typename A::info_type;
    using tag_type = typename L::tag_type;
    struct state_type {
        tag_type lazy;
        bool pending = false;
    };

    [[no_unique_address]] A augment{};
    [[no_unique_address]] L action{};

    nfhq_policy() = default;
    explicit nfhq_policy(A augment) : augment(move(augment)) {}
    nfhq_policy(A augment, L action) : augment(move(augment)), action(move(action)) {}

    info_type id() const { return augment.id(); }
    info_type leaf(const T& value) const { return augment.one(value, 1); }
    state_type state_id() const { return {action.tag_id(), false}; }

    void pull(auto node) const {
        auto left = node.left(), right = node.right();
        info_type left_info = left ? left.info() : augment.id();
        info_type right_info = right ? right.info() : augment.id();
        node.info() = augment.op(augment.op(move(left_info), augment.one(node.val(), 1)),
                                 move(right_info));
    }
    void apply(auto node, const tag_type& tag) const {
        node.val() = action.apply_value(move(node.val()), tag, 1);
        node.info() = action.apply_info(move(node.info()), tag, node.len());
        auto& state = node.state();
        if (state.pending)
            state.lazy = action.compose(tag, state.lazy);
        else {
            state.lazy = tag;
            state.pending = true;
        }
    }
    void push(auto node) const {
        auto& state = node.state();
        if (!state.pending)
            return;
        tag_type tag = state.lazy;
        auto left = node.left(), right = node.right();
        if (left)
            left.apply(tag);
        if (right)
            right.apply(tag);
        state = state_id();
    }
};

/**
 * Implicit FHQ sequence engine.  P owns all semantic state: it provides info_type,
 * state_type, id(), leaf(value), state_id(), pull(node), push(node), and any desired
 * apply(node,tag) overloads.  The editable node passed to P exposes value/info/state,
 * child nodes, length, apply(tag), and exchange_children().  Thus lazy composition,
 * position-dependent child tags and structural actions belong to the policy rather
 * than a fixed trait protocol.  The engine only protects ownership, subtree sizes,
 * parent links and randomized split/merge.  Expected structural cost is O(log n).
 */
template <class T, class P = nfhq_policy<T>> class nimplicit_fhq {
    struct node {
        T value;
        uint64_t priority;
        int left = 0, right = 0, parent = 0, size = 1;
        typename P::info_type info;
        typename P::state_type state;

        node(T value, uint64_t priority, typename P::info_type info,
             typename P::state_type state)
            : value(move(value)), priority(priority), info(move(info)), state(move(state)) {}
    };

  public:
    using value_type = T;
    using policy_type = P;
    using info_type = typename P::info_type;
    using state_type = typename P::state_type;
    using node_view = nnode<nimplicit_fhq>;

    /**
     * Mutable policy-facing AST handle.  It never owns a node and is only valid during
     * the callback that received it.  exchange_children() is the primitive for any
     * order-changing tag; the policy must update info/state consistently in apply().
     */
    class node_editor {
        nimplicit_fhq* owner_ = nullptr;
        int handle_ = 0;

        node_editor(nimplicit_fhq* owner, int handle) : owner_(owner), handle_(handle) {}
        friend class nimplicit_fhq;

      public:
        explicit operator bool() const noexcept { return handle_ != 0; }
        T& val() const {
            npre(handle_);
            return owner_->pool_[handle_].value;
        }
        info_type& info() const {
            npre(handle_);
            return owner_->pool_[handle_].info;
        }
        state_type& state() const {
            npre(handle_);
            return owner_->pool_[handle_].state;
        }
        int len() const { return owner_->size_of(handle_); }
        node_editor left() const {
            npre(handle_);
            return {owner_, owner_->pool_[handle_].left};
        }
        node_editor right() const {
            npre(handle_);
            return {owner_, owner_->pool_[handle_].right};
        }
        void exchange_children() const {
            npre(handle_);
            swap(owner_->pool_[handle_].left, owner_->pool_[handle_].right);
        }
        template <class Tag> void apply(Tag&& tag) const {
            npre(handle_);
            owner_->put(handle_, forward<Tag>(tag));
        }
    };

  private:
    mutable ni::nslot_pool<node> pool_;
    int root_ = 0;
    [[no_unique_address]] P policy_{};
    mutable uint64_t epoch_ = 1;

    void touch() const noexcept {
        if (!++epoch_)
            ++epoch_;
    }
    int size_of(int handle) const { return handle ? pool_[handle].size : 0; }
    void make_root(int handle) {
        root_ = handle;
        if (handle)
            pool_[handle].parent = 0;
    }
    void attach_left(int parent, int child) {
        int old = pool_[parent].left;
        if (old && pool_[old].parent == parent)
            pool_[old].parent = 0;
        pool_[parent].left = child;
        if (child)
            pool_[child].parent = parent;
    }
    void attach_right(int parent, int child) {
        int old = pool_[parent].right;
        if (old && pool_[old].parent == parent)
            pool_[old].parent = 0;
        pool_[parent].right = child;
        if (child)
            pool_[child].parent = parent;
    }
    template <class Tag> void put(int handle, Tag&& tag) const {
        if (handle)
            policy_.apply(node_editor(const_cast<nimplicit_fhq*>(this), handle),
                          forward<Tag>(tag));
    }
    void push_down(int handle) const {
        if (handle)
            policy_.push(node_editor(const_cast<nimplicit_fhq*>(this), handle));
    }
    void pull(int handle) {
        if (!handle)
            return;
        long long size = 1LL + size_of(pool_[handle].left) + size_of(pool_[handle].right);
        npre(size <= INT_MAX);
        pool_[handle].size = int(size);
        policy_.pull(node_editor(this, handle));
    }
    int merge(int left, int right) {
        if (!left) {
            if (right)
                pool_[right].parent = 0;
            return right;
        }
        if (!right) {
            pool_[left].parent = 0;
            return left;
        }
        push_down(left);
        push_down(right);
        int result;
        if (pool_[left].priority >= pool_[right].priority) {
            int joined = merge(pool_[left].right, right);
            attach_right(left, joined);
            pull(left);
            result = left;
        } else {
            int joined = merge(left, pool_[right].left);
            attach_left(right, joined);
            pull(right);
            result = right;
        }
        pool_[result].parent = 0;
        return result;
    }
    void split(int handle, int left_size, int& left, int& right) {
        npre(0 <= left_size && left_size <= size_of(handle));
        if (!handle) {
            left = right = 0;
            return;
        }
        push_down(handle);
        int current_left = size_of(pool_[handle].left);
        if (left_size <= current_left) {
            int child;
            split(pool_[handle].left, left_size, left, child);
            attach_left(handle, child);
            pull(handle);
            right = handle;
        } else {
            int child;
            split(pool_[handle].right, left_size - current_left - 1, child, right);
            attach_right(handle, child);
            pull(handle);
            left = handle;
        }
        if (left)
            pool_[left].parent = 0;
        if (right)
            pool_[right].parent = 0;
    }
    int find_handle(int index) const {
        npre(0 <= index && index < size_of(root_));
        for (int handle = root_; handle;) {
            push_down(handle);
            int left_size = size_of(pool_[handle].left);
            if (index < left_size)
                handle = pool_[handle].left;
            else if (index == left_size)
                return handle;
            else {
                index -= left_size + 1;
                handle = pool_[handle].right;
            }
        }
        npre(false);
        return 0;
    }
    void pull_ancestors(int handle) {
        for (handle = handle ? pool_[handle].parent : 0; handle; handle = pool_[handle].parent)
            pull(handle);
    }
    void release(int handle) {
        if (!handle)
            return;
        int left = pool_[handle].left, right = pool_[handle].right;
        release(left);
        release(right);
        pool_.erase(handle);
    }

    friend class nnode<nimplicit_fhq>;
    uint64_t nnode_epoch() const noexcept { return epoch_; }
    bool nnode_alive(int handle) const noexcept { return pool_.alive(handle); }
    const T& nnode_val(int handle) const { return pool_[handle].value; }
    int nnode_count(int handle) const { return handle ? 1 : 0; }
    int nnode_len(int handle) const { return size_of(handle); }
    info_type nnode_info(int handle) const { return handle ? pool_[handle].info : policy_.id(); }
    int nnode_left(int handle) const {
        push_down(handle);
        return handle ? pool_[handle].left : 0;
    }
    int nnode_right(int handle) const {
        push_down(handle);
        return handle ? pool_[handle].right : 0;
    }
    int nnode_parent(int handle) const { return handle ? pool_[handle].parent : 0; }
    state_type nnode_state(int handle) const {
        npre(handle);
        return pool_[handle].state;
    }

  public:
    nimplicit_fhq() = default;
    explicit nimplicit_fhq(P policy) : policy_(move(policy)) {}
    nimplicit_fhq(initializer_list<T> values, P policy = {}) : policy_(move(policy)) {
        reserve(int(values.size()));
        for (const T& value : values)
            ins(len(), value);
    }
    nimplicit_fhq(const nimplicit_fhq& other)
        : pool_(other.pool_), root_(other.root_), policy_(other.policy_) {}
    nimplicit_fhq(nimplicit_fhq&& other) noexcept(
        is_nothrow_move_constructible_v<ni::nslot_pool<node>> &&
        is_nothrow_move_constructible_v<P>)
        : pool_(move(other.pool_)), root_(exchange(other.root_, 0)),
          policy_(move(other.policy_)) {
        other.touch();
    }
    nimplicit_fhq& operator=(const nimplicit_fhq& other) {
        if (this != addressof(other)) {
            pool_ = other.pool_;
            root_ = other.root_;
            policy_ = other.policy_;
            touch();
        }
        return *this;
    }
    nimplicit_fhq& operator=(nimplicit_fhq&& other) noexcept(
        is_nothrow_move_assignable_v<ni::nslot_pool<node>> &&
        is_nothrow_move_assignable_v<P>) {
        if (this != addressof(other)) {
            pool_ = move(other.pool_);
            root_ = exchange(other.root_, 0);
            policy_ = move(other.policy_);
            touch();
            other.touch();
        }
        return *this;
    }

    int len() const { return size_of(root_); }
    bool empty() const noexcept { return root_ == 0; }
    const P& policy() const noexcept { return policy_; }
    node_view root() const { return node_view(this, root_, epoch_); }
    template <class F> node_view walk(F&& decide) const {
        return nwalk(*this, forward<F>(decide));
    }

    void reserve(int capacity) { pool_.reserve(capacity); }
    void clear() {
        if (root_)
            touch();
        pool_.clear();
        root_ = 0;
    }

    const T& operator[](int index) const { return pool_[find_handle(index)].value; }
    T get(int index) const { return (*this)[index]; }
    void set(int index, T value) {
        int handle = find_handle(index);
        push_down(handle);
        pool_[handle].value = move(value);
        pool_[handle].info = policy_.leaf(pool_[handle].value);
        policy_.pull(node_editor(this, handle));
        pull_ancestors(handle);
        touch();
    }

    void ins(int index, const T& value) {
        T copy = value;
        ins(index, move(copy));
    }
    void ins(int index, T&& value) {
        npre(0 <= index && index <= len());
        npre(len() < INT_MAX);
        int left, right;
        split(root_, index, left, right);
        info_type info = policy_.leaf(value);
        int fresh = pool_.make(move(value), nrng_global(), move(info), policy_.state_id());
        make_root(merge(merge(left, fresh), right));
        touch();
    }
    void push(const T& value) { ins(len(), value); }
    void push(T&& value) { ins(len(), move(value)); }

    int del(int left, int right) {
        npre(0 <= left && left <= right && right <= len());
        if (left == right)
            return 0;
        int prefix_middle, suffix, prefix, middle;
        split(root_, right, prefix_middle, suffix);
        split(prefix_middle, left, prefix, middle);
        int removed = size_of(middle);
        release(middle);
        make_root(merge(prefix, suffix));
        touch();
        return removed;
    }
    void del(int index) {
        npre(0 <= index && index < len());
        del(index, index + 1);
    }

    info_type fold() const { return root_ ? pool_[root_].info : policy_.id(); }
    info_type fold(int left, int right) {
        npre(0 <= left && left <= right && right <= len());
        if (left == right)
            return policy_.id();
        if (left == 0 && right == len())
            return pool_[root_].info;
        int prefix_middle, suffix, prefix, middle;
        split(root_, right, prefix_middle, suffix);
        split(prefix_middle, left, prefix, middle);
        info_type result = pool_[middle].info;
        make_root(merge(merge(prefix, middle), suffix));
        touch();
        return result;
    }

    template <class Tag> void apply(int left, int right, Tag&& tag) {
        npre(0 <= left && left <= right && right <= len());
        if (left == right)
            return;
        int prefix_middle, suffix, prefix, middle;
        split(root_, right, prefix_middle, suffix);
        split(prefix_middle, left, prefix, middle);
        put(middle, forward<Tag>(tag));
        make_root(merge(merge(prefix, middle), suffix));
        touch();
    }
    template <class Tag> void apply(Tag&& tag) {
        if (root_) {
            put(root_, forward<Tag>(tag));
            touch();
        }
    }
    template <class Tag> void apply(const node_view& selected, Tag&& tag) {
        npre(selected.current() && selected.ok() && addressof(selected.owner()) == this);
        int handle = selected.handle();
        put(handle, forward<Tag>(tag));
        pull_ancestors(handle);
        touch();
    }

    // Low-level escape hatch: callback must leave value/info/state and child order as a
    // valid lazy representation of the same node set.  Ancestor information is rebuilt.
    template <class F> void mutate(const node_view& selected, F&& mutation) {
        npre(selected.current() && selected.ok() && addressof(selected.owner()) == this);
        int handle = selected.handle();
        invoke(forward<F>(mutation), node_editor(this, handle));
        pull_ancestors(handle);
        touch();
    }
    template <class F> void mutate(int left, int right, F&& mutation) {
        npre(0 <= left && left < right && right <= len());
        int prefix_middle, suffix, prefix, middle;
        split(root_, right, prefix_middle, suffix);
        split(prefix_middle, left, prefix, middle);
        invoke(forward<F>(mutation), node_editor(this, middle));
        make_root(merge(merge(prefix, middle), suffix));
        touch();
    }

    // Move [left,right) to position `at` in the sequence after that interval is removed.
    void splice(int left, int right, int at) {
        npre(0 <= left && left <= right && right <= len());
        int width = right - left;
        npre(0 <= at && at <= len() - width);
        if (!width || at == left)
            return;
        int prefix_middle, suffix, prefix, middle;
        split(root_, right, prefix_middle, suffix);
        split(prefix_middle, left, prefix, middle);
        int rest = merge(prefix, suffix), before, after;
        split(rest, at, before, after);
        make_root(merge(merge(before, middle), after));
        touch();
    }
    void rotate(int left, int middle, int right) {
        npre(0 <= left && left <= middle && middle <= right && right <= len());
        if (left == middle || middle == right)
            return;
        int abc, d, ab, c, a, b;
        split(root_, right, abc, d);
        split(abc, middle, ab, c);
        split(ab, left, a, b);
        make_root(merge(merge(merge(a, c), b), d));
        touch();
    }

    struct cursor {
        const nimplicit_fhq* owner;
        vector<int> stack;
        int index = 0;

        explicit cursor(const nimplicit_fhq* owner) : owner(owner) { descend(owner->root_); }
        void descend(int handle) {
            while (handle) {
                owner->push_down(handle);
                stack.push_back(handle);
                handle = owner->pool_[handle].left;
            }
        }
        bool ok() const { return !stack.empty(); }
        const T& val() const { return owner->pool_[stack.back()].value; }
        int idx() const { return index; }
        void next() {
            int handle = stack.back();
            stack.pop_back();
            ++index;
            owner->push_down(handle);
            descend(owner->pool_[handle].right);
        }
    };
    cursor enumerate() const& { return cursor(this); }
    cursor enumerate() && = delete;
};

template <class T, class P = nfhq_policy<T>>
using nseq_fhq = nimplicit_fhq<T, P>;

/**
 * Randomized ordered multiset/set with pluggable augmentation and lazy node action.
 * C is a strict weak ordering; A has id/one/ordered op; L follows the nempty_tag
 * protocol and compose(newer,older) order.  Every applied tag must preserve C-order
 * and equivalence classes in the affected subtree.  Node views expire after topology
 * changes.  Expected operations are O(log n); adversarial/random-priority failure is
 * probabilistic rather than impossible.
 */
template <class T, class C = nless<T>, bool Multi = false, class A = nempty_augment<T>,
          class L = nempty_tag<T, typename A::info_type>>
class nset_fhq {
    struct node {
        T value;
        uint64_t priority;
        int left = 0, right = 0, size = 1, count = 1;
        typename A::info_type aggregate;
        typename L::tag_type lazy;
        bool pending = false;

        node(T value, uint64_t priority, int count, typename A::info_type aggregate,
             typename L::tag_type lazy)
            : value(move(value)), priority(priority), size(count), count(count),
              aggregate(move(aggregate)), lazy(move(lazy)) {}
    };

    mutable ni::nslot_pool<node> pool_;
    int root_ = 0;
    [[no_unique_address]] C compare_{};
    [[no_unique_address]] A augment_{};
    [[no_unique_address]] L action_{};
    mutable uint64_t epoch_ = 1;

    void touch() const noexcept {
        if (!++epoch_)
            ++epoch_;
    }
    int size_of(int handle) const { return handle ? pool_[handle].size : 0; }
    void pull(int handle) {
        if (!handle)
            return;
        node& current = pool_[handle];
        long long size = 1LL * size_of(current.left) + current.count + size_of(current.right);
        npre(size <= INT_MAX);
        current.size = int(size);
        current.aggregate = augment_.op(
            augment_.op(nnode_info(current.left), augment_.one(current.value, current.count)),
            nnode_info(current.right));
    }
    void apply_tag(int handle, const typename L::tag_type& tag) const {
        if (!handle)
            return;
        node& current = pool_[handle];
        current.value = action_.apply_value(move(current.value), tag, current.count);
        current.aggregate = action_.apply_info(move(current.aggregate), tag, current.size);
        if (current.pending)
            current.lazy = action_.compose(tag, current.lazy);
        else {
            current.lazy = tag;
            current.pending = true;
        }
    }
    void push(int handle) const {
        if (!handle || !pool_[handle].pending)
            return;
        node& current = pool_[handle];
        apply_tag(current.left, current.lazy);
        apply_tag(current.right, current.lazy);
        current.lazy = action_.tag_id();
        current.pending = false;
    }
    int merge(int left, int right) {
        if (!left)
            return right;
        if (!right)
            return left;
        push(left);
        push(right);
        if (pool_[left].priority >= pool_[right].priority) {
            pool_[left].right = merge(pool_[left].right, right);
            pull(left);
            return left;
        }
        pool_[right].left = merge(left, pool_[right].left);
        pull(right);
        return right;
    }
    void split(int handle, const T& value, int& left, int& right) {
        if (!handle) {
            left = right = 0;
            return;
        }
        push(handle);
        if (invoke(compare_, pool_[handle].value, value)) {
            left = handle;
            split(pool_[handle].right, value, pool_[left].right, right);
            pull(left);
        } else {
            right = handle;
            split(pool_[handle].left, value, left, pool_[right].left);
            pull(right);
        }
    }
    int add_existing(int handle, const T& value, int count) {
        if (!handle)
            return -1;
        push(handle);
        node& current = pool_[handle];
        if (equivalent(value, current.value)) {
            if constexpr (Multi) {
                npre(current.count <= INT_MAX - count);
                current.count += count;
                pull(handle);
                return count;
            }
            return 0;
        }
        int added = add_existing(invoke(compare_, value, current.value) ? current.left : current.right,
                                 value, count);
        if (added >= 0)
            pull(handle);
        return added;
    }
    int erase_at(int& handle, const T& value, int count, bool all) {
        if (!handle)
            return 0;
        push(handle);
        node& current = pool_[handle];
        if (equivalent(value, current.value)) {
            if constexpr (Multi) {
                if (!all && current.count > count) {
                    current.count -= count;
                    pull(handle);
                    return count;
                }
            }
            int removed = Multi ? current.count : 1;
            int old = handle;
            handle = merge(current.left, current.right);
            pool_.erase(old);
            return removed;
        }
        int removed = erase_at(invoke(compare_, value, current.value) ? current.left : current.right,
                               value, count, all);
        pull(handle);
        return removed;
    }

    friend class nnode<nset_fhq>;
    uint64_t nnode_epoch() const noexcept { return epoch_; }
    bool nnode_alive(int handle) const noexcept { return pool_.alive(handle); }
    const T& nnode_val(int handle) const { return pool_[handle].value; }
    int nnode_count(int handle) const { return handle ? pool_[handle].count : 0; }
    int nnode_len(int handle) const { return size_of(handle); }
    typename A::info_type nnode_info(int handle) const {
        return handle ? pool_[handle].aggregate : augment_.id();
    }
    int nnode_left(int handle) const {
        push(handle);
        return handle ? pool_[handle].left : 0;
    }
    int nnode_right(int handle) const {
        push(handle);
        return handle ? pool_[handle].right : 0;
    }
    typename L::tag_type nnode_tag(int handle) const {
        return handle && pool_[handle].pending ? pool_[handle].lazy : action_.tag_id();
    }

  public:
    using value_type = T;
    using augment_type = A;
    using info_type = typename A::info_type;
    using tag_action = L;
    using tag_type = typename L::tag_type;
    using node_view = nnode<nset_fhq>;

    nset_fhq() = default;
    explicit nset_fhq(C compare) : compare_(move(compare)) {}
    explicit nset_fhq(A augment)
        requires(!same_as<C, A>)
        : augment_(move(augment)) {}
    explicit nset_fhq(L action)
        requires(!same_as<C, L> && !same_as<A, L>)
        : action_(move(action)) {}
    nset_fhq(C compare, A augment, L action = {})
        : compare_(move(compare)), augment_(move(augment)), action_(move(action)) {}
    nset_fhq(initializer_list<T> values) {
        for (const T& value : values)
            ins(value);
    }
    nset_fhq(const nset_fhq& other)
        : pool_(other.pool_), root_(other.root_), compare_(other.compare_), augment_(other.augment_),
          action_(other.action_) {}
    nset_fhq(nset_fhq&& other) noexcept(
        is_nothrow_move_constructible_v<ni::nslot_pool<node>> && is_nothrow_move_constructible_v<C> &&
        is_nothrow_move_constructible_v<A> && is_nothrow_move_constructible_v<L>)
        : pool_(move(other.pool_)), root_(exchange(other.root_, 0)), compare_(move(other.compare_)),
          augment_(move(other.augment_)), action_(move(other.action_)) {
        other.touch();
    }
    nset_fhq& operator=(const nset_fhq& other) {
        if (this != addressof(other)) {
            pool_ = other.pool_;
            root_ = other.root_;
            compare_ = other.compare_;
            augment_ = other.augment_;
            action_ = other.action_;
            touch();
        }
        return *this;
    }
    nset_fhq& operator=(nset_fhq&& other) noexcept(
        is_nothrow_move_assignable_v<ni::nslot_pool<node>> && is_nothrow_move_assignable_v<C> &&
        is_nothrow_move_assignable_v<A> && is_nothrow_move_assignable_v<L>) {
        if (this != addressof(other)) {
            pool_ = move(other.pool_);
            root_ = exchange(other.root_, 0);
            compare_ = move(other.compare_);
            augment_ = move(other.augment_);
            action_ = move(other.action_);
            touch();
            other.touch();
        }
        return *this;
    }

    int len() const { return size_of(root_); }
    bool empty() const noexcept { return root_ == 0; }
    bool equivalent(const T& a, const T& b) const {
        return !invoke(compare_, a, b) && !invoke(compare_, b, a);
    }
    const A& augment() const noexcept { return augment_; }
    const L& action() const noexcept { return action_; }
    node_view root() const { return node_view(this, root_, epoch_); }
    template <class F> node_view walk(F&& decide) const {
        return nwalk(*this, forward<F>(decide));
    }
    template <class P> node_view first_prefix(P&& predicate) const {
        return nfirst_prefix(*this, forward<P>(predicate));
    }
    template <class P> node_view last_suffix(P&& predicate) const {
        return nlast_suffix(*this, forward<P>(predicate));
    }

    void reserve(int capacity) { pool_.reserve(capacity); }
    void clear() {
        if (root_)
            touch();
        pool_.clear();
        root_ = 0;
    }

    // Apply a lazy action to the whole tree or to a node selected through the
    // read-only AST view.  The action must preserve the comparator order; this
    // is a semantic precondition because a general value transform can destroy
    // the search-tree invariant.
    void apply(const typename L::tag_type& tag) {
        if (root_) {
            apply_tag(root_, tag);
            touch();
        }
    }
    void apply(const node_view& node, const typename L::tag_type& tag) {
        npre(node.current() && node.ok());
        int target = node.handle();
        vector<int> ancestors;
        for (int handle = root_; handle != target;) {
            npre(handle);
            push(handle);
            ancestors.push_back(handle);
            if (equivalent(pool_[target].value, pool_[handle].value)) {
                npre(false);
            } else {
                handle = invoke(compare_, pool_[target].value, pool_[handle].value)
                             ? pool_[handle].left
                             : pool_[handle].right;
            }
        }
        apply_tag(target, tag);
        for (auto iterator = ancestors.rbegin(); iterator != ancestors.rend(); ++iterator)
            pull(*iterator);
        touch();
    }

    int ins(const T& value, int count = 1) {
        T copy = value;
        return ins(move(copy), count);
    }
    int ins(T&& value, int count = 1) {
        npre(count >= 0);
        if (!count)
            return 0;
        npre(len() <= INT_MAX - (Multi ? count : 1));
        int added = add_existing(root_, value, count);
        if (added >= 0) {
            if (added)
                touch();
            return added;
        }
        int left, right;
        split(root_, value, left, right);
        int stored_count = Multi ? count : 1;
        auto aggregate = augment_.one(value, stored_count);
        int fresh = pool_.make(move(value), nrng_global(), stored_count, move(aggregate), action_.tag_id());
        root_ = merge(merge(left, fresh), right);
        touch();
        return stored_count;
    }
    int del(const T& value, int count = 1) {
        npre(count >= 0);
        int removed = count ? erase_at(root_, value, count, false) : 0;
        if (removed)
            touch();
        return removed;
    }
    int delall(const T& value) {
        int removed = erase_at(root_, value, 0, true);
        if (removed)
            touch();
        return removed;
    }
    int findi(const T& value) const {
        for (int handle = root_; handle;) {
            push(handle);
            const node& current = pool_[handle];
            if (equivalent(value, current.value))
                return handle;
            handle = invoke(compare_, value, current.value) ? current.left : current.right;
        }
        return 0;
    }
    bool has(const T& value) const { return findi(value) != 0; }
    int count(const T& value) const {
        int handle = findi(value);
        return handle ? pool_[handle].count : 0;
    }
    const T* get(const T& value) const {
        int handle = findi(value);
        return handle ? addressof(pool_[handle].value) : nullptr;
    }
    int rank(const T& value) const {
        int result = 0;
        for (int handle = root_; handle;) {
            push(handle);
            const node& current = pool_[handle];
            if (invoke(compare_, current.value, value)) {
                result += size_of(current.left) + current.count;
                handle = current.right;
            } else {
                handle = current.left;
            }
        }
        return result;
    }
    nmaybe<T> kth(int index) const {
        if (index < 0 || index >= len())
            return {};
        for (int handle = root_; handle;) {
            push(handle);
            const node& current = pool_[handle];
            int left_size = size_of(current.left);
            if (index < left_size)
                handle = current.left;
            else if (index < left_size + current.count)
                return current.value;
            else {
                index -= left_size + current.count;
                handle = current.right;
            }
        }
        npre(false);
        return {};
    }
    T kth(int index, T fallback) const {
        auto result = kth(index);
        return result ? result.val() : move(fallback);
    }
    nmaybe<T> lower(const T& value) const {
        int result = 0;
        for (int handle = root_; handle;) {
            push(handle);
            const node& current = pool_[handle];
            if (!invoke(compare_, current.value, value)) {
                result = handle;
                handle = current.left;
            } else {
                handle = current.right;
            }
        }
        return result ? nmaybe<T>(pool_[result].value) : nmaybe<T>{};
    }
    nmaybe<T> upper(const T& value) const {
        int result = 0;
        for (int handle = root_; handle;) {
            push(handle);
            const node& current = pool_[handle];
            if (invoke(compare_, value, current.value)) {
                result = handle;
                handle = current.left;
            } else {
                handle = current.right;
            }
        }
        return result ? nmaybe<T>(pool_[result].value) : nmaybe<T>{};
    }
    T lower(const T& value, T fallback) const {
        auto result = lower(value);
        return result ? result.val() : move(fallback);
    }
    T upper(const T& value, T fallback) const {
        auto result = upper(value);
        return result ? result.val() : move(fallback);
    }
    nmaybe<T> min() const { return kth(0); }
    nmaybe<T> max() const { return kth(len() - 1); }
    T min(T fallback) const { return kth(0, move(fallback)); }
    T max(T fallback) const { return kth(len() - 1, move(fallback)); }

    struct cursor {
        const nset_fhq* owner;
        vector<int> stack;
        int repetition = 0, index = 0;

        explicit cursor(const nset_fhq* owner) : owner(owner) { descend(owner->root_); }
        void descend(int handle) {
            while (handle) {
                owner->push(handle);
                stack.push_back(handle);
                handle = owner->pool_[handle].left;
            }
        }
        bool ok() const { return !stack.empty(); }
        const T& val() const { return owner->pool_[stack.back()].value; }
        int idx() const { return index; }
        void next() {
            int handle = stack.back();
            ++index;
            if (++repetition < owner->pool_[handle].count)
                return;
            repetition = 0;
            stack.pop_back();
            owner->push(handle);
            descend(owner->pool_[handle].right);
        }
    };
    cursor enumerate() const& { return cursor(this); }
    cursor enumerate() && = delete;

    nset_fhq& operator|=(const nset_fhq& other)
        requires(!Multi)
    {
        if (this != addressof(other))
            nfor(value, other)
                ins(value);
        return *this;
    }
    nset_fhq& operator&=(const nset_fhq& other)
        requires(!Multi)
    {
        if (this == addressof(other))
            return *this;
        nvector<T> removed;
        nfor(value, *this)
            if (!other.has(value))
                removed.push(value);
        nfor(value, removed)
            del(value);
        return *this;
    }
    nset_fhq& operator-=(const nset_fhq& other)
        requires(!Multi)
    {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(value, other)
            del(value);
        return *this;
    }
    nset_fhq& operator^=(const nset_fhq& other)
        requires(!Multi)
    {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(value, other) {
            if (has(value))
                del(value);
            else
                ins(value);
        }
        return *this;
    }
    friend nset_fhq operator|(nset_fhq left, const nset_fhq& right)
        requires(!Multi)
    {
        return left |= right;
    }
    friend nset_fhq operator&(nset_fhq left, const nset_fhq& right)
        requires(!Multi)
    {
        return left &= right;
    }
    friend nset_fhq operator-(nset_fhq left, const nset_fhq& right)
        requires(!Multi)
    {
        return left -= right;
    }
    friend nset_fhq operator^(nset_fhq left, const nset_fhq& right)
        requires(!Multi)
    {
        return left ^= right;
    }
    friend bool operator==(const nset_fhq& left, const nset_fhq& right) {
        return ni::nordered_equal(left, right);
    }
};

/**
 * Splay ordered multiset/set with the same comparator and augmentation contracts as
 * nset_fhq, but no lazy action.  Even logically read-only searches may rotate the
 * tree and invalidate node snapshots.  Amortized operations are O(log n).
 */
template <class T, class C = nless<T>, bool Multi = false, class A = nempty_augment<T>>
class nset_splay {
    struct node {
        T value;
        int left = 0, right = 0, parent = 0, size = 1, count = 1;
        typename A::info_type aggregate;

        node(T value, int count, typename A::info_type aggregate)
            : value(move(value)), size(count), count(count), aggregate(move(aggregate)) {}
    };

    mutable ni::nslot_pool<node> pool_;
    mutable int root_ = 0;
    [[no_unique_address]] C compare_{};
    [[no_unique_address]] A augment_{};
    mutable uint64_t epoch_ = 1;

    void touch() const noexcept {
        if (!++epoch_)
            ++epoch_;
    }
    int size_of(int handle) const { return handle ? pool_[handle].size : 0; }
    void pull(int handle) const {
        if (!handle)
            return;
        node& current = pool_[handle];
        long long size = 1LL * size_of(current.left) + current.count + size_of(current.right);
        npre(size <= INT_MAX);
        current.size = int(size);
        current.aggregate = augment_.op(
            augment_.op(nnode_info(current.left), augment_.one(current.value, current.count)),
            nnode_info(current.right));
    }
    int& child(int handle, bool right) const { return right ? pool_[handle].right : pool_[handle].left; }
    void rotate(int handle) const {
        int parent = pool_[handle].parent;
        int grandparent = pool_[parent].parent;
        bool right = handle == pool_[parent].right;
        int middle = child(handle, !right);
        if (grandparent)
            child(grandparent, parent == pool_[grandparent].right) = handle;
        else
            root_ = handle;
        pool_[handle].parent = grandparent;
        child(parent, right) = middle;
        if (middle)
            pool_[middle].parent = parent;
        child(handle, !right) = parent;
        pool_[parent].parent = handle;
        pull(parent);
        pull(handle);
    }
    void splay(int handle) const {
        if (pool_[handle].parent)
            touch();
        while (pool_[handle].parent) {
            int parent = pool_[handle].parent;
            int grandparent = pool_[parent].parent;
            if (grandparent) {
                bool zigzig = (handle == pool_[parent].right) == (parent == pool_[grandparent].right);
                rotate(zigzig ? parent : handle);
            }
            rotate(handle);
        }
    }
    nmaybe<T> bound(const T& value, bool strict) const {
        int handle = root_, result = 0, last = 0;
        while (handle) {
            last = handle;
            const node& current = pool_[handle];
            bool candidate = strict ? invoke(compare_, value, current.value)
                                    : !invoke(compare_, current.value, value);
            if (candidate) {
                result = handle;
                handle = current.left;
            } else {
                handle = current.right;
            }
        }
        if (result)
            splay(result);
        else if (last)
            splay(last);
        return result ? nmaybe<T>(pool_[result].value) : nmaybe<T>{};
    }

    friend class nnode<nset_splay>;
    uint64_t nnode_epoch() const noexcept { return epoch_; }
    bool nnode_alive(int handle) const noexcept { return pool_.alive(handle); }
    const T& nnode_val(int handle) const { return pool_[handle].value; }
    int nnode_count(int handle) const { return handle ? pool_[handle].count : 0; }
    int nnode_len(int handle) const { return size_of(handle); }
    typename A::info_type nnode_info(int handle) const {
        return handle ? pool_[handle].aggregate : augment_.id();
    }
    int nnode_left(int handle) const { return handle ? pool_[handle].left : 0; }
    int nnode_right(int handle) const { return handle ? pool_[handle].right : 0; }

  public:
    using value_type = T;
    using augment_type = A;
    using info_type = typename A::info_type;
    using node_view = nnode<nset_splay>;

    nset_splay() = default;
    explicit nset_splay(C compare) : compare_(move(compare)) {}
    explicit nset_splay(A augment)
        requires(!same_as<C, A>)
        : augment_(move(augment)) {}
    nset_splay(C compare, A augment) : compare_(move(compare)), augment_(move(augment)) {}
    nset_splay(initializer_list<T> values) {
        for (const T& value : values)
            ins(value);
    }
    nset_splay(const nset_splay& other)
        : pool_(other.pool_), root_(other.root_), compare_(other.compare_), augment_(other.augment_) {}
    nset_splay(nset_splay&& other) noexcept(
        is_nothrow_move_constructible_v<ni::nslot_pool<node>> && is_nothrow_move_constructible_v<C> &&
        is_nothrow_move_constructible_v<A>)
        : pool_(move(other.pool_)), root_(exchange(other.root_, 0)), compare_(move(other.compare_)),
          augment_(move(other.augment_)) {
        other.touch();
    }
    nset_splay& operator=(const nset_splay& other) {
        if (this != addressof(other)) {
            pool_ = other.pool_;
            root_ = other.root_;
            compare_ = other.compare_;
            augment_ = other.augment_;
            touch();
        }
        return *this;
    }
    nset_splay& operator=(nset_splay&& other) noexcept(
        is_nothrow_move_assignable_v<ni::nslot_pool<node>> && is_nothrow_move_assignable_v<C> &&
        is_nothrow_move_assignable_v<A>) {
        if (this != addressof(other)) {
            pool_ = move(other.pool_);
            root_ = exchange(other.root_, 0);
            compare_ = move(other.compare_);
            augment_ = move(other.augment_);
            touch();
            other.touch();
        }
        return *this;
    }

    int len() const { return size_of(root_); }
    bool empty() const noexcept { return root_ == 0; }
    bool equivalent(const T& a, const T& b) const {
        return !invoke(compare_, a, b) && !invoke(compare_, b, a);
    }
    const A& augment() const noexcept { return augment_; }
    node_view root() const { return node_view(this, root_, epoch_); }
    template <class F> node_view walk(F&& decide) const {
        return nwalk(*this, forward<F>(decide));
    }
    template <class P> node_view first_prefix(P&& predicate) const {
        return nfirst_prefix(*this, forward<P>(predicate));
    }
    template <class P> node_view last_suffix(P&& predicate) const {
        return nlast_suffix(*this, forward<P>(predicate));
    }

    void reserve(int capacity) { pool_.reserve(capacity); }
    void clear() {
        if (root_)
            touch();
        pool_.clear();
        root_ = 0;
    }
    int findi(const T& value) const {
        int handle = root_, last = 0;
        while (handle) {
            last = handle;
            if (equivalent(value, pool_[handle].value))
                break;
            handle = child(handle, invoke(compare_, pool_[handle].value, value));
        }
        if (last)
            splay(last);
        return last && equivalent(value, pool_[last].value) ? last : 0;
    }
    bool has(const T& value) const { return findi(value) != 0; }
    int count(const T& value) const {
        int handle = findi(value);
        return handle ? pool_[handle].count : 0;
    }
    const T* get(const T& value) const {
        int handle = findi(value);
        return handle ? addressof(pool_[handle].value) : nullptr;
    }
    int ins(const T& value, int count = 1) {
        T copy = value;
        return ins(move(copy), count);
    }
    int ins(T&& value, int count = 1) {
        npre(count >= 0);
        if (!count)
            return 0;
        npre(len() <= INT_MAX - (Multi ? count : 1));
        if (!root_) {
            int stored_count = Multi ? count : 1;
            auto aggregate = augment_.one(value, stored_count);
            root_ = pool_.make(move(value), stored_count, move(aggregate));
            touch();
            return stored_count;
        }
        int handle = root_, parent = 0;
        while (handle) {
            parent = handle;
            if (equivalent(value, pool_[handle].value)) {
                splay(handle);
                if constexpr (Multi) {
                    npre(pool_[handle].count <= INT_MAX - count);
                    pool_[handle].count += count;
                    pull(handle);
                    touch();
                    return count;
                }
                return 0;
            }
            handle = child(handle, invoke(compare_, pool_[handle].value, value));
        }
        int stored_count = Multi ? count : 1;
        auto aggregate = augment_.one(value, stored_count);
        int fresh = pool_.make(move(value), stored_count, move(aggregate));
        pool_[fresh].parent = parent;
        child(parent, invoke(compare_, pool_[parent].value, pool_[fresh].value)) = fresh;
        splay(fresh);
        return stored_count;
    }
    int del(const T& value, int count = 1) {
        npre(count >= 0);
        if (!count)
            return 0;
        int handle = findi(value);
        if (!handle)
            return 0;
        if constexpr (Multi) {
            if (pool_[handle].count > count) {
                pool_[handle].count -= count;
                pull(handle);
                touch();
                return count;
            }
        }
        int removed = Multi ? pool_[handle].count : 1;
        int left = pool_[handle].left, right = pool_[handle].right;
        pool_.erase(handle);
        touch();
        if (!left) {
            root_ = right;
            if (right)
                pool_[right].parent = 0;
            return removed;
        }
        root_ = left;
        pool_[left].parent = 0;
        int maximum = left;
        while (pool_[maximum].right)
            maximum = pool_[maximum].right;
        splay(maximum);
        pool_[root_].right = right;
        if (right)
            pool_[right].parent = root_;
        pull(root_);
        return removed;
    }
    int delall(const T& value) {
        int multiplicity = count(value);
        return multiplicity ? del(value, multiplicity) : 0;
    }
    int rank(const T& value) const {
        int handle = root_, last = 0, result = 0;
        while (handle) {
            last = handle;
            const node& current = pool_[handle];
            if (invoke(compare_, current.value, value)) {
                result += size_of(current.left) + current.count;
                handle = current.right;
            } else {
                handle = current.left;
            }
        }
        if (last)
            splay(last);
        return result;
    }
    nmaybe<T> kth(int index) const {
        if (index < 0 || index >= len())
            return {};
        int handle = root_;
        for (;;) {
            const node& current = pool_[handle];
            int left_size = size_of(current.left);
            if (index < left_size)
                handle = current.left;
            else if (index < left_size + current.count) {
                splay(handle);
                return pool_[handle].value;
            } else {
                index -= left_size + current.count;
                handle = current.right;
            }
        }
    }
    T kth(int index, T fallback) const {
        auto result = kth(index);
        return result ? result.val() : move(fallback);
    }
    nmaybe<T> lower(const T& value) const { return bound(value, false); }
    nmaybe<T> upper(const T& value) const { return bound(value, true); }
    T lower(const T& value, T fallback) const {
        auto result = lower(value);
        return result ? result.val() : move(fallback);
    }
    T upper(const T& value, T fallback) const {
        auto result = upper(value);
        return result ? result.val() : move(fallback);
    }
    nmaybe<T> min() const { return kth(0); }
    nmaybe<T> max() const { return kth(len() - 1); }
    T min(T fallback) const { return kth(0, move(fallback)); }
    T max(T fallback) const { return kth(len() - 1, move(fallback)); }

    struct cursor {
        const nset_splay* owner;
        vector<int> stack;
        int repetition = 0, index = 0;

        explicit cursor(const nset_splay* owner) : owner(owner) { descend(owner->root_); }
        void descend(int handle) {
            while (handle) {
                stack.push_back(handle);
                handle = owner->pool_[handle].left;
            }
        }
        bool ok() const { return !stack.empty(); }
        const T& val() const { return owner->pool_[stack.back()].value; }
        int idx() const { return index; }
        void next() {
            int handle = stack.back();
            ++index;
            if (++repetition < owner->pool_[handle].count)
                return;
            repetition = 0;
            stack.pop_back();
            descend(owner->pool_[handle].right);
        }
    };
    cursor enumerate() const& { return cursor(this); }
    cursor enumerate() && = delete;

    nset_splay& operator|=(const nset_splay& other)
        requires(!Multi)
    {
        if (this != addressof(other))
            nfor(value, other)
                ins(value);
        return *this;
    }
    nset_splay& operator&=(const nset_splay& other)
        requires(!Multi)
    {
        if (this == addressof(other))
            return *this;
        nvector<T> removed;
        nfor(value, *this)
            if (!other.has(value))
                removed.push(value);
        nfor(value, removed)
            del(value);
        return *this;
    }
    nset_splay& operator-=(const nset_splay& other)
        requires(!Multi)
    {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(value, other)
            del(value);
        return *this;
    }
    nset_splay& operator^=(const nset_splay& other)
        requires(!Multi)
    {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(value, other) {
            if (has(value))
                del(value);
            else
                ins(value);
        }
        return *this;
    }
    friend nset_splay operator|(nset_splay left, const nset_splay& right)
        requires(!Multi)
    {
        return left |= right;
    }
    friend nset_splay operator&(nset_splay left, const nset_splay& right)
        requires(!Multi)
    {
        return left &= right;
    }
    friend nset_splay operator-(nset_splay left, const nset_splay& right)
        requires(!Multi)
    {
        return left -= right;
    }
    friend nset_splay operator^(nset_splay left, const nset_splay& right)
        requires(!Multi)
    {
        return left ^= right;
    }
    friend bool operator==(const nset_splay& left, const nset_splay& right) {
        return ni::nordered_equal(left, right);
    }
};

// Reference ordered-set backend.  C is a strict weak ordering; iterator/reference
// invalidation follows std::set/multiset and no AST node/augmentation API is provided.
template <class T, class C = nless<T>> class nset_stl {
    set<T, C> values_;

  public:
    using value_type = T;

    nset_stl() = default;
    nset_stl(initializer_list<T> values) : values_(values) {}
    explicit nset_stl(C compare) : values_(move(compare)) {}

    int len() const {
        npre(values_.size() <= size_t(INT_MAX));
        return int(values_.size());
    }
    bool empty() const noexcept { return values_.empty(); }
    bool equivalent(const T& a, const T& b) const {
        auto compare = values_.key_comp();
        return !invoke(compare, a, b) && !invoke(compare, b, a);
    }
    void clear() noexcept { values_.clear(); }
    void reserve(int capacity) { npre(capacity >= 0); }
    int ins(const T& value, int count = 1) {
        npre(count >= 0);
        return count ? int(values_.insert(value).second) : 0;
    }
    int ins(T&& value, int count = 1) {
        npre(count >= 0);
        return count ? int(values_.insert(move(value)).second) : 0;
    }
    int del(const T& value, int count = 1) {
        npre(count >= 0);
        return count ? int(values_.erase(value)) : 0;
    }
    int delall(const T& value) { return del(value); }
    bool has(const T& value) const { return values_.contains(value); }
    int count(const T& value) const { return int(values_.contains(value)); }
    const T* get(const T& value) const {
        auto found = values_.find(value);
        return found == values_.end() ? nullptr : addressof(*found);
    }
    int rank(const T& value) const {
        return int(distance(values_.begin(), values_.lower_bound(value)));
    }
    nmaybe<T> kth(int index) const {
        if (index < 0 || index >= len())
            return {};
        auto found = values_.begin();
        advance(found, index);
        return *found;
    }
    T kth(int index, T fallback) const {
        auto result = kth(index);
        return result ? result.val() : move(fallback);
    }
    nmaybe<T> lower(const T& value) const {
        auto found = values_.lower_bound(value);
        return found == values_.end() ? nmaybe<T>{} : nmaybe<T>(*found);
    }
    nmaybe<T> upper(const T& value) const {
        auto found = values_.upper_bound(value);
        return found == values_.end() ? nmaybe<T>{} : nmaybe<T>(*found);
    }
    T lower(const T& value, T fallback) const {
        auto result = lower(value);
        return result ? result.val() : move(fallback);
    }
    T upper(const T& value, T fallback) const {
        auto result = upper(value);
        return result ? result.val() : move(fallback);
    }
    nmaybe<T> min() const { return kth(0); }
    nmaybe<T> max() const { return kth(len() - 1); }
    T min(T fallback) const { return kth(0, move(fallback)); }
    T max(T fallback) const { return kth(len() - 1, move(fallback)); }

    struct cursor {
        const nset_stl* owner;
        typename set<T, C>::const_iterator iterator;
        int index = 0;
        bool ok() const { return iterator != owner->values_.end(); }
        const T& val() const { return *iterator; }
        int idx() const { return index; }
        void next() {
            ++iterator;
            ++index;
        }
    };
    cursor enumerate() const& { return {this, values_.begin()}; }
    cursor enumerate() && = delete;

    nset_stl& operator|=(const nset_stl& other) {
        if (this != addressof(other))
            nfor(value, other)
                ins(value);
        return *this;
    }
    nset_stl& operator&=(const nset_stl& other) {
        if (this == addressof(other))
            return *this;
        for (auto iterator = values_.begin(); iterator != values_.end();)
            if (!other.has(*iterator))
                iterator = values_.erase(iterator);
            else
                ++iterator;
        return *this;
    }
    nset_stl& operator-=(const nset_stl& other) {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(value, other)
            del(value);
        return *this;
    }
    nset_stl& operator^=(const nset_stl& other) {
        if (this == addressof(other)) {
            clear();
            return *this;
        }
        nfor(value, other) {
            if (has(value))
                del(value);
            else
                ins(value);
        }
        return *this;
    }
    friend nset_stl operator|(nset_stl left, const nset_stl& right) { return left |= right; }
    friend nset_stl operator&(nset_stl left, const nset_stl& right) { return left &= right; }
    friend nset_stl operator-(nset_stl left, const nset_stl& right) { return left -= right; }
    friend nset_stl operator^(nset_stl left, const nset_stl& right) { return left ^= right; }
    friend bool operator==(const nset_stl& left, const nset_stl& right) {
        return ni::nordered_equal(left, right);
    }
};

template <class T, class C = nless<T>, class A = nempty_augment<T>,
          class L = nempty_tag<T, typename A::info_type>>
using nset = nset_fhq<T, C, false, A, L>;

template <class T, class C = nless<T>, class A = nempty_augment<T>,
          class L = nempty_tag<T, typename A::info_type>>
using nbag = nset_fhq<T, C, true, A, L>;

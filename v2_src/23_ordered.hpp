namespace ni {
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

template <class T, class C = nless<T>, bool Multi = false, class A = nempty_augment<T>>
    requires naugment<A, T>
class nset_fhq {
    struct node {
        T value;
        uint64_t priority;
        int left = 0, right = 0, size = 1, count = 1;
        typename A::info_type aggregate;

        node(T value, uint64_t priority, int count, typename A::info_type aggregate)
            : value(move(value)), priority(priority), size(count), count(count),
              aggregate(move(aggregate)) {}
    };

    ni::nslot_pool<node> pool_;
    int root_ = 0;
    [[no_unique_address]] C compare_{};
    [[no_unique_address]] A augment_{};
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
    int merge(int left, int right) {
        if (!left)
            return right;
        if (!right)
            return left;
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
    int nnode_left(int handle) const { return handle ? pool_[handle].left : 0; }
    int nnode_right(int handle) const { return handle ? pool_[handle].right : 0; }

  public:
    using value_type = T;
    using augment_type = A;
    using info_type = typename A::info_type;
    using node_view = nnode<nset_fhq>;

    nset_fhq() = default;
    explicit nset_fhq(C compare) : compare_(move(compare)) {}
    explicit nset_fhq(A augment)
        requires(!same_as<C, A>)
        : augment_(move(augment)) {}
    nset_fhq(C compare, A augment) : compare_(move(compare)), augment_(move(augment)) {}
    nset_fhq(initializer_list<T> values) {
        for (const T& value : values)
            ins(value);
    }
    nset_fhq(const nset_fhq& other)
        : pool_(other.pool_), root_(other.root_), compare_(other.compare_), augment_(other.augment_) {}
    nset_fhq(nset_fhq&& other) noexcept(
        is_nothrow_move_constructible_v<ni::nslot_pool<node>> && is_nothrow_move_constructible_v<C> &&
        is_nothrow_move_constructible_v<A>)
        : pool_(move(other.pool_)), root_(exchange(other.root_, 0)), compare_(move(other.compare_)),
          augment_(move(other.augment_)) {
        other.touch();
    }
    nset_fhq& operator=(const nset_fhq& other) {
        if (this != addressof(other)) {
            pool_ = other.pool_;
            root_ = other.root_;
            compare_ = other.compare_;
            augment_ = other.augment_;
            touch();
        }
        return *this;
    }
    nset_fhq& operator=(nset_fhq&& other) noexcept(
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
        int fresh = pool_.make(move(value), nrng_global(), stored_count, move(aggregate));
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

template <class T, class C = nless<T>, bool Multi = false, class A = nempty_augment<T>>
    requires naugment<A, T>
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

template <class T, class C = nless<T>, class A = nempty_augment<T>>
using nset = nset_fhq<T, C, false, A>;

template <class T, class C = nless<T>, class A = nempty_augment<T>>
using nbag = nset_fhq<T, C, true, A>;

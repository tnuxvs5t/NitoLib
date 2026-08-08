// Scratch views borrow storage and are invalidated by the next resize/reallocation or
// by destruction of the owner; never retain them across another space()/filled() call.
template <class T> class nscratch {
    nvector<T> storage_;

  public:
    int cap() const noexcept { return storage_.cap(); }
    void reserve(int n) { storage_.reserve(n); }

    // Any view returned by a previous call may be invalidated by the next resize.
    nview<T> space(int n) {
        storage_.resize(n);
        return {storage_.data(), n};
    }
    nview<T> filled(int n, const T& value = T{}) {
        auto result = space(n);
        for (int i = 0; i < n; ++i)
            result[i] = value;
        return result;
    }
};

// Integer-handle arena.  Handles index owned storage and remain meaningful until
// rewind/clear/move destroys their slot; they are not pointers and have no generation.
template <class T> class narena {
    nvector<T> storage_;

  public:
    int len() const noexcept { return storage_.len(); }
    bool empty() const noexcept { return storage_.empty(); }
    void reserve(int n) { storage_.reserve(n); }
    int mark() const noexcept { return len(); }

    template <class... A> int make(A&&... arguments) {
        npre(len() < INT_MAX);
        int handle = len();
        storage_.push(forward<A>(arguments)...);
        return handle;
    }
    T& operator[](int handle) { return storage_[handle]; }
    const T& operator[](int handle) const { return storage_[handle]; }
    T* get(int handle) noexcept { return storage_.get(handle); }
    const T* get(int handle) const noexcept { return storage_.get(handle); }

    void rollback(int checkpoint) {
        npre(0 <= checkpoint && checkpoint <= len());
        while (len() > checkpoint)
            storage_.pop();
    }
    void clear() noexcept { storage_.clear(); }
};

/**
 * Owning reusable resource pool.
 *
 * Handles are positive integers and zero is the empty handle.  A deleted slot may
 * be reused, but its generation changes; callers that keep a node identity must
 * retain both the handle and the generation (or use a domain epoch).  References
 * and pointers may be invalidated by growth, while integer handles are stable until
 * erase/clear.  This is deliberately a small storage primitive: it does not impose
 * a graph/tree meaning on T and it does not try to encode algebraic laws in types.
 */
template <class T> class nresource_pool {
    struct slot {
        optional<T> value;
        uint64_t generation = 1;
    };

    vector<slot> storage_;
    vector<int> free_;
    int live_ = 0;

  public:
    int len() const noexcept { return live_; }
    int cap() const noexcept {
        npre(storage_.size() <= size_t(INT_MAX));
        return int(storage_.size());
    }
    bool empty() const noexcept { return live_ == 0; }

    void reserve(int capacity) {
        npre(capacity >= 0);
        storage_.reserve(size_t(capacity));
        free_.reserve(size_t(capacity));
    }

    template <class... A> int make(A&&... arguments) {
        npre(live_ < INT_MAX);
        int handle;
        if (free_.empty()) {
            npre(storage_.size() < size_t(INT_MAX));
            storage_.emplace_back();
            handle = int(storage_.size());
            storage_.back().value.emplace(forward<A>(arguments)...);
        } else {
            handle = free_.back();
            free_.pop_back();
            storage_[size_t(handle - 1)].value.emplace(forward<A>(arguments)...);
        }
        ++live_;
        return handle;
    }

    bool alive(int handle) const noexcept {
        return 0 < handle && handle <= int(storage_.size()) &&
               storage_[size_t(handle - 1)].value.has_value();
    }
    uint64_t generation(int handle) const noexcept {
        npre(0 < handle && handle <= int(storage_.size()));
        return storage_[size_t(handle - 1)].generation;
    }
    T& operator[](int handle) {
        npre(alive(handle));
        return *storage_[size_t(handle - 1)].value;
    }
    const T& operator[](int handle) const {
        npre(alive(handle));
        return *storage_[size_t(handle - 1)].value;
    }
    T* get(int handle) noexcept {
        return alive(handle) ? addressof(*storage_[size_t(handle - 1)].value) : nullptr;
    }
    const T* get(int handle) const noexcept {
        return alive(handle) ? addressof(*storage_[size_t(handle - 1)].value) : nullptr;
    }
    void erase(int handle) {
        npre(alive(handle));
        slot& current = storage_[size_t(handle - 1)];
        current.value.reset();
        if (!++current.generation)
            ++current.generation;
        free_.push_back(handle);
        --live_;
    }
    void del(int handle) { erase(handle); }
    void clear() noexcept {
        storage_.clear();
        free_.clear();
        live_ = 0;
    }
};

/**
 * Shared node domain: one resource pool and one structural epoch for several
 * cooperating owners.  Copying a domain shares it; `clone()` makes an independent
 * deep copy for an owning container copy.  A domain does not know whether its T is a
 * sequence node, ordered-tree node, segment node or graph record.  The owner above
 * it decides which roots are live and calls touch() once per public mutation.
 *
 * The shared epoch is intentional.  A view into one owner becomes stale when another
 * owner rewires the same domain, which is safer than allowing an old handle to look
 * current after cross-owner merge/split.  `make/erase` are low-level allocation
 * primitives and do not touch the epoch; higher layers publish one transaction with
 * touch() after restoring all invariants.
 */
template <class T> class nnode_domain {
    struct state {
        nresource_pool<T> resources;
        uint64_t epoch = 1;
    };

    shared_ptr<state> state_ = make_shared<state>();

    state& writable() {
        if (!state_)
            state_ = make_shared<state>();
        return *state_;
    }
  public:
    nnode_domain() = default;
    nnode_domain(const nnode_domain&) = default;
    nnode_domain& operator=(const nnode_domain&) = default;
    nnode_domain(nnode_domain&&) noexcept = default;
    nnode_domain& operator=(nnode_domain&&) noexcept = default;

    nnode_domain clone() const {
        nnode_domain result;
        if (state_)
            result.state_ = make_shared<state>(*state_);
        return result;
    }

    bool same_domain(const nnode_domain& other) const noexcept {
        return state_.get() == other.state_.get();
    }
    const void* domain_token() const noexcept { return state_.get(); }
    uint64_t epoch() const noexcept { return state_ ? state_->epoch : 0; }
    void touch() noexcept {
        // A moved-from domain has no live roots.  Keeping touch() noexcept lets
        // owners invalidate the moved-from shell without allocating; its next
        // actual allocation recreates the state through writable().
        if (!state_)
            return;
        state& current = *state_;
        if (!++current.epoch)
            ++current.epoch;
    }

    int len() const noexcept { return state_ ? state_->resources.len() : 0; }
    int cap() const noexcept { return state_ ? state_->resources.cap() : 0; }
    bool empty() const noexcept { return len() == 0; }
    void reserve(int capacity) { writable().resources.reserve(capacity); }
    template <class... A> int make(A&&... arguments) {
        return writable().resources.make(forward<A>(arguments)...);
    }
    bool alive(int handle) const noexcept { return state_ && state_->resources.alive(handle); }
    uint64_t generation(int handle) const noexcept {
        npre(state_);
        return state_->resources.generation(handle);
    }
    nnode_identity identity(int handle) const noexcept {
        npre(alive(handle));
        return {domain_token(), handle, generation(handle)};
    }
    T& operator[](int handle) { return writable().resources[handle]; }
    const T& operator[](int handle) const {
        npre(state_);
        return state_->resources[handle];
    }
    T* get(int handle) noexcept { return state_ ? state_->resources.get(handle) : nullptr; }
    const T* get(int handle) const noexcept {
        return state_ ? state_->resources.get(handle) : nullptr;
    }
    void erase(int handle) { writable().resources.erase(handle); }
    void del(int handle) { erase(handle); }
    void clear() {
        writable().resources.clear();
        touch();
    }
};

template <class T> using nnode_pool = nnode_domain<T>;

// The legacy names remain available, but all new storage code goes through the
// resource primitive above so AST, ordered trees and future graph domains share one
// lifetime/generation vocabulary.
template <class T> using npool_dynamic = nresource_pool<T>;
template <class T> using npool = nresource_pool<T>;

namespace ni {
template <class T> using nslot_pool = nresource_pool<T>;
} // namespace ni

/**
 * Owning hash map with separate chaining.  H and E must agree on key identity
 * (equal keys have equal hashes); mutating operations may invalidate references and
 * enumeration cursors.  nseed must be called before construction for reproducibility.
 */
template <class K, class V, class H = nhash<K>, class E = equal_to<K>> class nmap_hash {
    using storage_type = unordered_map<K, V, H, E>;
    storage_type values_;

  public:
    using key_type = K;
    using mapped_type = V;

    nmap_hash() = default;
    explicit nmap_hash(int expected, H hash = {}, E equal = {})
        : values_(0, move(hash), move(equal)) {
        reserve(expected);
    }

    int len() const {
        npre(values_.size() <= size_t(INT_MAX));
        return int(values_.size());
    }
    bool empty() const noexcept { return values_.empty(); }
    void clear() noexcept { values_.clear(); }
    void reserve(int expected) {
        npre(expected >= 0);
        values_.reserve(size_t(expected));
    }
    bool has(const K& key) const { return values_.contains(key); }
    V* get(const K& key) {
        auto found = values_.find(key);
        return found == values_.end() ? nullptr : addressof(found->second);
    }
    const V* get(const K& key) const {
        auto found = values_.find(key);
        return found == values_.end() ? nullptr : addressof(found->second);
    }
    V get(const K& key, V fallback) const {
        const V* found = get(key);
        return found ? *found : move(fallback);
    }
    bool ins(const K& key, const V& value) { return values_.emplace(key, value).second; }
    bool ins(K&& key, V&& value) { return values_.emplace(move(key), move(value)).second; }
    template <class X> V& set(const K& key, X&& value) {
        return values_.insert_or_assign(key, forward<X>(value)).first->second;
    }
    int del(const K& key) { return int(values_.erase(key)); }
    V& operator[](const K& key) { return values_[key]; }
    V& operator[](K&& key) { return values_[move(key)]; }
    V& operator()(const K& key) {
        V* found = get(key);
        npre(found != nullptr);
        return *found;
    }
    const V& operator()(const K& key) const {
        const V* found = get(key);
        npre(found != nullptr);
        return *found;
    }

    struct cursor {
        nmap_hash* owner;
        typename storage_type::iterator iterator;
        int index = 0;
        bool ok() const { return iterator != owner->values_.end(); }
        const K& key() const { return iterator->first; }
        V& val() const { return iterator->second; }
        int idx() const { return index; }
        void next() {
            ++iterator;
            ++index;
        }
    };
    struct const_cursor {
        const nmap_hash* owner;
        typename storage_type::const_iterator iterator;
        int index = 0;
        bool ok() const { return iterator != owner->values_.end(); }
        const K& key() const { return iterator->first; }
        const V& val() const { return iterator->second; }
        int idx() const { return index; }
        void next() {
            ++iterator;
            ++index;
        }
    };
    cursor enumerate() & { return {this, values_.begin()}; }
    const_cursor enumerate() const& { return {this, values_.begin()}; }
    cursor enumerate() && = delete;

    friend bool operator==(const nmap_hash& left, const nmap_hash& right) {
        if (left.len() != right.len())
            return false;
        for (const auto& [key, value] : left.values_) {
            const V* found = right.get(key);
            if (!found || !(*found == value))
                return false;
        }
        return true;
    }
};

/**
 * Open-addressing owning map.  H/E obey the same equivalence contract, and load factor
 * is bounded by the implementation.  Rehashing invalidates all borrowed references.
 */
template <class K, class V, class H = nhash<K>, class E = equal_to<K>> class nmap_flat {
    struct node {
        K key;
        V value;
        size_t hash;
    };

    vector<node> entries_;
    vector<int> buckets_;
    int tombstones_ = 0;
    [[no_unique_address]] H hash_{};
    [[no_unique_address]] E equal_{};

    static int bucket_capacity(uint64_t requested) {
        requested = max<uint64_t>(8, requested);
        npre(requested <= (uint64_t(1) << 30));
        return int(bit_ceil(uint32_t(requested)));
    }
    void place(int entry) {
        int mask = cap() - 1;
        int bucket = int(entries_[entry].hash & size_t(mask));
        while (buckets_[bucket] > 0)
            bucket = (bucket + 1) & mask;
        buckets_[bucket] = entry + 1;
    }
    void rehash(int capacity) {
        capacity = bucket_capacity(uint64_t(capacity));
        buckets_.assign(size_t(capacity), 0);
        tombstones_ = 0;
        for (int entry = 0; entry < len(); ++entry)
            place(entry);
    }
    void ensure_insert_capacity() {
        if (!cap()) {
            rehash(8);
            return;
        }
        if ((1LL * len() + tombstones_ + 1) * 10 <= 1LL * cap() * 7)
            return;
        if (tombstones_ > len())
            rehash(cap());
        else {
            npre(cap() <= (1 << 29));
            rehash(cap() * 2);
        }
    }
    int slot(const K& key, size_t hash, bool& found) const {
        if (!cap()) {
            found = false;
            return 0;
        }
        int mask = cap() - 1;
        int bucket = int(hash & size_t(mask));
        int first_tombstone = npos;
        for (int probes = 0; probes < cap(); ++probes, bucket = (bucket + 1) & mask) {
            int encoded = buckets_[bucket];
            if (!encoded) {
                found = false;
                return first_tombstone == npos ? bucket : first_tombstone;
            }
            if (encoded < 0) {
                if (first_tombstone == npos)
                    first_tombstone = bucket;
                continue;
            }
            const node& candidate = entries_[encoded - 1];
            if (candidate.hash == hash && invoke(equal_, candidate.key, key)) {
                found = true;
                return bucket;
            }
        }
        npre(first_tombstone != npos);
        found = false;
        return first_tombstone;
    }
    template <class X, class Y> bool insert_impl(X&& key, Y&& value) {
        ensure_insert_capacity();
        size_t hash = size_t(invoke(hash_, key));
        bool found;
        int bucket = slot(key, hash, found);
        if (found)
            return false;
        npre(entries_.size() < size_t(INT_MAX));
        if (buckets_[bucket] < 0)
            --tombstones_;
        entries_.push_back({forward<X>(key), forward<Y>(value), hash});
        buckets_[bucket] = len();
        return true;
    }

  public:
    using key_type = K;
    using mapped_type = V;

    nmap_flat() = default;
    explicit nmap_flat(int expected, H hash = {}, E equal = {})
        : hash_(move(hash)), equal_(move(equal)) {
        reserve(expected);
    }

    int len() const {
        npre(entries_.size() <= size_t(INT_MAX));
        return int(entries_.size());
    }
    int cap() const {
        npre(buckets_.size() <= size_t(INT_MAX));
        return int(buckets_.size());
    }
    bool empty() const noexcept { return entries_.empty(); }
    void reserve(int expected) {
        npre(expected >= 0);
        uint64_t required = (uint64_t(expected) * 10 + 6) / 7;
        npre(required <= (uint64_t(1) << 30));
        entries_.reserve(size_t(expected));
        int capacity = bucket_capacity(required);
        if (capacity > cap())
            rehash(capacity);
    }
    void clear() noexcept {
        entries_.clear();
        fill(buckets_.begin(), buckets_.end(), 0);
        tombstones_ = 0;
    }
    bool has(const K& key) const { return get(key) != nullptr; }
    V* get(const K& key) {
        bool found;
        int bucket = slot(key, size_t(invoke(hash_, key)), found);
        return found ? addressof(entries_[buckets_[bucket] - 1].value) : nullptr;
    }
    const V* get(const K& key) const {
        bool found;
        int bucket = slot(key, size_t(invoke(hash_, key)), found);
        return found ? addressof(entries_[buckets_[bucket] - 1].value) : nullptr;
    }
    V get(const K& key, V fallback) const {
        const V* found = get(key);
        return found ? *found : move(fallback);
    }
    bool ins(const K& key, const V& value) { return insert_impl(key, value); }
    bool ins(K&& key, V&& value) { return insert_impl(move(key), move(value)); }
    template <class X> V& set(const K& key, X&& value) {
        if (V* found = get(key)) {
            *found = forward<X>(value);
            return *found;
        }
        insert_impl(key, forward<X>(value));
        return entries_.back().value;
    }
    V& operator[](const K& key) {
        if (V* found = get(key))
            return *found;
        insert_impl(key, V{});
        return entries_.back().value;
    }
    V& operator[](K&& key) {
        if (V* found = get(key))
            return *found;
        insert_impl(move(key), V{});
        return entries_.back().value;
    }
    V& operator()(const K& key) {
        V* found = get(key);
        npre(found != nullptr);
        return *found;
    }
    const V& operator()(const K& key) const {
        const V* found = get(key);
        npre(found != nullptr);
        return *found;
    }
    int del(const K& key) {
        if (!cap())
            return 0;
        size_t hash = size_t(invoke(hash_, key));
        bool found;
        int bucket = slot(key, hash, found);
        if (!found)
            return 0;
        int erased = buckets_[bucket] - 1;
        int last = len() - 1;
        buckets_[bucket] = -1;
        ++tombstones_;
        if (erased != last) {
            entries_[erased] = move(entries_[last]);
            int mask = cap() - 1;
            int moved_bucket = int(entries_[erased].hash & size_t(mask));
            for (int probes = 0; probes < cap(); ++probes,
                     moved_bucket = (moved_bucket + 1) & mask) {
                if (buckets_[moved_bucket] == last + 1) {
                    buckets_[moved_bucket] = erased + 1;
                    break;
                }
                npre(probes + 1 < cap());
            }
        }
        entries_.pop_back();
        if (tombstones_ > len() && cap() > 8)
            rehash(cap());
        return 1;
    }

    struct cursor {
        nmap_flat* owner;
        int index = 0;
        bool ok() const { return index < owner->len(); }
        const K& key() const { return owner->entries_[index].key; }
        V& val() const { return owner->entries_[index].value; }
        int idx() const { return index; }
        void next() { ++index; }
    };
    struct const_cursor {
        const nmap_flat* owner;
        int index = 0;
        bool ok() const { return index < owner->len(); }
        const K& key() const { return owner->entries_[index].key; }
        const V& val() const { return owner->entries_[index].value; }
        int idx() const { return index; }
        void next() { ++index; }
    };
    cursor enumerate() & { return {this}; }
    const_cursor enumerate() const& { return {this}; }
    cursor enumerate() && = delete;

    friend bool operator==(const nmap_flat& left, const nmap_flat& right) {
        if (left.len() != right.len())
            return false;
        for (const node& entry : left.entries_) {
            const V* found = right.get(entry.key);
            if (!found || !(*found == entry.value))
                return false;
        }
        return true;
    }
};

template <class K, class V, class H = nhash<K>, class E = equal_to<K>>
using nmap_stl = nmap_hash<K, V, H, E>;

template <class K, class V, class H = nhash<K>, class E = equal_to<K>>
using nmap = nmap_flat<K, V, H, E>;

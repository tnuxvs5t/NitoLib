template <class T> class nscratch {
    nvector<T> storage_;

  public:
    int cap() const noexcept { return storage_.cap(); }
    void reserve(int n) { storage_.reserve(n); }

    // Any span returned by a previous call may be invalidated by the next resize.
    nspan<T> space(int n) {
        storage_.resize(n);
        return {storage_.data(), n};
    }
    nspan<T> filled(int n, const T& value = T{}) {
        auto result = space(n);
        for (int i = 0; i < n; ++i)
            result[i] = value;
        return result;
    }
};

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

template <class T> class npool_dynamic {
    vector<optional<T>> storage_;
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
        int handle;
        if (free_.empty()) {
            npre(storage_.size() < size_t(INT_MAX));
            storage_.emplace_back(in_place, forward<A>(arguments)...);
            handle = int(storage_.size());
        } else {
            handle = free_.back();
            free_.pop_back();
            storage_[handle - 1].emplace(forward<A>(arguments)...);
        }
        ++live_;
        return handle;
    }
    void del(int handle) {
        npre(get(handle) != nullptr);
        storage_[handle - 1].reset();
        free_.push_back(handle);
        --live_;
    }
    T& operator[](int handle) {
        T* value = get(handle);
        npre(value != nullptr);
        return *value;
    }
    const T& operator[](int handle) const {
        const T* value = get(handle);
        npre(value != nullptr);
        return *value;
    }
    T* get(int handle) noexcept {
        return 0 < handle && handle <= cap() && storage_[handle - 1]
                   ? addressof(*storage_[handle - 1])
                   : nullptr;
    }
    const T* get(int handle) const noexcept {
        return 0 < handle && handle <= cap() && storage_[handle - 1]
                   ? addressof(*storage_[handle - 1])
                   : nullptr;
    }
    void clear() noexcept {
        storage_.clear();
        free_.clear();
        live_ = 0;
    }
};

template <class T> using npool = npool_dynamic<T>;

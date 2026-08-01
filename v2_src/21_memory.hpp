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

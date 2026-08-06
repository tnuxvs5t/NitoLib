template <class T> class nvector {
    vector<T> storage_;

    static size_t checked_size(int n) {
        npre(n >= 0);
        return size_t(n);
    }

  public:
    using value_type = T;

    nvector() = default;
    explicit nvector(int n) : storage_(checked_size(n)) {}
    nvector(int n, const T& value) : storage_(checked_size(n), value) {}
    nvector(initializer_list<T> values) : storage_(values) {}

    int len() const noexcept {
        npre(storage_.size() <= size_t(INT_MAX));
        return int(storage_.size());
    }
    int cap() const noexcept {
        npre(storage_.capacity() <= size_t(INT_MAX));
        return int(storage_.capacity());
    }
    bool empty() const noexcept { return storage_.empty(); }
    T* data() noexcept { return storage_.data(); }
    const T* data() const noexcept { return storage_.data(); }

    T& operator[](int i) {
        npre(0 <= i && i < len());
        return storage_[i];
    }
    const T& operator[](int i) const {
        npre(0 <= i && i < len());
        return storage_[i];
    }
    T* get(int i) noexcept { return 0 <= i && i < len() ? addressof(storage_[i]) : nullptr; }
    const T* get(int i) const noexcept { return 0 <= i && i < len() ? addressof(storage_[i]) : nullptr; }
    T get(int i, T fallback) const { return 0 <= i && i < len() ? storage_[i] : move(fallback); }

    void reserve(int n) {
        npre(n >= 0);
        storage_.reserve(size_t(n));
    }
    void resize(int n) {
        npre(n >= 0);
        storage_.resize(size_t(n));
    }
    void resize(int n, const T& value) {
        npre(n >= 0);
        storage_.resize(size_t(n), value);
    }
    void clear() noexcept { storage_.clear(); }

    template <class... A> T& push(A&&... args) {
        npre(storage_.size() < size_t(INT_MAX));
        return storage_.emplace_back(forward<A>(args)...);
    }
    T pop() {
        npre(!empty());
        T value = move(storage_.back());
        storage_.pop_back();
        return value;
    }
    T pop(T fallback) { return empty() ? move(fallback) : pop(); }

    T& front() {
        npre(!empty());
        return storage_.front();
    }
    const T& front() const {
        npre(!empty());
        return storage_.front();
    }
    T front(T fallback) const { return empty() ? move(fallback) : storage_.front(); }
    T& back() {
        npre(!empty());
        return storage_.back();
    }
    const T& back() const {
        npre(!empty());
        return storage_.back();
    }
    T back(T fallback) const { return empty() ? move(fallback) : storage_.back(); }

    void del(int index) {
        npre(0 <= index && index < len());
        storage_.erase(storage_.begin() + index);
    }
    T swapdel(int index) {
        npre(0 <= index && index < len());
        T result = move(storage_[index]);
        if (index + 1 < len())
            storage_[index] = move(storage_.back());
        storage_.pop_back();
        return result;
    }
    nvector& operator+=(const T& value) {
        push(value);
        return *this;
    }
    nvector& operator+=(T&& value) {
        push(move(value));
        return *this;
    }

    friend bool operator==(const nvector&, const nvector&) = default;
};

template <class T> using nvector_stl = nvector<T>;

namespace ni {
template <class T> struct nowned_value_impl {
    using type = T;
};

template <class A, class B> struct nowned_value_impl<pair<A, B>> {
    using type = pair<typename nowned_value_impl<remove_cvref_t<A>>::type,
                      typename nowned_value_impl<remove_cvref_t<B>>::type>;
};

template <class... T> struct nowned_value_impl<tuple<T...>> {
    using type = tuple<typename nowned_value_impl<remove_cvref_t<T>>::type...>;
};

template <class T>
using nowned_value_t = typename nowned_value_impl<remove_cvref_t<T>>::type;

template <class A, class = void> struct ncollect_value {
    using cursor_type = nenumerator_t<A>;
    using type = nowned_value_t<decltype(declval<cursor_type&>().val())>;
};

template <class A>
struct ncollect_value<A, void_t<typename remove_cvref_t<A>::value_type>> {
    using type = nowned_value_t<typename remove_cvref_t<A>::value_type>;
};
} // namespace ni

template <class T = void, class A>
    requires nenumerable<A&&>
auto ncollect(A&& source) {
    using inferred_type = typename ni::ncollect_value<A&&>::type;
    using value_type = conditional_t<same_as<T, void>, inferred_type, ni::nowned_value_t<T>>;
    nvector<value_type> result;
    if constexpr (requires { nlen(source); })
        result.reserve(nlen(source));
    nfor(value, forward<A>(source))
        result.push(forward<decltype(value)>(value));
    return result;
}

template <class G>
    requires nkeyed_indexed<remove_reference_t<G>>
auto ntabulate(G&& function) {
    return ncollect(forward<G>(function));
}

template <class T> class ndeque_stl {
    deque<T> storage_;

  public:
    using value_type = T;

    ndeque_stl() = default;
    ndeque_stl(initializer_list<T> values) : storage_(values) {}

    int len() const noexcept {
        npre(storage_.size() <= size_t(INT_MAX));
        return int(storage_.size());
    }
    bool empty() const noexcept { return storage_.empty(); }

    T& operator[](int i) {
        npre(0 <= i && i < len());
        return storage_[i];
    }
    const T& operator[](int i) const {
        npre(0 <= i && i < len());
        return storage_[i];
    }
    T* get(int i) noexcept { return 0 <= i && i < len() ? addressof(storage_[i]) : nullptr; }
    const T* get(int i) const noexcept { return 0 <= i && i < len() ? addressof(storage_[i]) : nullptr; }
    T get(int i, T fallback) const { return 0 <= i && i < len() ? storage_[i] : move(fallback); }

    template <class... A> T& pushr(A&&... args) {
        npre(storage_.size() < size_t(INT_MAX));
        return storage_.emplace_back(forward<A>(args)...);
    }
    template <class... A> T& pushl(A&&... args) {
        npre(storage_.size() < size_t(INT_MAX));
        return storage_.emplace_front(forward<A>(args)...);
    }
    T popr() {
        npre(!empty());
        T value = move(storage_.back());
        storage_.pop_back();
        return value;
    }
    T popl() {
        npre(!empty());
        T value = move(storage_.front());
        storage_.pop_front();
        return value;
    }
    T popr(T fallback) { return empty() ? move(fallback) : popr(); }
    T popl(T fallback) { return empty() ? move(fallback) : popl(); }

    T& front() {
        npre(!empty());
        return storage_.front();
    }
    const T& front() const {
        npre(!empty());
        return storage_.front();
    }
    T front(T fallback) const { return empty() ? move(fallback) : storage_.front(); }
    T& back() {
        npre(!empty());
        return storage_.back();
    }
    const T& back() const {
        npre(!empty());
        return storage_.back();
    }
    T back(T fallback) const { return empty() ? move(fallback) : storage_.back(); }
    void clear() noexcept { storage_.clear(); }

    ndeque_stl& operator+=(const T& value) {
        pushr(value);
        return *this;
    }
    ndeque_stl& operator+=(T&& value) {
        pushr(move(value));
        return *this;
    }
};

template <class T> class ndeque_ring {
    vector<optional<T>> storage_;
    int first_ = 0, size_ = 0;

    int physical(int index) const {
        npre(cap() > 0);
        return (first_ + index) & (cap() - 1);
    }
    void rebuild(int capacity) {
        npre(capacity > size_ && has_single_bit(unsigned(capacity)));
        auto next = vector<optional<T>>(size_t(capacity));
        for (int index = 0; index < size_; ++index)
            next[index].emplace(move_if_noexcept((*this)[index]));
        storage_.swap(next);
        first_ = 0;
    }
    void grow() {
        npre(cap() <= INT_MAX / 2);
        rebuild(cap() ? 2 * cap() : 1);
    }

  public:
    using value_type = T;

    ndeque_ring() = default;
    ndeque_ring(const ndeque_ring&) = default;
    ndeque_ring& operator=(const ndeque_ring&) = default;
    ndeque_ring(ndeque_ring&& other) noexcept
        : storage_(move(other.storage_)), first_(exchange(other.first_, 0)),
          size_(exchange(other.size_, 0)) {}
    ndeque_ring& operator=(ndeque_ring&& other) noexcept {
        if (this != addressof(other)) {
            storage_ = move(other.storage_);
            first_ = exchange(other.first_, 0);
            size_ = exchange(other.size_, 0);
        }
        return *this;
    }
    ndeque_ring(initializer_list<T> values) {
        reserve(int(values.size()));
        for (const T& value : values)
            pushr(value);
    }

    int len() const noexcept { return size_; }
    int cap() const noexcept {
        npre(storage_.size() <= size_t(INT_MAX));
        return int(storage_.size());
    }
    bool empty() const noexcept { return size_ == 0; }
    void reserve(int capacity) {
        npre(capacity >= 0);
        if (capacity <= cap())
            return;
        unsigned target = bit_ceil(unsigned(capacity));
        npre(target <= unsigned(INT_MAX));
        rebuild(int(target));
    }

    T& operator[](int index) {
        npre(0 <= index && index < size_);
        return *storage_[physical(index)];
    }
    const T& operator[](int index) const {
        npre(0 <= index && index < size_);
        return *storage_[physical(index)];
    }
    T* get(int index) noexcept {
        return 0 <= index && index < size_ ? addressof(*storage_[physical(index)]) : nullptr;
    }
    const T* get(int index) const noexcept {
        return 0 <= index && index < size_ ? addressof(*storage_[physical(index)]) : nullptr;
    }
    T get(int index, T fallback) const {
        return 0 <= index && index < size_ ? (*this)[index] : move(fallback);
    }

    template <class... A> T& pushr(A&&... arguments) {
        if (size_ == cap())
            grow();
        int index = physical(size_);
        npre(!storage_[index]);
        storage_[index].emplace(forward<A>(arguments)...);
        ++size_;
        return *storage_[index];
    }
    template <class... A> T& pushl(A&&... arguments) {
        if (size_ == cap())
            grow();
        first_ = (first_ - 1) & (cap() - 1);
        npre(!storage_[first_]);
        storage_[first_].emplace(forward<A>(arguments)...);
        ++size_;
        return *storage_[first_];
    }
    T popr() {
        npre(!empty());
        int index = physical(size_ - 1);
        T result = move(*storage_[index]);
        storage_[index].reset();
        --size_;
        if (!size_)
            first_ = 0;
        return result;
    }
    T popl() {
        npre(!empty());
        int index = first_;
        T result = move(*storage_[index]);
        storage_[index].reset();
        --size_;
        first_ = size_ ? (first_ + 1) & (cap() - 1) : 0;
        return result;
    }
    T popr(T fallback) { return empty() ? move(fallback) : popr(); }
    T popl(T fallback) { return empty() ? move(fallback) : popl(); }

    T& front() {
        npre(!empty());
        return (*this)[0];
    }
    const T& front() const {
        npre(!empty());
        return (*this)[0];
    }
    T front(T fallback) const { return empty() ? move(fallback) : (*this)[0]; }
    T& back() {
        npre(!empty());
        return (*this)[size_ - 1];
    }
    const T& back() const {
        npre(!empty());
        return (*this)[size_ - 1];
    }
    T back(T fallback) const { return empty() ? move(fallback) : (*this)[size_ - 1]; }

    void clear() noexcept {
        for (int index = 0; index < size_; ++index)
            storage_[physical(index)].reset();
        first_ = size_ = 0;
    }
    ndeque_ring& operator+=(const T& value) {
        pushr(value);
        return *this;
    }
    ndeque_ring& operator+=(T&& value) {
        pushr(move(value));
        return *this;
    }
};

template <class T> using ndeque = ndeque_ring<T>;

template <class T, class C = nless<T>> class nheap_binary {
    nvector<T> values_;
    [[no_unique_address]] C compare_{};

    int up(int index) {
        while (index) {
            int parent = (index - 1) / 2;
            if (!invoke(compare_, values_[index], values_[parent]))
                break;
            ranges::swap(values_[index], values_[parent]);
            index = parent;
        }
        return index;
    }

    void down(int index) {
        for (;;) {
            long long first_child = 2LL * index + 1;
            if (first_child >= len())
                return;
            int child = int(first_child);
            if (child + 1 < len() && invoke(compare_, values_[child + 1], values_[child]))
                ++child;
            if (!invoke(compare_, values_[child], values_[index]))
                return;
            ranges::swap(values_[index], values_[child]);
            index = child;
        }
    }

  public:
    using value_type = T;

    nheap_binary() = default;
    explicit nheap_binary(C compare) : compare_(move(compare)) {}

    template <class A>
        requires nenumerable<const A&>
    explicit nheap_binary(const A& source, C compare = {}) : compare_(move(compare)) {
        if constexpr (requires { nlen(source); })
            values_.reserve(nlen(source));
        nfor(value, source)
            values_.push(value);
        for (int root = len() / 2; root-- > 0;)
            down(root);
    }

    int len() const noexcept { return values_.len(); }
    bool empty() const noexcept { return values_.empty(); }
    void clear() noexcept { values_.clear(); }
    void reserve(int capacity) { values_.reserve(capacity); }

    template <class... A> T& push(A&&... args) {
        values_.push(forward<A>(args)...);
        return values_[up(len() - 1)];
    }

    const T& top() const {
        npre(!empty());
        return values_[0];
    }
    T top(T fallback) const { return empty() ? move(fallback) : values_[0]; }

    T pop() {
        npre(!empty());
        T result = move(values_[0]);
        if (len() == 1) {
            values_.pop();
            return result;
        }
        values_[0] = values_.pop();
        down(0);
        return result;
    }
    T pop(T fallback) { return empty() ? move(fallback) : pop(); }

    void replace(T value) {
        npre(!empty());
        values_[0] = move(value);
        down(0);
    }
};

template <class T, class C = nless<T>> using nheap = nheap_binary<T, C>;

template <class T, int Rank>
    requires(Rank > 0)
class narray {
    array<int, Rank> shape_{};
    vector<T> storage_;

    static int volume(const array<int, Rank>& shape) {
        long long product = 1;
        for (int extent : shape) {
            npre(extent >= 0);
            if (extent == 0) {
                product = 0;
                continue;
            }
            if (product == 0)
                continue;
            npre(product <= INT_MAX / extent);
            product *= extent;
        }
        return int(product);
    }

    template <integral I> static constexpr int coordinate(I value) { return ni::nchecked_int(value); }

  public:
    using value_type = T;
    using coord_type = array<int, Rank>;

    narray() = default;
    explicit narray(coord_type shape) : shape_(shape), storage_(size_t(volume(shape))) {}
    narray(coord_type shape, const T& value) : shape_(shape), storage_(size_t(volume(shape)), value) {}

    static constexpr int rank() noexcept { return Rank; }
    int len() const noexcept { return int(storage_.size()); }
    bool empty() const noexcept { return storage_.empty(); }
    int dim(int axis, int fallback = npos) const noexcept {
        return 0 <= axis && axis < Rank ? shape_[axis] : fallback;
    }
    nview<const int> shape() const noexcept { return {shape_.data(), Rank}; }
    T* data() noexcept { return storage_.data(); }
    const T* data() const noexcept { return storage_.data(); }

    int pos(const coord_type& coord, int fallback = npos) const {
        int index = 0;
        for (int axis = 0; axis < Rank; ++axis) {
            if (coord[axis] < 0 || coord[axis] >= shape_[axis])
                return fallback;
            index = index * shape_[axis] + coord[axis];
        }
        return index;
    }

    T& operator[](int i) {
        npre(0 <= i && i < len());
        return storage_[i];
    }
    const T& operator[](int i) const {
        npre(0 <= i && i < len());
        return storage_[i];
    }
    T& operator()(const coord_type& coord) {
        int i = pos(coord);
        npre(i != npos);
        return storage_[i];
    }
    const T& operator()(const coord_type& coord) const {
        int i = pos(coord);
        npre(i != npos);
        return storage_[i];
    }

    template <class... I>
        requires(sizeof...(I) == Rank && (integral<remove_cvref_t<I>> && ...))
    T& operator()(I... coord) {
        return (*this)(coord_type{coordinate(coord)...});
    }
    template <class... I>
        requires(sizeof...(I) == Rank && (integral<remove_cvref_t<I>> && ...))
    const T& operator()(I... coord) const {
        return (*this)(coord_type{coordinate(coord)...});
    }
};

namespace ni {
template <class C, class P> class nprojected_compare {
    [[no_unique_address]] C compare_;
    [[no_unique_address]] P projection_;

  public:
    constexpr nprojected_compare(C compare, P projection)
        : compare_(move(compare)), projection_(move(projection)) {}

    template <class L, class R> constexpr bool operator()(L&& left, R&& right) {
        return invoke(compare_, invoke(projection_, forward<L>(left)),
                      invoke(projection_, forward<R>(right)));
    }
};

template <class A, class P>
using nprojected_value_t =
    remove_cvref_t<invoke_result_t<P&, nindex_reference_t<A>>>;

template <class A, class C> void nheap_sift(A& a, int root, int count, C& compare) {
    for (;;) {
        long long first_child = 2LL * root + 1;
        if (first_child >= count)
            return;
        int child = int(first_child);
        if (child + 1 < count && compare(a[child], a[child + 1]))
            ++child;
        if (!compare(a[root], a[child]))
            return;
        ranges::swap(a[root], a[child]);
        root = child;
    }
}

template <class A, class C> void nheap_sort(A& a, C& compare) {
    int n = nlen(a);
    for (int root = n / 2; root-- > 0;)
        nheap_sift(a, root, n, compare);
    for (int count = n; count > 1;) {
        ranges::swap(a[0], a[--count]);
        nheap_sift(a, 0, count, compare);
    }
}
} // namespace ni

// Basic write operations deliberately take the destination first. They mutate
// lvalue owners or temporary view descriptors without copying the owner.
template <class A, class X>
    requires nviewable_indexed<A&&> && nreference_indexed<remove_reference_t<A>> &&
             requires(remove_reference_t<A>& destination, const X& value) {
                 destination[0] = value;
             }
void nfill(A&& destination, const X& value) {
    int size = nlen(destination);
    for (int i = 0; i < size; ++i)
        destination[i] = value;
}

template <class D, class S, class P = nidentity>
    requires nviewable_indexed<D&&> && nreference_indexed<remove_reference_t<D>> &&
             nenumerable<S&&> &&
             requires(remove_reference_t<D>& destination,
                      nenumerator_t<S&&>& cursor, P& projection) {
                 destination[0] = invoke(projection, cursor.val());
             }
void nassign(D&& destination, S&& source, P projection = {}) {
    int size = nlen(destination);
    auto cursor = nenumerate(forward<S>(source));
    constexpr bool sized_source = requires { nlen(source); };
    if constexpr (sized_source)
        npre(size == nlen(source));
    for (int i = 0; i < size; ++i) {
        if constexpr (!sized_source)
            npre(cursor.ok());
        destination[i] = invoke(projection, cursor.val());
        cursor.next();
    }
    if constexpr (!sized_source)
        npre(!cursor.ok());
}

template <class A, class B>
    requires nviewable_indexed<A&&> && nviewable_indexed<B&&> &&
             nswappable_indexed<remove_reference_t<A>> &&
             nswappable_indexed<remove_reference_t<B>> &&
             requires(remove_reference_t<A>& left, remove_reference_t<B>& right) {
                 ranges::swap(left[0], right[0]);
             }
void nswap_ranges(A&& left, B&& right) {
    int size = nlen(left);
    npre(size == nlen(right));
    for (int i = 0; i < size; ++i)
        ranges::swap(left[i], right[i]);
}

template <class A, class F, class P = nidentity>
    requires nenumerable<A&&>
int nfind_if(A&& sequence, F predicate, P projection = {}, int fallback = npos) {
    nfori(index, value, forward<A>(sequence))
        if (invoke(predicate, invoke(projection, value)))
            return index;
    return fallback;
}

template <class A, class X, class P = nidentity>
    requires nenumerable<A&&>
bool ncontains(A&& sequence, const X& value, P projection = {}) {
    nfor(element, forward<A>(sequence))
        if (invoke(projection, element) == value)
            return true;
    return false;
}

template <class A, class X, class P = nidentity>
    requires nenumerable<A&&>
int ncount(A&& sequence, const X& value, P projection = {}) {
    int result = 0;
    nfor(element, forward<A>(sequence))
        result += invoke(projection, element) == value;
    return result;
}

template <class A, class F, class P = nidentity>
    requires nenumerable<A&&>
int ncount_if(A&& sequence, F predicate, P projection = {}) {
    int result = 0;
    nfor(element, forward<A>(sequence))
        result += bool(invoke(predicate, invoke(projection, element)));
    return result;
}

template <class A, class F, class P = nidentity>
    requires nenumerable<A&&>
bool nall_of(A&& sequence, F predicate, P projection = {}) {
    nfor(element, forward<A>(sequence))
        if (!invoke(predicate, invoke(projection, element)))
            return false;
    return true;
}

template <class A, class F, class P = nidentity>
    requires nenumerable<A&&>
bool nany_of(A&& sequence, F predicate, P projection = {}) {
    nfor(element, forward<A>(sequence))
        if (invoke(predicate, invoke(projection, element)))
            return true;
    return false;
}

template <class A, class F, class P = nidentity>
    requires nenumerable<A&&>
bool nnone_of(A&& sequence, F predicate, P projection = {}) {
    return !nany_of(forward<A>(sequence), move(predicate), move(projection));
}

template <class A, class B, class E = nequal<>,
          class PA = nidentity, class PB = nidentity>
    requires nenumerable<A&&> && nenumerable<B&&>
bool nsame(A&& left, B&& right, E equal = {},
           PA left_projection = {}, PB right_projection = {}) {
    auto left_cursor = nenumerate(forward<A>(left));
    auto right_cursor = nenumerate(forward<B>(right));
    while (left_cursor.ok() && right_cursor.ok()) {
        if (!invoke(equal, invoke(left_projection, left_cursor.val()),
                    invoke(right_projection, right_cursor.val())))
            return false;
        left_cursor.next();
        right_cursor.next();
    }
    return !left_cursor.ok() && !right_cursor.ok();
}

template <class A, class C = nless<>, class P = nidentity>
    requires nindexed<A> && invocable<P&, nindex_reference_t<const A>>
int nargmin(const A& sequence, C compare = {}, P projection = {}) {
    int size = nlen(sequence);
    if (size == 0)
        return npos;
    int best = 0;
    for (int i = 1; i < size; ++i)
        if (invoke(compare, invoke(projection, sequence[i]),
                   invoke(projection, sequence[best])))
            best = i;
    return best;
}

template <class A, class C = nless<>, class P = nidentity>
    requires nindexed<A> && invocable<P&, nindex_reference_t<const A>>
int nargmax(const A& sequence, C compare = {}, P projection = {}) {
    int size = nlen(sequence);
    if (size == 0)
        return npos;
    int best = 0;
    for (int i = 1; i < size; ++i)
        if (invoke(compare, invoke(projection, sequence[best]),
                   invoke(projection, sequence[i])))
            best = i;
    return best;
}

template <class A, class C = nless<>, class P = nidentity>
    requires nviewable_indexed<A&&> && nswappable_indexed<remove_reference_t<A>>
void nsort(A&& a, C compare = {}, P projection = {}) {
    int n = nlen(a);
    if (n < 2)
        return;
    ni::nprojected_compare projected(move(compare), move(projection));
    if constexpr (ncontiguous_indexed<remove_reference_t<A>>)
        sort(a.data(), a.data() + n, move(projected));
    else
        ni::nheap_sort(a, projected);
}

template <class A>
    requires nviewable_indexed<A&&> && nswappable_indexed<remove_reference_t<A>>
void nreverse_inplace(A&& a, int l = 0, int r = npos) {
    if (r == npos)
        r = nlen(a);
    npre(0 <= l && l <= r && r <= nlen(a));
    while (l < --r)
        ranges::swap(a[l++], a[r]);
}

template <class A, class X, class P>
    requires nindexed<A> && invocable<P&, nindex_reference_t<const A>>
int nfind(const A& a, const X& value, P projection, int fallback = npos) {
    for (int i = 0; i < nlen(a); ++i)
        if (invoke(projection, a[i]) == value)
            return i;
    return fallback;
}

template <class A, class X>
    requires nindexed<A>
int nfind(const A& a, const X& value, int fallback = npos) {
    return nfind(a, value, nidentity{}, fallback);
}

template <class A, class X, class C = nless<>, class P = nidentity>
    requires nindexed<A> && invocable<P&, nindex_reference_t<const A>>
int nlower(const A& a, const X& value, C compare = {}, P projection = {}) {
    int l = 0, r = nlen(a);
    while (l < r) {
        int m = l + (r - l) / 2;
        invoke(compare, invoke(projection, a[m]), value) ? l = m + 1 : r = m;
    }
    return l;
}

template <class A, class X, class C = nless<>, class P = nidentity>
    requires nindexed<A> && invocable<P&, nindex_reference_t<const A>>
int nupper(const A& a, const X& value, C compare = {}, P projection = {}) {
    int l = 0, r = nlen(a);
    while (l < r) {
        int m = l + (r - l) / 2;
        invoke(compare, value, invoke(projection, a[m])) ? r = m : l = m + 1;
    }
    return l;
}

template <class A, class X, class C = nless<>, class P = nidentity>
    requires nindexed<A> && invocable<P&, nindex_reference_t<const A>>
int nfind_sorted(const A& a, const X& value, C compare = {}, P projection = {},
                 int fallback = npos) {
    int index = nlower(a, value, compare, projection);
    if (index == nlen(a))
        return fallback;
    decltype(auto) projected = invoke(projection, a[index]);
    return !invoke(compare, value, projected) && !invoke(compare, projected, value)
               ? index
               : fallback;
}

template <class A, class X, class C>
    requires nindexed<A>
int nfind_sorted(const A& a, const X& value, C compare, int fallback) {
    return nfind_sorted(a, value, move(compare), nidentity{}, fallback);
}

template <class A, class P = nidentity,
          class O = nadd<ni::nprojected_value_t<const A, P>>>
    requires nindexed<A> &&
             nmonoid<O, ni::nprojected_value_t<const A, P>>
auto nfold(const A& a, int l, int r, O op = {}, P projection = {}) {
    using T = ni::nprojected_value_t<const A, P>;
    npre(0 <= l && l <= r && r <= nlen(a));
    T result = op.id();
    for (int i = l; i < r; ++i)
        result = op(move(result), invoke(projection, a[i]));
    return result;
}

template <class A, class P = nidentity,
          class O = nadd<ni::nprojected_value_t<const A, P>>>
    requires nindexed<A> &&
             nmonoid<O, ni::nprojected_value_t<const A, P>>
auto nfold(const A& a, O op = {}, P projection = {}) {
    return nfold(a, 0, nlen(a), move(op), move(projection));
}

template <class A, class E = nequal<>, class P = nidentity>
    requires nviewable_indexed<A&&> && nreference_indexed<remove_reference_t<A>> &&
             (!is_const_v<remove_reference_t<nindex_reference_t<remove_reference_t<A>>>>)
int nunique_compact(A&& a, E equal = {}, P projection = {}) {
    if (nlen(a) == 0)
        return 0;
    int kept = 1;
    for (int i = 1; i < nlen(a); ++i)
        if (!invoke(equal, invoke(projection, a[kept - 1]), invoke(projection, a[i]))) {
            if (kept != i)
                a[kept] = move(a[i]);
            ++kept;
        }
    return kept;
}

template <class A, class E = nequal<>, class P = nidentity>
    requires nviewable_indexed<A&&> && nresizable<remove_reference_t<A>> &&
             nreference_indexed<remove_reference_t<A>> &&
             (!is_const_v<remove_reference_t<nindex_reference_t<remove_reference_t<A>>>>)
int nunique(A&& a, E equal = {}, P projection = {}) {
    int kept = nunique_compact(a, move(equal), move(projection));
    a.resize(kept);
    return kept;
}

template <class A, class C = nless<>, class E = nequal<>, class P = nidentity>
    requires nviewable_indexed<A&&> && nresizable<remove_reference_t<A>> &&
             nswappable_indexed<remove_reference_t<A>>
int nsort_unique(A&& a, C compare = {}, E equal = {}, P projection = {}) {
    nsort(a, compare, projection);
    return nunique(a, move(equal), move(projection));
}

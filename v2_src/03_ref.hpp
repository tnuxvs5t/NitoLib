namespace ni {
template <class T> class ncontiguous_access {
    T* data_ = nullptr;

  public:
    constexpr ncontiguous_access() = default;
    constexpr explicit ncontiguous_access(T* data) : data_(data) {}
    constexpr T& operator()(int index) const noexcept { return data_[index]; }
    constexpr T* data() const noexcept { return data_; }
};

constexpr int nview_volume(int rows, int columns) {
    npre(rows >= 0 && columns >= 0);
    npre(rows == 0 || columns <= INT_MAX / rows);
    return rows * columns;
}

template <class T> class ncontiguous2_access {
    T* data_ = nullptr;
    int rows_ = 0, columns_ = 0;

  public:
    constexpr ncontiguous2_access() = default;
    constexpr ncontiguous2_access(T* data, int rows, int columns)
        : data_(data), rows_(rows), columns_(columns) {}
    constexpr T& operator()(int index) const noexcept { return data_[index]; }
    constexpr T& operator()(int row, int column) const {
        npre(0 <= row && row < rows_ && 0 <= column && column < columns_);
        return data_[row * columns_ + column];
    }
    constexpr T* data() const noexcept { return data_; }
    constexpr T* row_data(int row) const {
        npre(0 <= row && row < rows_);
        return columns_ ? data_ + ptrdiff_t(row) * columns_ : data_;
    }
    constexpr int rows() const noexcept { return rows_; }
    constexpr int cols() const noexcept { return columns_; }
};

template <class T> class nstrided2_access {
    T* data_ = nullptr;
    int rows_ = 0, columns_ = 0;
    ptrdiff_t row_stride_ = 0, column_stride_ = 0;

  public:
    constexpr nstrided2_access() = default;
    constexpr nstrided2_access(T* data, int rows, int columns,
                               ptrdiff_t row_stride, ptrdiff_t column_stride)
        : data_(data), rows_(rows), columns_(columns),
          row_stride_(row_stride), column_stride_(column_stride) {}
    constexpr T& operator()(int index) const {
        return (*this)(index / columns_, index % columns_);
    }
    constexpr T& operator()(int row, int column) const {
        npre(0 <= row && row < rows_ && 0 <= column && column < columns_);
        return data_[ptrdiff_t(row) * row_stride_ + ptrdiff_t(column) * column_stride_];
    }
    constexpr int rows() const noexcept { return rows_; }
    constexpr int cols() const noexcept { return columns_; }
};

template <class F> class nindexed2_access {
    int rows_ = 0, columns_ = 0;
    [[no_unique_address]] F access_;

  public:
    constexpr nindexed2_access(int rows, int columns, F access)
        : rows_(rows), columns_(columns), access_(move(access)) {}
    constexpr decltype(auto) operator()(int index) {
        return (*this)(index / columns_, index % columns_);
    }
    constexpr decltype(auto) operator()(int index) const
        requires invocable<const F&, int, int>
    {
        return (*this)(index / columns_, index % columns_);
    }
    constexpr decltype(auto) operator()(int row, int column) {
        npre(0 <= row && row < rows_ && 0 <= column && column < columns_);
        return invoke(access_, row, column);
    }
    constexpr decltype(auto) operator()(int row, int column) const
        requires invocable<const F&, int, int>
    {
        npre(0 <= row && row < rows_ && 0 <= column && column < columns_);
        return invoke(access_, row, column);
    }
    constexpr int rows() const noexcept { return rows_; }
    constexpr int cols() const noexcept { return columns_; }
};
} // namespace ni

// One public view family. The model/accessor determines capabilities: the default
// model is contiguous, while lambda and composed models may expose only indexing.
// Copying an nview copies the access description and never materializes elements.
template <class T, class F = ni::ncontiguous_access<T>> class nview {
    int size_ = 0;
    [[no_unique_address]] F access_;

  public:
    using element_type = T;
    using value_type = remove_cv_t<T>;
    using reference = invoke_result_t<F&, int>;
    using accessor_type = F;
    using nview_tag = void;
    using nrange_tag = void;

    constexpr nview()
        requires default_initializable<F>
    = default;

    constexpr nview(T* data, int size)
        requires same_as<F, ni::ncontiguous_access<T>>
        : size_(size), access_(data) {
        npre(size >= 0);
        npre(size == 0 || data != nullptr);
    }

    constexpr nview(T* data, int rows, int columns)
        requires same_as<F, ni::ncontiguous2_access<T>>
        : size_(ni::nview_volume(rows, columns)), access_(data, rows, columns) {
        npre(size_ == 0 || data != nullptr);
    }

    constexpr nview(T* data, int rows, int columns,
                    ptrdiff_t row_stride, ptrdiff_t column_stride)
        requires same_as<F, ni::nstrided2_access<T>>
        : size_(ni::nview_volume(rows, columns)),
          access_(data, rows, columns, row_stride, column_stride) {
        npre(size_ == 0 || data != nullptr);
    }

    template <class G>
    constexpr nview(int rows, int columns, G access)
        requires same_as<F, ni::nindexed2_access<G>>
        : size_(ni::nview_volume(rows, columns)),
          access_(rows, columns, move(access)) {}

    template <size_t N>
    constexpr nview(T (&data)[N])
        requires same_as<F, ni::ncontiguous_access<T>>
        : size_(int(N)), access_(data) {
        static_assert(N <= size_t(INT_MAX));
    }

    template <class U>
        requires same_as<F, ni::ncontiguous_access<T>> &&
                 is_convertible_v<U (*)[], T (*)[]>
    constexpr nview(const nview<U>& other) : size_(other.len()), access_(other.data()) {}

    constexpr nview(int size, F access) : size_(size), access_(move(access)) { npre(size >= 0); }

    constexpr int len() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr int rows() const
        requires requires(const F& access) { { access.rows() } -> same_as<int>; }
    {
        return access_.rows();
    }
    constexpr int cols() const
        requires requires(const F& access) { { access.cols() } -> same_as<int>; }
    {
        return access_.cols();
    }
    constexpr int dim(int axis, int fallback = npos) const
        requires requires(const F& access) {
            { access.rows() } -> same_as<int>;
            { access.cols() } -> same_as<int>;
        }
    {
        return axis == 0 ? access_.rows() : axis == 1 ? access_.cols() : fallback;
    }

    constexpr decltype(auto) data()
        requires requires(F& access) { access.data(); }
    {
        return access_.data();
    }
    constexpr decltype(auto) data() const
        requires requires(const F& access) { access.data(); }
    {
        return access_.data();
    }
    constexpr decltype(auto) row_data(int row)
        requires requires(F& access) { access.row_data(row); }
    {
        return access_.row_data(row);
    }
    constexpr decltype(auto) row_data(int row) const
        requires requires(const F& access) { access.row_data(row); }
    {
        return access_.row_data(row);
    }

    constexpr decltype(auto) operator[](int i) {
        npre(0 <= i && i < size_);
        return invoke(access_, i);
    }
    constexpr decltype(auto) operator[](int i) const
        requires invocable<const F&, int>
    {
        npre(0 <= i && i < size_);
        return invoke(access_, i);
    }

    template <integral... I>
        requires(sizeof...(I) > 1 && invocable<F&, I...>)
    constexpr decltype(auto) operator()(I... coordinate) {
        return invoke(access_, coordinate...);
    }
    template <integral... I>
        requires(sizeof...(I) > 1 && invocable<const F&, I...>)
    constexpr decltype(auto) operator()(I... coordinate) const {
        return invoke(access_, coordinate...);
    }

    constexpr auto get(int i)
        requires is_lvalue_reference_v<decltype((*this)[0])>
    {
        using pointer = add_pointer_t<remove_reference_t<decltype((*this)[0])>>;
        return 0 <= i && i < size_ ? addressof((*this)[i]) : pointer{};
    }
    constexpr auto get(int i) const
        requires requires(const nview& self) { self[0]; } &&
                 is_lvalue_reference_v<decltype(declval<const nview&>()[0])>
    {
        using pointer = add_pointer_t<remove_reference_t<decltype(declval<const nview&>()[0])>>;
        return 0 <= i && i < size_ ? addressof((*this)[i]) : pointer{};
    }
};

template <class T> nview(T*, int) -> nview<T>;
template <class T, size_t N> nview(T (&)[N]) -> nview<T>;
template <class T> nview(T*, int, int) -> nview<T, ni::ncontiguous2_access<T>>;
template <class T>
nview(T*, int, int, ptrdiff_t, ptrdiff_t) -> nview<T, ni::nstrided2_access<T>>;
template <class F>
nview(int, int, F)
    -> nview<remove_reference_t<invoke_result_t<F&, int, int>>, ni::nindexed2_access<F>>;
template <class F>
nview(int, F) -> nview<remove_reference_t<invoke_result_t<F&, int>>, F>;

template <class A>
concept nindexed = requires(A& a, const A& ca, int i) {
    { nlen(ca) } -> same_as<int>;
    a[i];
    ca[i];
};

template <class A> using nindex_reference_t = decltype(declval<A&>()[0]);
template <class A> using nindex_value_t = remove_cvref_t<nindex_reference_t<A>>;

template <class A>
concept nreference_indexed = nindexed<A> && is_lvalue_reference_v<nindex_reference_t<A>>;

template <class A>
concept nswappable_indexed = nreference_indexed<A> &&
                             (!is_const_v<remove_reference_t<nindex_reference_t<A>>>) && requires(A& a) {
                                 ranges::swap(a[0], a[0]);
                             };

template <class A>
concept ncontiguous_indexed = nindexed<A> && requires(A& a) {
    { a.data() } -> contiguous_iterator;
};

template <class A>
concept nresizable = requires(A& a, int n) {
    a.resize(n);
};

template <class A>
concept nview_object = requires { typename remove_cvref_t<A>::nview_tag; };

template <class A>
concept nrange_object = requires { typename remove_cvref_t<A>::nrange_tag; };

template <class A>
concept nviewable_indexed = nindexed<remove_reference_t<A>> &&
                            (is_lvalue_reference_v<A> || nrange_object<remove_cvref_t<A>>) &&
                            (!nrange_object<remove_cvref_t<A>> ||
                             constructible_from<remove_cvref_t<A>, A>);

namespace ni {
template <class A> class nindexed_holder {
    using value_type = remove_cvref_t<A>;
    static constexpr bool stores_value =
        nview_object<value_type> || !is_lvalue_reference_v<A>;
    using pointer_type = remove_reference_t<A>*;
    using storage_type = conditional_t<stores_value, value_type, pointer_type>;
    storage_type storage_;

    static constexpr storage_type make(A&& value) {
        if constexpr (stores_value)
            return forward<A>(value);
        else
            return addressof(value);
    }

  public:
    constexpr explicit nindexed_holder(A&& value) : storage_(make(forward<A>(value))) {
        static_assert(stores_value || is_lvalue_reference_v<A>);
        static_assert(is_lvalue_reference_v<A> || nrange_object<value_type>);
    }

    constexpr decltype(auto) get() {
        if constexpr (stores_value)
            return (storage_);
        else
            return (*storage_);
    }
    constexpr decltype(auto) get() const {
        if constexpr (stores_value)
            return as_const(storage_);
        else
            return (*storage_);
    }
};

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nhold_indexed(A&& value) {
    return nindexed_holder<A&&>(forward<A>(value));
}

template <class H> class nsub_access {
    H owner_;
    int offset_ = 0;

  public:
    constexpr nsub_access(H owner, int offset) : owner_(move(owner)), offset_(offset) {}

    constexpr decltype(auto) operator()(int index) { return owner_.get()[offset_ + index]; }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& owner) { owner.get()[0]; }
    {
        return owner_.get()[offset_ + index];
    }

    constexpr decltype(auto) data()
        requires requires(H& owner) { owner.get().data(); }
    {
        auto pointer = owner_.get().data();
        return offset_ ? pointer + offset_ : pointer;
    }
    constexpr decltype(auto) data() const
        requires requires(const H& owner) { owner.get().data(); }
    {
        auto pointer = owner_.get().data();
        return offset_ ? pointer + offset_ : pointer;
    }
};

template <class H> class nstride_access {
    H owner_;
    int first_ = 0, step_ = 1;

  public:
    constexpr nstride_access(H owner, int first, int step)
        : owner_(move(owner)), first_(first), step_(step) {}

    constexpr decltype(auto) operator()(int index) {
        return owner_.get()[int(first_ + 1LL * index * step_)];
    }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& owner) { owner.get()[0]; }
    {
        return owner_.get()[int(first_ + 1LL * index * step_)];
    }
};

template <class H, bool Row> class naxis2_access {
    H owner_;
    int fixed_ = 0;

  public:
    constexpr naxis2_access(H owner, int fixed) : owner_(move(owner)), fixed_(fixed) {}

    constexpr decltype(auto) operator()(int index) {
        if constexpr (Row)
            return owner_.get()(fixed_, index);
        else
            return owner_.get()(index, fixed_);
    }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& owner) { owner.get()(0, 0); }
    {
        if constexpr (Row)
            return owner_.get()(fixed_, index);
        else
            return owner_.get()(index, fixed_);
    }
    constexpr decltype(auto) data()
        requires Row && requires(H& owner) {
            owner.get().row_data(0);
        }
    {
        return owner_.get().row_data(fixed_);
    }
    constexpr decltype(auto) data() const
        requires Row && requires(const H& owner) {
            owner.get().row_data(0);
        }
    {
        return owner_.get().row_data(fixed_);
    }
};

template <class H> class ndiagonal2_access {
    H owner_;
    int first_row_ = 0, first_column_ = 0;

  public:
    constexpr ndiagonal2_access(H owner, int first_row, int first_column)
        : owner_(move(owner)), first_row_(first_row), first_column_(first_column) {}
    constexpr decltype(auto) operator()(int index) {
        return owner_.get()(first_row_ + index, first_column_ + index);
    }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& owner) { owner.get()(0, 0); }
    {
        return owner_.get()(first_row_ + index, first_column_ + index);
    }
};
} // namespace ni

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nall(A&& a) {
    if constexpr (nview_object<remove_cvref_t<A>>) {
        return remove_cvref_t<A>(forward<A>(a));
    } else {
        auto owner = ni::nhold_indexed(forward<A>(a));
        int size = nlen(owner.get());
        using access_type = ni::nsub_access<decltype(owner)>;
        return nview(size, access_type(move(owner), 0));
    }
}

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nsub(A&& a, int l, int r) {
    auto owner = ni::nhold_indexed(forward<A>(a));
    npre(0 <= l && l <= r && r <= nlen(owner.get()));
    using access_type = ni::nsub_access<decltype(owner)>;
    return nview(r - l, access_type(move(owner), l));
}

template <class A>
    requires nviewable_indexed<A&&>
constexpr auto nstride(A&& a, int first, int last, int step) {
    auto owner = ni::nhold_indexed(forward<A>(a));
    int size = nlen(owner.get());
    npre(step != 0);

    long long distance = 0, stride = step;
    if (step > 0) {
        npre(0 <= first && first <= last && last <= size);
        distance = 1LL * last - first;
    } else {
        if (first == last)
            npre(-1 <= first && first <= size);
        else
            npre(-1 <= last && last < first && first < size);
        distance = 1LL * first - last;
        stride = -stride;
    }
    long long count = distance / stride + (distance % stride != 0);
    npre(count <= INT_MAX);
    using access_type = ni::nstride_access<decltype(owner)>;
    return nview(int(count), access_type(move(owner), first, step));
}

template <class A>
    requires nviewable_indexed<A&&> && requires(remove_reference_t<A>& matrix) {
        { matrix.rows() } -> same_as<int>;
        { matrix.cols() } -> same_as<int>;
        matrix(0, 0);
    }
constexpr auto nrow(A&& matrix, int row) {
    auto owner = ni::nhold_indexed(forward<A>(matrix));
    npre(0 <= row && row < owner.get().rows());
    int size = owner.get().cols();
    using access_type = ni::naxis2_access<decltype(owner), true>;
    return nview(size, access_type(move(owner), row));
}

template <class A>
    requires nviewable_indexed<A&&> && requires(remove_reference_t<A>& matrix) {
        { matrix.rows() } -> same_as<int>;
        { matrix.cols() } -> same_as<int>;
        matrix(0, 0);
    }
constexpr auto ncolumn(A&& matrix, int column) {
    auto owner = ni::nhold_indexed(forward<A>(matrix));
    npre(0 <= column && column < owner.get().cols());
    int size = owner.get().rows();
    using access_type = ni::naxis2_access<decltype(owner), false>;
    return nview(size, access_type(move(owner), column));
}

template <class A>
    requires nviewable_indexed<A&&> && requires(remove_reference_t<A>& matrix) {
        { matrix.rows() } -> same_as<int>;
        { matrix.cols() } -> same_as<int>;
        matrix(0, 0);
    }
constexpr auto ndiagonal(A&& matrix, int offset = 0) {
    auto owner = ni::nhold_indexed(forward<A>(matrix));
    int rows = owner.get().rows(), columns = owner.get().cols();
    npre(-1LL * rows <= offset && offset <= columns);
    int first_row = int(max(0LL, -1LL * offset)), first_column = max(0, offset);
    int size = min(rows - first_row, columns - first_column);
    npre(size >= 0);
    using access_type = ni::ndiagonal2_access<decltype(owner)>;
    return nview(size, access_type(move(owner), first_row, first_column));
}

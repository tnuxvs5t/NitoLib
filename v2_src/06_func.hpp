namespace ni {
template <class A> class ndomain_holder {
    using value_type = remove_cvref_t<A>;
    static constexpr bool owns = !is_lvalue_reference_v<A>;
    using storage_type = conditional_t<owns, value_type, remove_reference_t<A>*>;
    storage_type storage_;

    static constexpr storage_type make(A&& domain) {
        if constexpr (owns)
            return forward<A>(domain);
        else
            return addressof(domain);
    }

  public:
    constexpr explicit ndomain_holder(A&& domain) : storage_(make(forward<A>(domain))) {}
    constexpr decltype(auto) get() {
        if constexpr (owns)
            return (storage_);
        else
            return (*storage_);
    }
    constexpr decltype(auto) get() const {
        if constexpr (owns)
            return as_const(storage_);
        else
            return (*storage_);
    }
};
} // namespace ni

template <class H, class F> class ndiscrete_function {
    H domain_;
    [[no_unique_address]] F evaluate_;

  public:
    using nview_tag = void;

    constexpr ndiscrete_function(H domain, F evaluate)
        : domain_(move(domain)), evaluate_(move(evaluate)) {}

    constexpr int len() const { return nlen(domain_.get()); }
    constexpr bool empty() const { return len() == 0; }

    constexpr decltype(auto) key(int index) {
        npre(0 <= index && index < len());
        return domain_.get()[index];
    }
    constexpr decltype(auto) key(int index) const {
        npre(0 <= index && index < len());
        return domain_.get()[index];
    }

    constexpr decltype(auto) operator[](int index) {
        return invoke(evaluate_, key(index));
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const F& evaluate, const H& domain) {
            invoke(evaluate, domain.get()[0]);
        }
    {
        return invoke(evaluate_, key(index));
    }

    template <class X> constexpr decltype(auto) operator()(X&& value) {
        return invoke(evaluate_, forward<X>(value));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& value) const
        requires invocable<const F&, X&&>
    {
        return invoke(evaluate_, forward<X>(value));
    }

    constexpr auto keys() const
        requires copy_constructible<H>
    {
        int size = len();
        return nview(size, [domain = domain_](int index) -> decltype(auto) {
            return domain.get()[index];
        });
    }
};

template <class D, class F>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> || constructible_from<remove_cvref_t<D>, D&&>)
constexpr auto nfunc(D&& domain, F evaluate) {
    auto holder = ni::ndomain_holder<D&&>(forward<D>(domain));
    return ndiscrete_function<decltype(holder), F>(move(holder), move(evaluate));
}

template <class G, class D>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> || constructible_from<remove_cvref_t<D>, D&&>)
constexpr auto nrestrict(G function, D&& domain) {
    return nfunc(forward<D>(domain),
                 [function = move(function)](auto&& value) -> decltype(auto) {
                     return invoke(function, forward<decltype(value)>(value));
                 });
}

template <class Outer, class H, class Inner>
    requires copy_constructible<H>
constexpr auto ncompose(Outer outer, ndiscrete_function<H, Inner> inner) {
    auto domain = inner.keys();
    return nfunc(move(domain),
                 [outer = move(outer), inner = move(inner)](auto&& value) -> decltype(auto) {
                     return invoke(outer, invoke(inner, forward<decltype(value)>(value)));
                 });
}

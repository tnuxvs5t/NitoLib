namespace ni {
// A zero-allocation lifetime bridge used by discrete-function adaptors:
// lvalues are borrowed, rvalues are owned.
template <class A> class nobject_holder {
    using value_type = remove_cvref_t<A>;
    static constexpr bool owns = !is_lvalue_reference_v<A>;
    using storage_type = conditional_t<owns, value_type, remove_reference_t<A>*>;
    storage_type storage_;

    static constexpr storage_type make(A&& value) {
        if constexpr (owns)
            return forward<A>(value);
        else
            return addressof(value);
    }

  public:
    constexpr explicit nobject_holder(A&& value) : storage_(make(forward<A>(value))) {}

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

template <class A> constexpr auto nhold_object(A&& value) {
    return nobject_holder<A&&>(forward<A>(value));
}

template <class A> using ndomain_holder = nobject_holder<A>;
} // namespace ni

// A finite discrete function is a keyed view, not an associative container.
// key(i) moves from enumeration position to semantic argument; operator[](i)
// evaluates at that position; operator()(x) evaluates an arbitrary argument.
template <class H, class F> class ndiscrete_function {
    H domain_;
    [[no_unique_address]] F evaluate_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

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

    template <class X> constexpr decltype(auto) operator()(X&& argument) {
        return invoke(evaluate_, forward<X>(argument));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires invocable<const F&, X&&>
    {
        return invoke(evaluate_, forward<X>(argument));
    }

    // Compatibility convenience. nkeys(f) below is the zero-copy generic form;
    // this member deliberately copies the small domain holder so the returned
    // view does not point at the ndiscrete_function object itself.
    constexpr auto keys() const
        requires copy_constructible<H>
    {
        int size = len();
        return nview(size, [domain = domain_](int index) -> decltype(auto) {
            return domain.get()[index];
        });
    }
};

template <class A>
concept ndiscrete = nindexed<A> && requires(A& function, const A& constant, int index) {
    function.key(index);
    constant.key(index);
};

template <class A> using nfunction_key_reference_t = decltype(declval<A&>().key(0));
template <class A> using nfunction_key_t = remove_cvref_t<nfunction_key_reference_t<A>>;

template <class D, class F>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> || constructible_from<remove_cvref_t<D>, D&&>)
constexpr auto nfunc(D&& domain, F evaluate) {
    auto holder = ni::nhold_object(forward<D>(domain));
    return ndiscrete_function<decltype(holder), F>(move(holder), move(evaluate));
}

namespace ni {
template <class H> class nfunction_key_access {
    H function_;

  public:
    constexpr explicit nfunction_key_access(H function) : function_(move(function)) {}
    constexpr decltype(auto) operator()(int index) { return function_.get().key(index); }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& function) { function.get().key(0); }
    {
        return function_.get().key(index);
    }
};

template <class H> class nfunction_entry_access {
    H function_;

    template <class G> static constexpr auto entry(G& function, int index) {
        using key_reference = decltype(function.key(index));
        using value_reference = decltype(function[index]);
        return pair<key_reference, value_reference>(function.key(index), function[index]);
    }

  public:
    constexpr explicit nfunction_entry_access(H function) : function_(move(function)) {}
    constexpr auto operator()(int index) { return entry(function_.get(), index); }
    constexpr auto operator()(int index) const
        requires requires(const H& function) {
            function.get().key(0);
            function.get()[0];
        }
    {
        return entry(function_.get(), index);
    }
};
} // namespace ni

template <class G>
    requires ndiscrete<remove_reference_t<G>>
constexpr auto nkeys(G&& function) {
    auto holder = ni::nhold_object(forward<G>(function));
    int size = nlen(holder.get());
    using access_type = ni::nfunction_key_access<decltype(holder)>;
    return nview(size, access_type(move(holder)));
}

template <class G>
    requires ndiscrete<remove_reference_t<G>> && nviewable_indexed<G&&>
constexpr auto nvalues(G&& function) {
    return nall(forward<G>(function));
}

template <class G>
    requires ndiscrete<remove_reference_t<G>>
constexpr auto nentries(G&& function) {
    auto holder = ni::nhold_object(forward<G>(function));
    int size = nlen(holder.get());
    using access_type = ni::nfunction_entry_access<decltype(holder)>;
    return nview(size, access_type(move(holder)));
}

// Semantic restriction: domain contains arguments, not positions.
template <class G, class D>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> || constructible_from<remove_cvref_t<D>, D&&>)
constexpr auto nrestrict(G&& function, D&& domain) {
    auto owner = ni::nhold_object(forward<G>(function));
    return nfunc(forward<D>(domain),
                 [owner = move(owner)](auto&& argument) -> decltype(auto) {
                     return invoke(owner.get(), forward<decltype(argument)>(argument));
                 });
}

template <class OH, class IH> class ncomposed_function {
    [[no_unique_address]] OH outer_;
    IH inner_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    constexpr ncomposed_function(OH outer, IH inner)
        : outer_(move(outer)), inner_(move(inner)) {}
    constexpr int len() const { return nlen(inner_.get()); }
    constexpr bool empty() const { return len() == 0; }
    constexpr decltype(auto) key(int index) { return inner_.get().key(index); }
    constexpr decltype(auto) key(int index) const { return inner_.get().key(index); }

    constexpr decltype(auto) operator[](int index) {
        decltype(auto) middle = inner_.get()[index];
        return invoke(outer_.get(), forward<decltype(middle)>(middle));
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const OH& outer, const IH& inner) {
            invoke(outer.get(), inner.get()[0]);
        }
    {
        decltype(auto) middle = inner_.get()[index];
        return invoke(outer_.get(), forward<decltype(middle)>(middle));
    }

    template <class X> constexpr decltype(auto) operator()(X&& argument) {
        decltype(auto) middle = invoke(inner_.get(), forward<X>(argument));
        return invoke(outer_.get(), forward<decltype(middle)>(middle));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires requires(const OH& outer, const IH& inner, X&& value) {
            invoke(outer.get(), invoke(inner.get(), forward<X>(value)));
        }
    {
        decltype(auto) middle = invoke(inner_.get(), forward<X>(argument));
        return invoke(outer_.get(), forward<decltype(middle)>(middle));
    }
};

template <class Outer, class Inner>
    requires ndiscrete<remove_reference_t<Inner>>
constexpr auto ncompose(Outer&& outer, Inner&& inner) {
    auto outer_holder = ni::nhold_object(forward<Outer>(outer));
    auto inner_holder = ni::nhold_object(forward<Inner>(inner));
    return ncomposed_function<decltype(outer_holder), decltype(inner_holder)>(
        move(outer_holder), move(inner_holder));
}

template <class G, class F>
    requires ndiscrete<remove_reference_t<G>>
constexpr auto nmap_values(G&& function, F&& transform) {
    return ncompose(forward<F>(transform), forward<G>(function));
}

template <class GH, class PH> class ngathered_function {
    GH function_;
    PH positions_;

    constexpr int source_position(int index) const {
        npre(0 <= index && index < len());
        int position = ni::nchecked_int(positions_.get()[index]);
        npre(0 <= position && position < nlen(function_.get()));
        return position;
    }

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    constexpr ngathered_function(GH function, PH positions)
        : function_(move(function)), positions_(move(positions)) {}
    constexpr int len() const { return nlen(positions_.get()); }
    constexpr bool empty() const { return len() == 0; }
    constexpr int position(int index) const { return source_position(index); }
    constexpr decltype(auto) key(int index) {
        return function_.get().key(source_position(index));
    }
    constexpr decltype(auto) key(int index) const {
        return function_.get().key(source_position(index));
    }
    constexpr decltype(auto) operator[](int index) {
        return function_.get()[source_position(index)];
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const GH& function) { function.get()[0]; }
    {
        return function_.get()[source_position(index)];
    }
    template <class X> constexpr decltype(auto) operator()(X&& argument) {
        return invoke(function_.get(), forward<X>(argument));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires requires(const GH& function, X&& value) {
            invoke(function.get(), forward<X>(value));
        }
    {
        return invoke(function_.get(), forward<X>(argument));
    }
};

// Positional selection: positions index the source enumeration, while the result
// keeps the source semantic keys. Repeated positions deliberately keep aliases.
template <class G, class P>
    requires ndiscrete<remove_reference_t<G>> && nindexed<remove_reference_t<P>> &&
             integral<nindex_value_t<remove_reference_t<P>>> &&
             (is_lvalue_reference_v<P&&> || constructible_from<remove_cvref_t<P>, P&&>)
constexpr auto ngather(G&& function, P&& positions) {
    auto function_holder = ni::nhold_object(forward<G>(function));
    auto position_holder = ni::nhold_object(forward<P>(positions));
    return ngathered_function<decltype(function_holder), decltype(position_holder)>(
        move(function_holder), move(position_holder));
}

template <class G>
    requires ndiscrete<remove_reference_t<G>>
constexpr auto nsubfunc(G&& function, int left, int right) {
    npre(0 <= left && left <= right && right <= nlen(function));
    return ngather(forward<G>(function), nrange(left, right));
}

template <class G>
    requires ndiscrete<remove_reference_t<G>>
constexpr auto nblock(G&& function, int block, int width) {
    npre(block >= 0 && width > 0);
    long long left = 1LL * block * width;
    npre(left <= nlen(function));
    int first = int(left);
    int last = int(min<long long>(nlen(function), left + width));
    return nsubfunc(forward<G>(function), first, last);
}

namespace ni {
template <class H> class nfunction_block_access {
    H function_;
    int width_;

  public:
    constexpr nfunction_block_access(H function, int width)
        : function_(move(function)), width_(width) {}
    constexpr auto operator()(int block) {
        return nblock(function_.get(), block, width_);
    }
    constexpr auto operator()(int block) const
        requires requires(const H& function) { nblock(function.get(), 0, 1); }
    {
        return nblock(function_.get(), block, width_);
    }
};
} // namespace ni

template <class G>
    requires ndiscrete<remove_reference_t<G>>
constexpr auto nblocks(G&& function, int width) {
    auto holder = ni::nhold_object(forward<G>(function));
    npre(width > 0);
    int size = nlen(holder.get());
    int count = size / width + (size % width != 0);
    using access_type = ni::nfunction_block_access<decltype(holder)>;
    return nview(count, access_type(move(holder), width));
}

namespace ni {
template <class D, class K>
concept ndirect_domain_locator = requires(const D& domain, const K& key) {
    { domain.position(key) } -> convertible_to<int>;
};

template <class DH, class VH> class nbound_function {
    using domain_type = remove_reference_t<decltype(declval<DH&>().get())>;
    using key_type = nindex_value_t<domain_type>;
    static constexpr bool direct = ndirect_domain_locator<domain_type, key_type>;
    using index_type = conditional_t<direct, monostate, nmap<key_type, int>>;

    DH domain_;
    VH values_;
    [[no_unique_address]] index_type position_;

    int locate(const key_type& key) const {
        if constexpr (direct) {
            int position = domain_.get().position(key);
            npre(position != npos);
            return position;
        } else {
            const int* position = position_.get(key);
            npre(position != nullptr);
            return *position;
        }
    }

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    nbound_function(DH domain, VH values)
        : domain_(move(domain)), values_(move(values)) {
        npre(nlen(domain_.get()) == nlen(values_.get()));
        if constexpr (!direct) {
            position_.reserve(len());
            for (int index = 0; index < len(); ++index)
                npre(position_.ins(key_type(domain_.get()[index]), index));
        }
    }

    int len() const { return nlen(domain_.get()); }
    bool empty() const { return len() == 0; }

    decltype(auto) key(int index) {
        npre(0 <= index && index < len());
        return as_const(domain_.get())[index];
    }
    decltype(auto) key(int index) const {
        npre(0 <= index && index < len());
        return domain_.get()[index];
    }
    decltype(auto) operator[](int index) {
        npre(0 <= index && index < len());
        npre(nlen(values_.get()) == len());
        return values_.get()[index];
    }
    decltype(auto) operator[](int index) const {
        npre(0 <= index && index < len());
        npre(nlen(values_.get()) == len());
        return values_.get()[index];
    }
    decltype(auto) operator()(const key_type& argument) {
        return values_.get()[locate(argument)];
    }
    decltype(auto) operator()(const key_type& argument) const {
        return values_.get()[locate(argument)];
    }
};
} // namespace ni

// Strict ordinal binding. Unlike nzip, a finite function never truncates a side:
// the domain and value enumeration must have exactly the same length.
template <class D, class V>
    requires nindexed<remove_reference_t<D>> && nindexed<remove_reference_t<V>> &&
             (is_lvalue_reference_v<V&&> || constructible_from<remove_cvref_t<V>, V&&>)
auto nfunc(D&& domain, V&& values) {
    auto value_holder = ni::nhold_object(forward<V>(values));
    using domain_type = remove_reference_t<D>;
    using key_type = nindex_value_t<domain_type>;
    if constexpr (ni::ndirect_domain_locator<domain_type, key_type> &&
                  constructible_from<remove_cvref_t<D>, D&&>) {
        auto domain_holder = ni::nhold_object(remove_cvref_t<D>(forward<D>(domain)));
        return ni::nbound_function<decltype(domain_holder), decltype(value_holder)>(
            move(domain_holder), move(value_holder));
    } else {
        auto keys = ncollect<key_type>(forward<D>(domain));
        auto domain_holder = ni::nhold_object(move(keys));
        return ni::nbound_function<decltype(domain_holder), decltype(value_holder)>(
            move(domain_holder), move(value_holder));
    }
}

// Re-anchor a positional enumeration without semantically evaluating the source.
template <class S, class A>
    requires nindexed<remove_reference_t<S>> && nindexed<remove_reference_t<A>>
auto nanchors(S&& source, A&& anchors) {
    return nfunc(forward<A>(anchors), forward<S>(source));
}

namespace ni {
template <class T> class nconstant_branch {
    T value_;

  public:
    explicit nconstant_branch(T value) : value_(move(value)) {}
    template <class X> T operator()(X&&) const { return value_; }
};

template <class GH, class P, class A> class nbranch_function {
    GH base_;
    [[no_unique_address]] P predicate_;
    [[no_unique_address]] A alternative_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    nbranch_function(GH base, P predicate, A alternative)
        : base_(move(base)), predicate_(move(predicate)), alternative_(move(alternative)) {}

    int len() const { return nlen(base_.get()); }
    bool empty() const { return len() == 0; }
    decltype(auto) key(int index) { return base_.get().key(index); }
    decltype(auto) key(int index) const { return base_.get().key(index); }

    decltype(auto) operator[](int index) {
        decltype(auto) argument = base_.get().key(index);
        return invoke(predicate_, argument) ? invoke(alternative_, argument)
                                            : base_.get()[index];
    }
    decltype(auto) operator[](int index) const
        requires requires(const GH& base, const P& predicate, const A& alternative) {
            invoke(predicate, base.get().key(0));
            invoke(alternative, base.get().key(0));
            base.get()[0];
        }
    {
        decltype(auto) argument = base_.get().key(index);
        return invoke(predicate_, argument) ? invoke(alternative_, argument)
                                            : base_.get()[index];
    }

    template <class X> decltype(auto) operator()(X&& argument) {
        return invoke(predicate_, argument)
                   ? invoke(alternative_, forward<X>(argument))
                   : invoke(base_.get(), forward<X>(argument));
    }
    template <class X>
    decltype(auto) operator()(X&& argument) const
        requires requires(const GH& base, const P& predicate, const A& alternative, X&& value) {
            invoke(predicate, value);
            invoke(alternative, forward<X>(value));
            invoke(base.get(), forward<X>(value));
        }
    {
        return invoke(predicate_, argument)
                   ? invoke(alternative_, forward<X>(argument))
                   : invoke(base_.get(), forward<X>(argument));
    }
};

template <class H> class nshared_function {
    shared_ptr<H> owner_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    explicit nshared_function(shared_ptr<H> owner) : owner_(move(owner)) {}
    int len() const { return nlen(owner_->get()); }
    bool empty() const { return len() == 0; }
    decltype(auto) key(int index) { return owner_->get().key(index); }
    decltype(auto) key(int index) const { return owner_->get().key(index); }
    decltype(auto) operator[](int index) { return owner_->get()[index]; }
    decltype(auto) operator[](int index) const { return owner_->get()[index]; }
    template <class X> decltype(auto) operator()(X&& argument) {
        return invoke(owner_->get(), forward<X>(argument));
    }
    template <class X>
    decltype(auto) operator()(X&& argument) const
        requires requires(const H& owner, X&& value) {
            invoke(owner.get(), forward<X>(value));
        }
    {
        return invoke(owner_->get(), forward<X>(argument));
    }
};

template <class S> struct nrun_state {
    S source;
    nvector<int> starts;
    nmap<int, int> position;
};

template <class S> class nrun_function {
    shared_ptr<nrun_state<S>> state_;

    auto segment(int index) const {
        npre(0 <= index && index < len());
        int left = state_->starts[index];
        int right = index + 1 < len() ? state_->starts[index + 1] : state_->source.len();
        return nsubfunc(S(state_->source), left, right);
    }

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    explicit nrun_function(shared_ptr<nrun_state<S>> state) : state_(move(state)) {}
    int len() const { return state_->starts.len(); }
    bool empty() const { return len() == 0; }
    int key(int index) const {
        npre(0 <= index && index < len());
        return state_->starts[index];
    }
    int key(int index) { return as_const(*this).key(index); }
    auto operator[](int index) { return segment(index); }
    auto operator[](int index) const { return segment(index); }
    auto operator()(int start) {
        const int* index = state_->position.get(start);
        npre(index != nullptr);
        return segment(*index);
    }
    auto operator()(int start) const {
        const int* index = state_->position.get(start);
        npre(index != nullptr);
        return segment(*index);
    }
};
} // namespace ni

template <class G, class P>
    requires ndiscrete<remove_reference_t<G>>
auto nbranch(G&& function, P predicate, auto&& alternative) {
    auto holder = ni::nhold_object(forward<G>(function));
    using key_reference = decltype(holder.get().key(0));
    if constexpr (invocable<remove_reference_t<decltype(alternative)>&, key_reference>) {
        using alternative_type = remove_cvref_t<decltype(alternative)>;
        return ni::nbranch_function<decltype(holder), P, alternative_type>(
            move(holder), move(predicate), forward<decltype(alternative)>(alternative));
    } else {
        using constant_type = remove_cvref_t<decltype(alternative)>;
        using alternative_type = ni::nconstant_branch<constant_type>;
        return ni::nbranch_function<decltype(holder), P, alternative_type>(
            move(holder), move(predicate),
            alternative_type(forward<decltype(alternative)>(alternative)));
    }
}

template <class G, class P = nequal<>>
    requires ndiscrete<remove_reference_t<G>>
auto nruns(G&& function, P together = {}) {
    auto holder = ni::nhold_object(forward<G>(function));
    using holder_type = decltype(holder);
    using source_type = ni::nshared_function<holder_type>;
    source_type source(make_shared<holder_type>(move(holder)));

    nvector<int> starts;
    if (!source.empty()) {
        starts.push(0);
        for (int index = 1; index < source.len(); ++index)
            if (!invoke(together, source[index - 1], source[index]))
                starts.push(index);
    }
    nmap<int, int> position(starts.len());
    for (int index = 0; index < starts.len(); ++index)
        npre(position.ins(starts[index], index));

    using state_type = ni::nrun_state<source_type>;
    auto state = make_shared<state_type>(
        state_type{move(source), move(starts), move(position)});
    return ni::nrun_function<source_type>(move(state));
}

template <class A, class P = nequal<>>
    requires nindexed<remove_reference_t<A>> && (!ndiscrete<remove_reference_t<A>>)
auto nruns(A&& source, P together = {}) {
    int size = nlen(source);
    return nruns(nfunc(nrange(size), forward<A>(source)), move(together));
}

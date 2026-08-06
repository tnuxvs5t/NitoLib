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
    static constexpr bool nstable_borrow_result = true;

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
        return ninvoke_public([&]() -> decltype(auto) {
            return as_const(domain_.get())[index];
        });
    }
    decltype(auto) key(int index) const {
        npre(0 <= index && index < len());
        return ninvoke_public([&]() -> decltype(auto) {
            return domain_.get()[index];
        });
    }
    decltype(auto) operator[](int index) {
        npre(0 <= index && index < len());
        npre(nlen(values_.get()) == len());
        return ninvoke_public([&]() -> decltype(auto) {
            return values_.get()[index];
        });
    }
    decltype(auto) operator[](int index) const {
        npre(0 <= index && index < len());
        npre(nlen(values_.get()) == len());
        return ninvoke_public([&]() -> decltype(auto) {
            return values_.get()[index];
        });
    }
    decltype(auto) operator()(const key_type& argument) {
        return ninvoke_public([&]() -> decltype(auto) {
            return values_.get()[locate(argument)];
        });
    }
    decltype(auto) operator()(const key_type& argument) const {
        return ninvoke_public([&]() -> decltype(auto) {
            return values_.get()[locate(argument)];
        });
    }
};
} // namespace ni

// Strict ordinal binding. Unlike nzip, a finite function never truncates a side:
// the domain and value enumeration must have exactly the same length. The domain
// is snapshotted so its key locator cannot be invalidated externally.
template <class D, class V>
    requires nindexed<remove_reference_t<D>> && nindexed<remove_reference_t<V>> &&
             (is_lvalue_reference_v<V&&> ||
              constructible_from<remove_cvref_t<V>, V&&>)
auto nfunc_bind(D&& domain, V&& values) {
    auto value_holder = ni::nhold_object(forward<V>(values));
    using domain_type = remove_reference_t<D>;
    using key_type = nindex_value_t<domain_type>;
    if constexpr (ni::ndirect_domain_locator<domain_type, key_type> &&
                  constructible_from<remove_cvref_t<D>, D&&>) {
        auto domain_holder =
            ni::nhold_object(remove_cvref_t<D>(forward<D>(domain)));
        return ni::nbound_function<decltype(domain_holder), decltype(value_holder)>(
            move(domain_holder), move(value_holder));
    } else {
        auto keys = ncollect<key_type>(forward<D>(domain));
        auto domain_holder = ni::nhold_object(move(keys));
        return ni::nbound_function<decltype(domain_holder), decltype(value_holder)>(
            move(domain_holder), move(value_holder));
    }
}

template <class D, class V>
    requires nindexed<remove_reference_t<D>> && nindexed<remove_reference_t<V>> &&
             (!invocable<V&, nindex_reference_t<remove_reference_t<D>>>)
[[deprecated("use nfunc_bind")]]
auto nfunc(D&& domain, V&& values) {
    return nfunc_bind(forward<D>(domain), forward<V>(values));
}

template <class D, class X>
    requires nindexed<remove_reference_t<D>> && nindexed<remove_reference_t<X>> &&
             invocable<X&, nindex_reference_t<remove_reference_t<D>>>
auto nfunc(D&&, X&&) {
    static_assert(!same_as<X, X>,
                  "ambiguous nfunc: use nfunc_eval/nfunc_value/nfunc_ref or "
                  "nfunc_bind");
}

// Re-anchor a positional enumeration without semantically evaluating the source.
template <class S, class A>
    requires nindexed<remove_reference_t<S>> && nindexed<remove_reference_t<A>>
auto nanchors(S&& source, A&& anchors) {
    return nfunc_bind(forward<A>(anchors), forward<S>(source));
}

namespace ni {
template <class T> class nconstant_branch {
    T value_;

  public:
    explicit nconstant_branch(T value) : value_(move(value)) {}
    template <class X> const T& operator()(X&&) const { return value_; }
    template <class X> T& operator()(X&&) { return value_; }
};

template <class R>
using nnamed_key_reference_t = add_lvalue_reference_t<remove_reference_t<R>>;

template <class G>
using nbranch_key_reference_t =
    nnamed_key_reference_t<nfunction_key_reference_t<G>>;

template <class G, class A>
concept nbranch_ref_compatible =
    nmutable_discrete_function<G> &&
    invocable<A&, nbranch_key_reference_t<G>> &&
    is_lvalue_reference_v<decltype(declval<G&>()[0])> &&
    same_as<decltype(declval<G&>()[0]),
            invoke_result_t<A&, nbranch_key_reference_t<G>>> &&
    same_as<decltype(declval<G&>()[0]),
            invoke_result_t<G&, nbranch_key_reference_t<G>>>;

template <class GH, class PH, class AH> class nbranch_value_function {
    GH base_;
    [[no_unique_address]] PH predicate_;
    [[no_unique_address]] AH alternative_;

    using key_reference = nnamed_key_reference_t<
        decltype(declval<GH&>().get().key(0))>;
    using const_key_reference = nnamed_key_reference_t<
        decltype(declval<const GH&>().get().key(0))>;
    using base_result = decltype(declval<GH&>().get()[0]);
    using alternative_result =
        invoke_result_t<decltype(declval<AH&>().get()), key_reference>;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;
    using result_type = common_type_t<remove_cvref_t<base_result>,
                                      remove_cvref_t<alternative_result>>;
    static constexpr bool nstable_borrow_result = false;

    nbranch_value_function(GH base, PH predicate, AH alternative)
        : base_(move(base)), predicate_(move(predicate)),
          alternative_(move(alternative)) {}

    int len() const { return nlen(base_.get()); }
    bool empty() const { return len() == 0; }
    decltype(auto) key(int index) {
        return ninvoke_public([&]() -> decltype(auto) {
            return base_.get().key(index);
        });
    }
    decltype(auto) key(int index) const {
        return ninvoke_public([&]() -> decltype(auto) {
            return base_.get().key(index);
        });
    }

    result_type operator[](int index) {
        decltype(auto) argument = base_.get().key(index);
        if (invoke(predicate_.get(), argument))
            return result_type(invoke(alternative_.get(), argument));
        return result_type(base_.get()[index]);
    }
    result_type operator[](int index) const
        requires requires(const GH& base, const PH& predicate,
                          const AH& alternative,
                          const_key_reference argument) {
            invoke(predicate.get(), argument);
            result_type(invoke(alternative.get(), argument));
            result_type(base.get()[0]);
        }
    {
        decltype(auto) argument = base_.get().key(index);
        if (invoke(predicate_.get(), argument))
            return result_type(invoke(alternative_.get(), argument));
        return result_type(base_.get()[index]);
    }

    template <class X>
    result_type operator()(X&& argument)
        requires requires(GH& base, PH& predicate, AH& alternative,
                          nnamed_key_reference_t<X&&> value) {
            invoke(predicate.get(), value);
            result_type(invoke(alternative.get(), value));
            result_type(invoke(base.get(), value));
        }
    {
        if (invoke(predicate_.get(), argument))
            return result_type(invoke(alternative_.get(), argument));
        return result_type(invoke(base_.get(), argument));
    }
    template <class X>
    result_type operator()(X&& argument) const
        requires requires(const GH& base, const PH& predicate,
                          const AH& alternative,
                          nnamed_key_reference_t<X&&> value) {
            invoke(predicate.get(), value);
            result_type(invoke(alternative.get(), value));
            result_type(invoke(base.get(), value));
        }
    {
        if (invoke(predicate_.get(), argument))
            return result_type(invoke(alternative_.get(), argument));
        return result_type(invoke(base_.get(), argument));
    }
};

template <class GH, class PH, class AH> class nbranch_ref_function {
    GH base_;
    [[no_unique_address]] PH predicate_;
    [[no_unique_address]] AH alternative_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;
    using result_type = decltype(declval<GH&>().get()[0]);
    static constexpr bool nstable_borrow_result = true;

    static_assert(is_lvalue_reference_v<result_type>);
    static_assert(same_as<
                  result_type,
                  invoke_result_t<decltype(declval<AH&>().get()),
                                  nnamed_key_reference_t<decltype(
                                      declval<GH&>().get().key(0))>>>);

    nbranch_ref_function(GH base, PH predicate, AH alternative)
        : base_(move(base)), predicate_(move(predicate)),
          alternative_(move(alternative)) {}

    int len() const { return nlen(base_.get()); }
    bool empty() const { return len() == 0; }
    decltype(auto) key(int index) {
        return ninvoke_public([&]() -> decltype(auto) {
            return base_.get().key(index);
        });
    }
    decltype(auto) key(int index) const {
        return ninvoke_public([&]() -> decltype(auto) {
            return base_.get().key(index);
        });
    }

    result_type operator[](int index) {
        decltype(auto) argument = base_.get().key(index);
        if (invoke(predicate_.get(), argument))
            return invoke(alternative_.get(), argument);
        return base_.get()[index];
    }
    decltype(auto) operator[](int index) const
        requires requires(const GH& base, const PH& predicate,
                          const AH& alternative,
                          nnamed_key_reference_t<decltype(
                              declval<const GH&>().get().key(0))> argument) {
            invoke(predicate.get(), argument);
            invoke(alternative.get(), argument);
            base.get()[0];
        } && same_as<decltype(declval<const GH&>().get()[0]),
                     invoke_result_t<decltype(declval<const AH&>().get()),
                                     nnamed_key_reference_t<decltype(
                                         declval<const GH&>().get().key(0))>>> &&
                 is_lvalue_reference_v<decltype(declval<const GH&>().get()[0])>
    {
        decltype(auto) argument = base_.get().key(index);
        if (invoke(predicate_.get(), argument))
            return invoke(alternative_.get(), argument);
        return base_.get()[index];
    }

    template <class X>
    result_type operator()(X&& argument)
        requires requires(GH& base, PH& predicate, AH& alternative,
                          nnamed_key_reference_t<X&&> value) {
            invoke(predicate.get(), value);
            { invoke(base.get(), value) } -> same_as<result_type>;
            { invoke(alternative.get(), value) } -> same_as<result_type>;
        }
    {
        if (invoke(predicate_.get(), argument))
            return invoke(alternative_.get(), argument);
        return invoke(base_.get(), argument);
    }
    template <class X>
    decltype(auto) operator()(X&& argument) const
        requires requires(const GH& base, const PH& predicate,
                          const AH& alternative,
                          nnamed_key_reference_t<X&&> value) {
            invoke(predicate.get(), value);
            invoke(base.get(), value);
            invoke(alternative.get(), value);
        } && same_as<invoke_result_t<decltype(declval<const GH&>().get()),
                                      nnamed_key_reference_t<X&&>>,
                     invoke_result_t<decltype(declval<const AH&>().get()),
                                      nnamed_key_reference_t<X&&>>> &&
                 is_lvalue_reference_v<invoke_result_t<
                     decltype(declval<const GH&>().get()),
                     nnamed_key_reference_t<X&&>>>
    {
        if (invoke(predicate_.get(), argument))
            return invoke(alternative_.get(), argument);
        return invoke(base_.get(), argument);
    }
};

template <class A, class Key> auto nbranch_alternative_holder(A&& alternative) {
    if constexpr (invocable<A&, Key>) {
        return nhold_object(forward<A>(alternative));
    } else {
        using constant_type = remove_cvref_t<A>;
        return nhold_object(
            nconstant_branch<constant_type>(forward<A>(alternative)));
    }
}
} // namespace ni

template <class G, class P, class A>
    requires ni::nmutable_discrete_function<remove_reference_t<G>>
auto nbranch_value(G&& function, P&& predicate, A&& alternative) {
    auto base_holder = ni::nhold_object(forward<G>(function));
    auto predicate_holder = ni::nhold_object(forward<P>(predicate));
    using key_reference = ni::nnamed_key_reference_t<
        decltype(base_holder.get().key(0))>;
    auto alternative_holder =
        ni::nbranch_alternative_holder<A, key_reference>(forward<A>(alternative));
    return ni::nbranch_value_function<decltype(base_holder),
                                      decltype(predicate_holder),
                                      decltype(alternative_holder)>(
        move(base_holder), move(predicate_holder), move(alternative_holder));
}

template <class G, class P, class A>
    requires ni::nbranch_ref_compatible<remove_reference_t<G>,
                                        remove_reference_t<A>>
auto nbranch_ref(G&& function, P&& predicate, A&& alternative) {
    auto base_holder = ni::nhold_object(forward<G>(function));
    auto predicate_holder = ni::nhold_object(forward<P>(predicate));
    auto alternative_holder = ni::nhold_object(forward<A>(alternative));
    return ni::nbranch_ref_function<decltype(base_holder),
                                    decltype(predicate_holder),
                                    decltype(alternative_holder)>(
        move(base_holder), move(predicate_holder), move(alternative_holder));
}

template <class G, class P, class A>
    requires ni::nmutable_discrete_function<remove_reference_t<G>>
[[deprecated("use nbranch_value or nbranch_ref")]]
auto nbranch(G&& function, P&& predicate, A&& alternative) {
    if constexpr (ni::nbranch_ref_compatible<remove_reference_t<G>,
                                               remove_reference_t<A>>)
        return nbranch_ref(forward<G>(function), forward<P>(predicate),
                           forward<A>(alternative));
    else
        return nbranch_value(forward<G>(function), forward<P>(predicate),
                             forward<A>(alternative));
}

namespace ni {
template <class H> struct nrun_state {
    H source;
    nvector<int> starts;
};

template <class State> class nrun_segment {
    shared_ptr<State> state_;
    int left_;
    int right_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    nrun_segment(shared_ptr<State> state, int left, int right)
        : state_(move(state)), left_(left), right_(right) {}

    int len() const { return right_ - left_; }
    bool empty() const { return len() == 0; }
    decltype(auto) key(int index) {
        npre(0 <= index && index < len());
        return ninvoke_public([&]() -> decltype(auto) {
            return state_->source.get().key(left_ + index);
        });
    }
    decltype(auto) key(int index) const {
        npre(0 <= index && index < len());
        return ninvoke_public([&]() -> decltype(auto) {
            return as_const(*state_).source.get().key(left_ + index);
        });
    }
    decltype(auto) operator[](int index) {
        npre(0 <= index && index < len());
        return ninvoke_public([&]() -> decltype(auto) {
            return state_->source.get()[left_ + index];
        });
    }
    decltype(auto) operator[](int index) const {
        npre(0 <= index && index < len());
        return ninvoke_public([&]() -> decltype(auto) {
            return as_const(*state_).source.get()[left_ + index];
        });
    }
    template <class X>
    decltype(auto) operator()(X&& argument)
        requires requires(State& state, X&& value) {
            ninvoke_public(state.source.get(), forward<X>(value));
        }
    {
        return ninvoke_public(state_->source.get(),
                              forward<X>(argument));
    }
    template <class X>
    decltype(auto) operator()(X&& argument) const
        requires requires(const State& state, X&& value) {
            ninvoke_public(state.source.get(), forward<X>(value));
        }
    {
        return ninvoke_public(as_const(*state_).source.get(),
                              forward<X>(argument));
    }
};

template <class H> class nrun_function {
    using state_type = nrun_state<H>;
    shared_ptr<state_type> state_;

    int locate(int start) const {
        int left = 0, right = len();
        while (left < right) {
            int middle = left + (right - left) / 2;
            if (state_->starts[middle] < start)
                left = middle + 1;
            else
                right = middle;
        }
        npre(left < len() && state_->starts[left] == start);
        return left;
    }
    auto segment(int index) {
        npre(0 <= index && index < len());
        int left = state_->starts[index];
        int right = index + 1 < len() ? state_->starts[index + 1]
                                      : nlen(state_->source.get());
        return nrun_segment<state_type>(state_, left, right);
    }
    auto segment(int index) const {
        npre(0 <= index && index < len());
        int left = state_->starts[index];
        int right = index + 1 < len() ? state_->starts[index + 1]
                                      : nlen(state_->source.get());
        shared_ptr<const state_type> constant_state = state_;
        return nrun_segment<const state_type>(move(constant_state), left, right);
    }

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    explicit nrun_function(shared_ptr<state_type> state) : state_(move(state)) {}
    int len() const { return state_->starts.len(); }
    bool empty() const { return len() == 0; }
    int key(int index) const {
        npre(0 <= index && index < len());
        return state_->starts[index];
    }
    int key(int index) { return as_const(*this).key(index); }
    auto operator[](int index) { return segment(index); }
    auto operator[](int index) const { return segment(index); }
    auto operator()(int start) { return segment(locate(start)); }
    auto operator()(int start) const { return segment(locate(start)); }
};
} // namespace ni

template <class G, class P = nequal<>>
    requires ni::nmutable_keyed_indexed<remove_reference_t<G>>
auto nruns(G&& function, P together = {}) {
    auto holder = ni::nhold_object(forward<G>(function));

    nvector<int> starts;
    if (nlen(holder.get()) != 0) {
        starts.push(0);
        for (int index = 1; index < nlen(holder.get()); ++index)
            if (!invoke(together, holder.get()[index - 1], holder.get()[index]))
                starts.push(index);
    }

    using holder_type = decltype(holder);
    using state_type = ni::nrun_state<holder_type>;
    auto state = make_shared<state_type>(
        state_type{move(holder), move(starts)});
    return ni::nrun_function<holder_type>(move(state));
}

template <class A, class P = nequal<>>
    requires nindexed<remove_reference_t<A>> &&
             (!ni::nmutable_keyed_indexed<remove_reference_t<A>>)
auto nruns(A&& source, P together = {}) {
    int size = nlen(source);
    return nruns(nfunc_bind(nrange(size), forward<A>(source)), move(together));
}

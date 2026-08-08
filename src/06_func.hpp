namespace ni {
// A zero-allocation lifetime bridge used by discrete-function adaptors:
// lvalues are borrowed, rvalues are owned. Const always propagates through the
// bridge; shallow const must be represented by a different, explicit type.
// Holder either borrows an lvalue or owns a decayed value.  Any adapter storing it must
// preserve that lifetime distinction; a view cannot outlive a borrowed domain/function.
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

    constexpr decltype(auto) get() & {
        if constexpr (owns)
            return (storage_);
        else
            return (*storage_);
    }
    constexpr decltype(auto) get() const& {
        if constexpr (owns)
            return as_const(storage_);
        else
            return as_const(*storage_);
    }
};

template <class A> constexpr auto nhold_object(A&& value) {
    return nobject_holder<A&&>(forward<A>(value));
}

template <class A> using ndomain_holder = nobject_holder<A>;

template <class R>
using npublic_result_t = conditional_t<is_rvalue_reference_v<R>, remove_cvref_t<R>, R>;

// Public adaptor calls never expose T&&. It is materialized while T/T&/const T&
// keep their natural result category.
template <class F, class... X>
    requires invocable<F&&, X&&...>
constexpr decltype(auto) ninvoke_public(F&& function, X&&... argument) {
    using result_type = invoke_result_t<F&&, X&&...>;
    if constexpr (is_void_v<result_type>) {
        invoke(forward<F>(function), forward<X>(argument)...);
    } else if constexpr (is_rvalue_reference_v<result_type>) {
        return remove_cvref_t<result_type>(
            invoke(forward<F>(function), forward<X>(argument)...));
    } else {
        return invoke(forward<F>(function), forward<X>(argument)...);
    }
}

// Composition has a stronger boundary: when the middle object is ephemeral,
// no reference produced from it may escape the full expression.
template <class F, class M>
    requires invocable<F&&, M&&>
constexpr decltype(auto) ninvoke_stable(F&& function, M&& middle) {
    using result_type = invoke_result_t<F&&, M&&>;
    if constexpr (is_void_v<result_type>) {
        invoke(forward<F>(function), forward<M>(middle));
    } else if constexpr (!is_lvalue_reference_v<M&&> && is_reference_v<result_type>) {
        return remove_cvref_t<result_type>(
            invoke(forward<F>(function), forward<M>(middle)));
    } else {
        return ninvoke_public(forward<F>(function), forward<M>(middle));
    }
}

struct nauto_function_result {};
struct nvalue_function_result {};
struct nreference_function_result {};

template <class Policy, class F, class... X>
concept nfunction_policy_invocable =
    invocable<F, X...> &&
    (same_as<Policy, nauto_function_result> ||
     (same_as<Policy, nvalue_function_result> &&
      (!is_void_v<invoke_result_t<F, X...>>) &&
      constructible_from<remove_cvref_t<invoke_result_t<F, X...>>,
                         invoke_result_t<F, X...>>) ||
     (same_as<Policy, nreference_function_result> &&
      is_lvalue_reference_v<invoke_result_t<F, X...>>));

template <class Policy, class F, class... X>
    requires nfunction_policy_invocable<Policy, F&&, X&&...>
constexpr decltype(auto) ninvoke_function(F&& function, X&&... argument) {
    using result_type = invoke_result_t<F&&, X&&...>;
    if constexpr (same_as<Policy, nvalue_function_result>) {
        return remove_cvref_t<result_type>(
            invoke(forward<F>(function), forward<X>(argument)...));
    } else if constexpr (same_as<Policy, nreference_function_result>) {
        return invoke(forward<F>(function), forward<X>(argument)...);
    } else {
        return ninvoke_public(forward<F>(function), forward<X>(argument)...);
    }
}

template <class Policy, class F, class M>
    requires nfunction_policy_invocable<Policy, F&&, M&&>
constexpr decltype(auto) ninvoke_evaluator(F&& function, M&& argument) {
    if constexpr (same_as<Policy, nauto_function_result>)
        return ninvoke_stable(forward<F>(function), forward<M>(argument));
    else
        return ninvoke_function<Policy>(forward<F>(function),
                                        forward<M>(argument));
}

template <class H> class nfunction_key_access {
    H function_;

  public:
    constexpr explicit nfunction_key_access(H function) : function_(move(function)) {}
    constexpr decltype(auto) operator()(int index) {
        return ninvoke_public([&]() -> decltype(auto) {
            return function_.get().key(index);
        });
    }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& function) { function.get().key(0); }
    {
        return ninvoke_public([&]() -> decltype(auto) {
            return function_.get().key(index);
        });
    }
};

template <class H> class nfunction_value_access {
    H function_;

  public:
    constexpr explicit nfunction_value_access(H function) : function_(move(function)) {}
    constexpr decltype(auto) operator()(int index) {
        return ninvoke_public([&]() -> decltype(auto) {
            return function_.get()[index];
        });
    }
    constexpr decltype(auto) operator()(int index) const
        requires requires(const H& function) { function.get()[0]; }
    {
        return ninvoke_public([&]() -> decltype(auto) {
            return function_.get()[index];
        });
    }
};
} // namespace ni

// A finite discrete function is a keyed view, not an associative container.
// key(i) moves from enumeration position to semantic argument; operator[](i)
// evaluates at that position; operator()(x) evaluates an arbitrary argument.
/**
 * Finite-domain function adapter.  The domain key/index protocol and evaluator result
 * category are part of the type contract; reference policy must never return a dangling
 * temporary.  A borrowed domain/function owner must outlive this adapter.
 */
template <class DH, class FH, class Policy = ni::nauto_function_result>
class nevaluated_function {
    DH domain_;
    [[no_unique_address]] FH evaluate_;

    constexpr auto key_access() & {
        auto holder = ni::nhold_object(*this);
        using access_type = ni::nfunction_key_access<decltype(holder)>;
        return nview(len(), access_type(move(holder)));
    }
    constexpr auto key_access() const& {
        auto holder = ni::nhold_object(*this);
        using access_type = ni::nfunction_key_access<decltype(holder)>;
        return nview(len(), access_type(move(holder)));
    }
    constexpr auto key_access() && {
        auto holder = ni::nhold_object(move(*this));
        int size = nlen(holder.get());
        using access_type = ni::nfunction_key_access<decltype(holder)>;
        return nview(size, access_type(move(holder)));
    }

  public:
    using nrange_tag = void;
    using nfunction_tag = void;
    static constexpr bool nstable_borrow_result =
        same_as<Policy, ni::nreference_function_result>;

    constexpr nevaluated_function(DH domain, FH evaluate)
        : domain_(move(domain)), evaluate_(move(evaluate)) {}

    constexpr int len() const { return nlen(domain_.get()); }
    constexpr bool empty() const { return len() == 0; }

    constexpr decltype(auto) key(int index) {
        npre(0 <= index && index < len());
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return domain_.get()[index];
        });
    }
    constexpr decltype(auto) key(int index) const {
        npre(0 <= index && index < len());
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return domain_.get()[index];
        });
    }

    constexpr decltype(auto) operator[](int index)
        requires requires(FH& evaluate, DH& domain) {
            ni::ninvoke_evaluator<Policy>(evaluate.get(), domain.get()[0]);
        }
    {
        npre(0 <= index && index < len());
        return ni::ninvoke_evaluator<Policy>(evaluate_.get(), domain_.get()[index]);
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const FH& evaluate, const DH& domain) {
            ni::ninvoke_evaluator<Policy>(evaluate.get(), domain.get()[0]);
        }
    {
        npre(0 <= index && index < len());
        return ni::ninvoke_evaluator<Policy>(evaluate_.get(), domain_.get()[index]);
    }

    template <class X>
    constexpr decltype(auto) operator()(X&& argument)
        requires requires(FH& evaluate, X&& value) {
            ni::ninvoke_evaluator<Policy>(evaluate.get(), forward<X>(value));
        }
    {
        return ni::ninvoke_evaluator<Policy>(evaluate_.get(), forward<X>(argument));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires requires(const FH& evaluate, X&& value) {
            ni::ninvoke_evaluator<Policy>(evaluate.get(), forward<X>(value));
        }
    {
        return ni::ninvoke_evaluator<Policy>(evaluate_.get(), forward<X>(argument));
    }

    constexpr auto keys() & { return key_access(); }
    constexpr auto keys() const& { return key_access(); }
    constexpr auto keys() && { return move(*this).key_access(); }
};

template <class A>
concept nkeyed_indexed = nindexed<A> &&
                         requires(A& function, const A& constant, int index) {
                             function.key(index);
                             constant.key(index);
                         };

template <class A> using nfunction_key_reference_t = decltype(declval<A&>().key(0));
template <class A> using nfunction_key_t = remove_cvref_t<nfunction_key_reference_t<A>>;

template <class A>
concept ndiscrete_function =
    nkeyed_indexed<A> && requires(A& function, nfunction_key_t<A> key) {
        function(key);
    };

namespace ni {
// Adaptors also accept mutable-only callables. The public nkeyed_indexed and
// ndiscrete_function concepts remain the stronger regular (const-readable)
// capabilities inherited from nindexed.
template <class A>
concept nmutable_keyed_indexed = requires(A& function, const A& constant,
                                          int index) {
    { nlen(constant) } -> same_as<int>;
    function[index];
    function.key(index);
    constant.key(index);
};

template <class A>
concept nmutable_discrete_function =
    nmutable_keyed_indexed<A> &&
    requires(A& function, nfunction_key_t<A> key) { function(key); };
} // namespace ni

template <class D, class F>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> ||
              constructible_from<remove_cvref_t<D>, D&&>) &&
             (is_lvalue_reference_v<F&&> ||
              constructible_from<remove_cvref_t<F>, F&&>) &&
             ni::nfunction_policy_invocable<
                 ni::nauto_function_result, F&,
                 nindex_reference_t<remove_reference_t<D>>>
constexpr auto nfunc_eval(D&& domain, F&& evaluate) {
    auto domain_holder = ni::nhold_object(forward<D>(domain));
    auto evaluate_holder = ni::nhold_object(forward<F>(evaluate));
    return nevaluated_function<decltype(domain_holder), decltype(evaluate_holder)>(
        move(domain_holder), move(evaluate_holder));
}

template <class D, class F>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> ||
              constructible_from<remove_cvref_t<D>, D&&>) &&
             (is_lvalue_reference_v<F&&> ||
              constructible_from<remove_cvref_t<F>, F&&>) &&
             ni::nfunction_policy_invocable<
                 ni::nvalue_function_result, F&,
                 nindex_reference_t<remove_reference_t<D>>>
constexpr auto nfunc_value(D&& domain, F&& evaluate) {
    auto domain_holder = ni::nhold_object(forward<D>(domain));
    auto evaluate_holder = ni::nhold_object(forward<F>(evaluate));
    return nevaluated_function<decltype(domain_holder), decltype(evaluate_holder),
                               ni::nvalue_function_result>(
        move(domain_holder), move(evaluate_holder));
}

template <class D, class F>
    requires nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> ||
              constructible_from<remove_cvref_t<D>, D&&>) &&
             (is_lvalue_reference_v<F&&> ||
              constructible_from<remove_cvref_t<F>, F&&>) &&
             ni::nfunction_policy_invocable<
                 ni::nreference_function_result, F&,
                 nindex_reference_t<remove_reference_t<D>>>
constexpr auto nfunc_ref(D&& domain, F&& evaluate) {
    auto domain_holder = ni::nhold_object(forward<D>(domain));
    auto evaluate_holder = ni::nhold_object(forward<F>(evaluate));
    return nevaluated_function<decltype(domain_holder), decltype(evaluate_holder),
                               ni::nreference_function_result>(
        move(domain_holder), move(evaluate_holder));
}

// Compatibility evaluator entry. Indexed+invocable arguments are rejected by
// the diagnostic overload declared after nfunc_bind becomes available.
template <class D, class F>
    requires nindexed<remove_reference_t<D>> &&
             (!nindexed<remove_reference_t<F>>) &&
             invocable<F&, nindex_reference_t<remove_reference_t<D>>>
[[deprecated("use nfunc_value, nfunc_ref, or nfunc_eval")]]
constexpr auto nfunc(D&& domain, F&& evaluate) {
    using result_type =
        invoke_result_t<F&, nindex_reference_t<remove_reference_t<D>>>;
    if constexpr (is_lvalue_reference_v<result_type>)
        return nfunc_ref(forward<D>(domain), forward<F>(evaluate));
    else
        return nfunc_eval(forward<D>(domain), forward<F>(evaluate));
}

namespace ni {
template <class H> class nfunction_entry_access {
    H function_;

    template <class G> static constexpr auto entry(G& function, int index) {
        using key_result = decltype(function.key(index));
        using value_result = decltype(function[index]);
        using key_type = npublic_result_t<key_result>;
        using value_type = npublic_result_t<value_result>;
        return pair<key_type, value_type>(function.key(index), function[index]);
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
    requires nkeyed_indexed<remove_reference_t<G>>
constexpr auto nkeys(G&& function) {
    auto holder = ni::nhold_object(forward<G>(function));
    int size = nlen(holder.get());
    using access_type = ni::nfunction_key_access<decltype(holder)>;
    return nview(size, access_type(move(holder)));
}

template <class G>
    requires nkeyed_indexed<remove_reference_t<G>>
constexpr auto nvalues(G&& function) {
    auto holder = ni::nhold_object(forward<G>(function));
    int size = nlen(holder.get());
    using access_type = ni::nfunction_value_access<decltype(holder)>;
    return nview(size, access_type(move(holder)));
}

template <class G>
    requires nkeyed_indexed<remove_reference_t<G>>
constexpr auto nentries(G&& function) {
    auto holder = ni::nhold_object(forward<G>(function));
    int size = nlen(holder.get());
    using access_type = ni::nfunction_entry_access<decltype(holder)>;
    return nview(size, access_type(move(holder)));
}

namespace ni {
template <class D, class X>
concept ndomain_membership_testable =
    requires(const D& domain, const X& value) {
        { domain.position(value) } -> convertible_to<int>;
    } || requires(const D& domain, const X& value) {
        { domain[0] == value } -> convertible_to<bool>;
    };

template <class D, class X>
    requires ndomain_membership_testable<D, X>
constexpr bool ndomain_contains(const D& domain, const X& value) {
    if constexpr (requires { domain.position(value); }) {
        return domain.position(value) != npos;
    } else {
        for (int index = 0; index < nlen(domain); ++index)
            if (domain[index] == value)
                return true;
        return false;
    }
}
} // namespace ni

// Re-domain adapter changes only the visible support.  If CheckMembership is true the
// domain membership predicate must be exact; all borrowed owners must outlive the view.
template <class GH, class DH, bool CheckMembership> class nredomain_function {
    GH function_;
    DH domain_;

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    constexpr nredomain_function(GH function, DH domain)
        : function_(move(function)), domain_(move(domain)) {}

    constexpr int len() const { return nlen(domain_.get()); }
    constexpr bool empty() const { return len() == 0; }
    constexpr decltype(auto) key(int index) {
        npre(0 <= index && index < len());
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return domain_.get()[index];
        });
    }
    constexpr decltype(auto) key(int index) const {
        npre(0 <= index && index < len());
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return domain_.get()[index];
        });
    }
    constexpr decltype(auto) operator[](int index) {
        npre(0 <= index && index < len());
        return ni::ninvoke_public(function_.get(), domain_.get()[index]);
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const GH& function, const DH& domain) {
            ni::ninvoke_public(function.get(), domain.get()[0]);
        }
    {
        npre(0 <= index && index < len());
        return ni::ninvoke_public(function_.get(), domain_.get()[index]);
    }

    template <class X>
    constexpr decltype(auto) operator()(X&& argument)
        requires(!CheckMembership ||
                 ni::ndomain_membership_testable<
                     remove_reference_t<decltype(declval<DH&>().get())>,
                     remove_reference_t<X>>)
    {
        if constexpr (CheckMembership)
            npre(ni::ndomain_contains(domain_.get(), argument));
        return ni::ninvoke_public(function_.get(), forward<X>(argument));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires requires(const GH& function, X&& value) {
            ni::ninvoke_public(function.get(), forward<X>(value));
        } &&
                 (!CheckMembership ||
                  ni::ndomain_membership_testable<
                      remove_reference_t<decltype(declval<const DH&>().get())>,
                      remove_reference_t<X>>)
    {
        if constexpr (CheckMembership)
            npre(ni::ndomain_contains(domain_.get(), argument));
        return ni::ninvoke_public(function_.get(), forward<X>(argument));
    }
};

template <class G, class D>
    requires ni::nmutable_discrete_function<remove_reference_t<G>> &&
             nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> ||
              constructible_from<remove_cvref_t<D>, D&&>)
constexpr auto nredomain(G&& function, D&& domain) {
    auto function_holder = ni::nhold_object(forward<G>(function));
    auto domain_holder = ni::nhold_object(forward<D>(domain));
    return nredomain_function<decltype(function_holder), decltype(domain_holder), false>(
        move(function_holder), move(domain_holder));
}

// True semantic restriction: [] enumerates the supplied domain, and () rejects
// arguments outside it. Domains with position() check in O(1); generic domains
// use an explicit O(n) membership scan.
template <class G, class D>
    requires ni::nmutable_discrete_function<remove_reference_t<G>> &&
             nindexed<remove_reference_t<D>> &&
             (is_lvalue_reference_v<D&&> ||
              constructible_from<remove_cvref_t<D>, D&&>)
constexpr auto nrestrict(G&& function, D&& domain) {
    auto function_holder = ni::nhold_object(forward<G>(function));
    auto domain_holder = ni::nhold_object(forward<D>(domain));
    return nredomain_function<decltype(function_holder), decltype(domain_holder), true>(
        move(function_holder), move(domain_holder));
}

// Composition owns/borrows two function holders and is valid only where inner output
// belongs to the outer domain.  Result reference policy follows the outer evaluator.
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
    constexpr decltype(auto) key(int index) {
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return inner_.get().key(index);
        });
    }
    constexpr decltype(auto) key(int index) const {
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return inner_.get().key(index);
        });
    }

    constexpr decltype(auto) operator[](int index) {
        npre(0 <= index && index < len());
        return ni::ninvoke_stable(outer_.get(), inner_.get()[index]);
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const OH& outer, const IH& inner) {
            ni::ninvoke_stable(outer.get(), inner.get()[0]);
        }
    {
        npre(0 <= index && index < len());
        return ni::ninvoke_stable(outer_.get(), inner_.get()[index]);
    }

    template <class X> constexpr decltype(auto) operator()(X&& argument) {
        return ni::ninvoke_stable(
            outer_.get(),
            ni::ninvoke_public(inner_.get(), forward<X>(argument)));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires requires(const OH& outer, const IH& inner, X&& value) {
            ni::ninvoke_stable(
                outer.get(), ni::ninvoke_public(inner.get(), forward<X>(value)));
        }
    {
        return ni::ninvoke_stable(
            outer_.get(),
            ni::ninvoke_public(inner_.get(), forward<X>(argument)));
    }
};

template <class Outer, class Inner>
    requires ni::nmutable_discrete_function<remove_reference_t<Inner>>
constexpr auto ncompose(Outer&& outer, Inner&& inner) {
    auto outer_holder = ni::nhold_object(forward<Outer>(outer));
    auto inner_holder = ni::nhold_object(forward<Inner>(inner));
    return ncomposed_function<decltype(outer_holder), decltype(inner_holder)>(
        move(outer_holder), move(inner_holder));
}

template <class G, class F>
    requires ni::nmutable_discrete_function<remove_reference_t<G>>
constexpr auto nmap_values(G&& function, F&& transform) {
    return ncompose(forward<F>(transform), forward<G>(function));
}

template <class GH, class PH> class nselected_positions_function {
    GH function_;
    PH positions_;

    constexpr int checked_source_index(int index) const {
        npre(0 <= index && index < len());
        int position = ni::nchecked_int(positions_.get()[index]);
        npre(0 <= position && position < nlen(function_.get()));
        return position;
    }

  public:
    using nrange_tag = void;
    using nfunction_tag = void;

    constexpr nselected_positions_function(GH function, PH positions)
        : function_(move(function)), positions_(move(positions)) {}
    constexpr int len() const { return nlen(positions_.get()); }
    constexpr bool empty() const { return len() == 0; }
    constexpr int source_index(int index) const { return checked_source_index(index); }
    [[deprecated("use source_index")]]
    constexpr int position(int index) const { return source_index(index); }
    constexpr decltype(auto) key(int index) {
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return function_.get().key(checked_source_index(index));
        });
    }
    constexpr decltype(auto) key(int index) const {
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return function_.get().key(checked_source_index(index));
        });
    }
    constexpr decltype(auto) operator[](int index) {
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return function_.get()[checked_source_index(index)];
        });
    }
    constexpr decltype(auto) operator[](int index) const
        requires requires(const GH& function) { function.get()[0]; }
    {
        return ni::ninvoke_public([&]() -> decltype(auto) {
            return function_.get()[checked_source_index(index)];
        });
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument)
        requires requires(GH& function, X&& value) {
            ni::ninvoke_public(function.get(), forward<X>(value));
        }
    {
        return ni::ninvoke_public(function_.get(), forward<X>(argument));
    }
    template <class X>
    constexpr decltype(auto) operator()(X&& argument) const
        requires requires(const GH& function, X&& value) {
            ni::ninvoke_public(function.get(), forward<X>(value));
        }
    {
        return ni::ninvoke_public(function_.get(), forward<X>(argument));
    }
};

// Positional selection: positions index the source enumeration, while the result
// keeps the source semantic keys. Repeated positions deliberately keep aliases.
template <class G, class P>
    requires ni::nmutable_keyed_indexed<remove_reference_t<G>> &&
             nindexed<remove_reference_t<P>> &&
             integral<nindex_value_t<remove_reference_t<P>>> &&
             (is_lvalue_reference_v<P&&> ||
              constructible_from<remove_cvref_t<P>, P&&>)
constexpr auto nselect_positions(G&& function, P&& positions) {
    auto function_holder = ni::nhold_object(forward<G>(function));
    auto position_holder = ni::nhold_object(forward<P>(positions));
    return nselected_positions_function<decltype(function_holder),
                                        decltype(position_holder)>(
        move(function_holder), move(position_holder));
}

template <class G, class P>
    requires ni::nmutable_keyed_indexed<remove_reference_t<G>> &&
             nindexed<remove_reference_t<P>> &&
             integral<nindex_value_t<remove_reference_t<P>>>
[[deprecated("use nselect_positions")]]
constexpr auto ngather(G&& function, P&& positions) {
    return nselect_positions(forward<G>(function), forward<P>(positions));
}

template <class G>
    requires ni::nmutable_keyed_indexed<remove_reference_t<G>>
constexpr auto nsubfunc(G&& function, int left, int right) {
    npre(0 <= left && left <= right && right <= nlen(function));
    return nselect_positions(forward<G>(function), nrange(left, right));
}

template <class G>
    requires ni::nmutable_keyed_indexed<remove_reference_t<G>>
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
    requires ni::nmutable_keyed_indexed<remove_reference_t<G>>
constexpr auto nblocks(G&& function, int width) {
    auto holder = ni::nhold_object(forward<G>(function));
    npre(width > 0);
    int size = nlen(holder.get());
    int count = size / width + (size % width != 0);
    using access_type = ni::nfunction_block_access<decltype(holder)>;
    return nview(count, access_type(move(holder), width));
}

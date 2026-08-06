#include "common.hpp"

template <class F>
concept nconst_indexable = requires(const F& function) { function[0]; };

struct MutableCounter {
    unique_ptr<int> value = make_unique<int>(0);

    MutableCounter() = default;
    MutableCounter(MutableCounter&&) = default;
    MutableCounter& operator=(MutableCounter&&) = default;
    MutableCounter(const MutableCounter&) = delete;
    MutableCounter& operator=(const MutableCounter&) = delete;

    int operator()(int) { return ++*value; }
};

struct Both {
    int len() const { return 1; }
    int operator[](int) const { return 7; }
    int operator()(int) const { return 11; }
};

struct KeyedOnly {
    nvector<int> keys{10, 20, 30};
    nvector<int> values{1, 2, 3};

    int len() const { return keys.len(); }
    const int& key(int index) const { return keys[index]; }
    int& key(int index) { return keys[index]; }
    const int& operator[](int index) const { return values[index]; }
    int& operator[](int index) { return values[index]; }
};

struct RvalueKeyed {
    mutable int stored_key = 4;
    mutable int stored_value = 9;

    int len() const { return 1; }
    int&& key(int) const { return move(stored_key); }
    int&& key(int) { return move(stored_key); }
    int&& operator[](int) const { return move(stored_value); }
    int&& operator[](int) { return move(stored_value); }
};

struct ValueDomain {
    int len() const { return 1; }
    string operator[](int) const { return string(1000, 'd'); }
    string operator[](int) { return string(1000, 'd'); }
};

struct IsNegative {
    bool operator()(int value) const { return value < 0; }
};

struct RefAlternative {
    long long* value;
    long long& operator()(int) const { return *value; }
};

struct ValueAlternative {
    long long operator()(int) const { return 0; }
};

template <class G, class A>
concept ncan_branch_ref = requires(G& function, A& alternative) {
    nbranch_ref(function, IsNegative{}, alternative);
};

struct TrackedDomain {
    shared_ptr<int> copies;
    array<int, 3> values{4, 5, 6};

    explicit TrackedDomain(shared_ptr<int> count) : copies(move(count)) {}
    TrackedDomain(const TrackedDomain& other)
        : copies(other.copies), values(other.values) {
        ++*copies;
    }
    TrackedDomain(TrackedDomain&&) = default;

    int len() const { return int(values.size()); }
    int& operator[](int index) { return values[index]; }
    const int& operator[](int index) const { return values[index]; }
};

template <class F, class K, class MutableIndex, class ConstIndex,
          class MutableCall, class ConstCall>
concept nadaptor_contract_matrix =
    same_as<decltype(declval<F&>()[0]), MutableIndex> &&
    same_as<decltype(declval<const F&>()[0]), ConstIndex> &&
    same_as<decltype(declval<F&>()(declval<K&>())), MutableCall> &&
    same_as<decltype(declval<const F&>()(declval<K&>())), ConstCall>;

template <class MutableIndex, class ConstIndex, class MutableCall,
          class ConstCall, class F, class K>
bool ncheck_adaptor_contract(F& function, K key, int expected) {
    static_assert(nadaptor_contract_matrix<F, K, MutableIndex, ConstIndex,
                                           MutableCall, ConstCall>);
    return function[0] == expected && as_const(function)[0] == expected &&
           function(key) == expected && as_const(function)(key) == expected;
}

struct MatrixFunction {
    nvector<int> values{11, 11, 22, 22};

    int len() const { return values.len(); }
    int key(int index) const { return index + 10; }
    int key(int index) { return as_const(*this).key(index); }
    int& operator[](int index) { return values[index]; }
    const int& operator[](int index) const { return values[index]; }
    int& operator()(int key) { return values[key - 10]; }
    const int& operator()(int key) const { return values[key - 10]; }
};

struct MatrixIdentity {
    int& operator()(int& value) { return value; }
    const int& operator()(const int& value) const { return value; }
};

struct NamedKeyPredicate {
    bool operator()(int& key) const { return key < 0; }
    bool operator()(int&&) const = delete;
};

struct NamedKeyValueAlternative {
    int operator()(int& key) const { return -key; }
    int operator()(int&&) const = delete;
};

struct NamedKeyRefAlternative {
    int* value;

    int& operator()(int&) { return *value; }
    const int& operator()(int&) const { return *value; }
    int& operator()(int&&) = delete;
    const int& operator()(int&&) const = delete;
};

int main() {
    static_assert(nindexed<Both> && invocable<Both&, int>);
    auto explicit_eval = nfunc_value(nrange(1), Both{});
    auto explicit_bind = nfunc_bind(nrange(1), Both{});
    ntest(explicit_eval[0] == 11 && explicit_bind[0] == 7);

    auto move_only = nfunc_value(nrange(2), MutableCounter{});
    ntest(move_only[0] == 1 && move_only[1] == 2);
    static_assert(!nconst_indexable<decltype(move_only)>);

    MutableCounter borrowed_callable;
    auto borrowed = nfunc_eval(nrange(2), borrowed_callable);
    auto borrowed_restricted = nrestrict(borrowed, nrange(2));
    ntest(borrowed_restricted[0] == 1 && borrowed_restricted[1] == 2);
    static_assert(!nconst_indexable<decltype(borrowed_restricted)>);

    auto owned_restricted =
        nrestrict(nfunc_eval(nrange(2), MutableCounter{}), nrange(2));
    ntest(owned_restricted[0] == 1 && owned_restricted[1] == 2);
    static_assert(!nconst_indexable<decltype(owned_restricted)>);

    auto value_inner = nfunc_value(nrange(1), [](int) {
        return string(1000, 'x');
    });
    auto stable_identity = ncompose(nidentity{}, value_inner);
    static_assert(same_as<decltype(stable_identity[0]), string>);
    static_assert(same_as<decltype(stable_identity(0)), string>);
    auto&& result = stable_identity[0];
    ntest(result.size() == 1000 && result[999] == 'x');

    auto stable_eval = nfunc_eval(
        ValueDomain{}, [](const string& value) -> const string& { return value; });
    static_assert(same_as<decltype(stable_eval[0]), string>);
    static_assert(same_as<decltype(stable_eval(string(1000, 'a'))), string>);
    auto&& evaluated = stable_eval[0];
    ntest(evaluated.size() == 1000 && evaluated[999] == 'd');

    int moved_from = 73;
    auto rvalue_result = nfunc_eval(nrange(1), [&](int) -> int&& {
        return move(moved_from);
    });
    static_assert(same_as<decltype(rvalue_result[0]), int>);
    ntest(rvalue_result[0] == 73);

    string stored = "river";
    auto reference_inner = nfunc_ref(nrange(1), [&](int) -> string& {
        return stored;
    });
    auto reference_composed = ncompose(
        [](string& value) -> string& { return value; }, reference_inner);
    static_assert(same_as<decltype(reference_composed[0]), string&>);
    reference_composed[0] = "kappa";
    ntest(stored == "kappa");

    auto reference_to_value =
        ncompose([](const string& value) { return value.size(); }, reference_inner);
    static_assert(same_as<decltype(reference_to_value[0]), size_t>);
    ntest(reference_to_value[0] == 5);

    auto value_copy = nfunc_value(nrange(1), [&](int) -> string& { return stored; });
    static_assert(same_as<decltype(value_copy[0]), string>);
    value_copy[0] = "copy";
    ntest(stored == "kappa");

    const int immutable = 19;
    auto const_reference = nfunc_ref(nrange(1), [&](int) -> const int& {
        return immutable;
    });
    static_assert(same_as<decltype(const_reference[0]), const int&>);
    ntest(const_reference[0] == 19);

    const nvector<int> bound_constants{3, 4};
    auto const_bound = nfunc_bind(nrange(2), bound_constants);
    static_assert(same_as<decltype(const_bound[0]), const int&>);
    static_assert(same_as<decltype(as_const(const_bound)[0]), const int&>);
    ntest(const_bound(1) == 4);

    nvector<long long> dp{5, 8, 13};
    auto state = nfunc_ref(nrange(dp.len()), [&](int index) -> long long& {
        return dp[index];
    });
    long long sentinel = -1;
    auto writable =
        nbranch_ref(state, IsNegative{}, RefAlternative{&sentinel});
    static_assert(same_as<decltype(writable(0)), long long&>);
    static_assert(ncan_branch_ref<decltype(state), RefAlternative>);
    static_assert(!ncan_branch_ref<decltype(state), ValueAlternative>);
    writable(-1) = 21;
    writable(1) = 34;
    ntest(sentinel == 21 && dp[1] == 34);

    auto mixed = nbranch_value(state, IsNegative{}, ValueAlternative{});
    static_assert(same_as<decltype(mixed(0)), long long>);
    ntest(mixed(-1) == 0 && mixed(2) == 13);

    auto square = nfunc_value(nrange(3), [](int value) { return value * value; });
    auto redomain = nredomain(square, nvector<int>{2});
    auto restricted = nrestrict(square, nvector<int>{2});
    ntest(redomain[0] == 4 && redomain(5) == 25);
    ntest(restricted[0] == 4 && restricted(2) == 4);

    KeyedOnly keyed;
    static_assert(nkeyed_indexed<KeyedOnly>);
    static_assert(!ndiscrete_function<KeyedOnly>);
    auto selected = nselect_positions(keyed, nvector<int>{2, 0, 2});
    static_assert(nkeyed_indexed<decltype(selected)>);
    static_assert(!ndiscrete_function<decltype(selected)>);
    ntest(selected.source_index(1) == 0);
    selected[0] = 30;
    ntest(keyed.values[2] == 30 && selected[2] == 30);

    RvalueKeyed unstable_source;
    auto stable_keys = nkeys(unstable_source);
    auto stable_values = nvalues(unstable_source);
    auto stable_selection = nselect_positions(unstable_source, nrange(1));
    static_assert(same_as<decltype(stable_keys[0]), int>);
    static_assert(same_as<decltype(stable_values[0]), int>);
    static_assert(same_as<decltype(stable_selection.key(0)), int>);
    static_assert(same_as<decltype(stable_selection[0]), int>);
    ntest(stable_keys[0] == 4 && stable_values[0] == 9);

    nvector<int> colors{1, 1, 2, 2};
    auto runs = nruns(colors);
    auto first = runs[0];
    static_assert(same_as<decltype(first[0]), int&>);
    first[0] = 9;
    ntest(colors[0] == 9);
    const auto constant_first = runs[0];
    static_assert(same_as<decltype(constant_first[0]), const int&>);
    const auto& constant_runs = runs;
    auto constant_second = constant_runs[1];
    static_assert(same_as<decltype(constant_second[0]), const int&>);
    ntest(constant_second[0] == 2);

    auto detached = nruns(nvector<int>{7, 7, 8})[0];
    ntest(ncollect(detached) == nvector<int>({7, 7}));

    auto copies = make_shared<int>(0);
    TrackedDomain domain(copies);
    auto keyed_values = nfunc_value(domain, [](int key) { return key * 10; });
    auto borrowed_keys = keyed_values.keys();
    ntest(*copies == 0);
    ntest(ncollect(borrowed_keys) == nvector<int>({4, 5, 6}));
    ntest(*copies == 0);

    auto owned_keys =
        nfunc_value(TrackedDomain(copies), [](int key) { return key; }).keys();
    ntest(ncollect(owned_keys) == nvector<int>({4, 5, 6}));

    MatrixFunction matrix_source;

    auto matrix_eval = nfunc_eval(nrange(10, 14), matrix_source);
    ntest((ncheck_adaptor_contract<int, int, int&, const int&>(
        matrix_eval, 10, 11)));

    auto matrix_value = nfunc_value(nrange(10, 14), matrix_source);
    ntest((ncheck_adaptor_contract<int, int, int, int>(matrix_value, 10,
                                                       11)));

    auto matrix_ref = nfunc_ref(nrange(10, 14), matrix_source);
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_ref, 10, 11)));

    auto matrix_bound = nfunc_bind(nrange(10, 14), matrix_source.values);
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_bound, 10, 11)));

    auto matrix_anchored = nanchors(matrix_source.values, nrange(10, 14));
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_anchored, 10, 11)));

    auto matrix_redomain = nredomain(matrix_source, nrange(10, 14));
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_redomain, 10, 11)));

    auto matrix_restricted = nrestrict(matrix_source, nrange(10, 14));
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_restricted, 10, 11)));

    auto matrix_composed = ncompose(MatrixIdentity{}, matrix_source);
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_composed, 10, 11)));

    auto matrix_mapped = nmap_values(matrix_source, MatrixIdentity{});
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_mapped, 10, 11)));

    auto matrix_selected = nselect_positions(matrix_source, nrange(4));
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_selected, 10, 11)));

    auto matrix_subfunction = nsubfunc(matrix_source, 0, 4);
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_subfunction, 10, 11)));

    auto matrix_block = nblock(matrix_source, 0, 4);
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_block, 10, 11)));

    auto matrix_blocks = nblocks(matrix_source, 2);
    auto matrix_block_from_view = matrix_blocks[0];
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_block_from_view, 10, 11)));
    auto constant_matrix_block = as_const(matrix_blocks)[0];
    ntest((ncheck_adaptor_contract<const int&, const int&, const int&,
                                  const int&>(constant_matrix_block, 10, 11)));

    auto matrix_branch_value =
        nbranch_value(matrix_source, NamedKeyPredicate{},
                      NamedKeyValueAlternative{});
    ntest((ncheck_adaptor_contract<int, int, int, int>(matrix_branch_value,
                                                       10, 11)));
    static_assert(same_as<decltype(matrix_branch_value(-1)), int>);
    static_assert(same_as<decltype(as_const(matrix_branch_value)(-1)), int>);
    ntest(matrix_branch_value(-1) == 1 &&
          as_const(matrix_branch_value)(-1) == 1);

    int matrix_sentinel = -1;
    auto matrix_branch_ref =
        nbranch_ref(matrix_source, NamedKeyPredicate{},
                    NamedKeyRefAlternative{&matrix_sentinel});
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        matrix_branch_ref, 10, 11)));
    static_assert(same_as<decltype(matrix_branch_ref(-1)), int&>);
    static_assert(
        same_as<decltype(as_const(matrix_branch_ref)(-1)), const int&>);
    matrix_branch_ref(-1) = 37;
    ntest(as_const(matrix_branch_ref)(-1) == 37);

    auto matrix_runs = nruns(matrix_source);
    using MutableRun = decltype(matrix_runs[0]);
    using ConstRun = decltype(as_const(matrix_runs)[0]);
    static_assert(same_as<MutableRun, decltype(matrix_runs(0))>);
    static_assert(same_as<ConstRun, decltype(as_const(matrix_runs)(0))>);
    static_assert(!same_as<MutableRun, ConstRun>);

    auto mutable_run_by_index = matrix_runs[0];
    auto mutable_run_by_call = matrix_runs(0);
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        mutable_run_by_index, 10, 11)));
    ntest((ncheck_adaptor_contract<int&, const int&, int&, const int&>(
        mutable_run_by_call, 10, 11)));

    auto constant_run_by_index = as_const(matrix_runs)[0];
    auto constant_run_by_call = as_const(matrix_runs)(0);
    ntest((ncheck_adaptor_contract<const int&, const int&, const int&,
                                  const int&>(constant_run_by_index, 10, 11)));
    ntest((ncheck_adaptor_contract<const int&, const int&, const int&,
                                  const int&>(constant_run_by_call, 10, 11)));
}

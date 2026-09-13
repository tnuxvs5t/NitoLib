#pragma once
#include "core.hpp"

/*
Owning doubly linked list, with one allocation per element and a valueless sentinel.
Insertion and splice preserve element addresses/iterators; erase invalidates only the
erased elements.  Iterators follow transferred nodes into their destination list.
end() belongs to the list object and does not transfer on move.  T need not be default
constructible, copyable or movable for emplace/splice.  Copying requires copyable T.

All iterator arguments belong to the indicated live list; erase excludes end().  A
splice range [first,last) is forward reachable without crossing the sentinel, and its
destination is outside its interior.  Same-list insertion at first/last is a no-op.
len() fits nidx_t.  No positional indexing or automatic ownership checks are provided.
*/
template <class T>
class nlist {
    struct link { link* prev; link* next; };
    struct node : link {
        T value;
        template <class... A>
        explicit node(A&&... args) : link{}, value(forward<A>(args)...) {}
    };
    link sentinel{&sentinel, &sentinel};
    nidx_t length = 0;

    static void join(link* left, link* right) {
        left->next = right;
        right->prev = left;
    }

    static void transfer(link* position, link* first, link* last) {
        if (first == last || position == first || position == last) return;
        link* tail = last->prev;
        join(first->prev, last);
        join(position->prev, first);
        join(tail, position);
    }

public:
    template <bool Const>
    class cursor {
        friend class nlist;
        template <bool> friend class cursor;
        link* current = nullptr;
        explicit cursor(link* position) : current(position) {}
    public:
        using value_type = T;
        using difference_type = nidx_t;
        using reference = conditional_t<Const, const T&, T&>;
        using pointer = conditional_t<Const, const T*, T*>;
        using iterator_category = bidirectional_iterator_tag;
        using iterator_concept = bidirectional_iterator_tag;

        cursor() = default;
        template <bool Other> requires (Const && !Other)
        cursor(cursor<Other> other) : current(other.current) {}
        reference operator*() const { return static_cast<node*>(current)->value; }
        pointer operator->() const { return addressof(**this); }
        cursor& operator++() { current = current->next; return *this; }
        cursor operator++(int) { auto old = *this; ++*this; return old; }
        cursor& operator--() { current = current->prev; return *this; }
        cursor operator--(int) { auto old = *this; --*this; return old; }
        friend bool operator==(cursor, cursor) = default;
    };

    using iterator = cursor<false>;
    using const_iterator = cursor<true>;

    nlist() = default;
    nlist(const nlist& other) requires is_copy_constructible_v<T> : nlist() {
        for (const T& value : other) emplace(end(), value);
    }
    nlist(nlist&& other) noexcept : nlist() { splice(end(), other); }
    nlist& operator=(nlist other) noexcept {
        clear();
        splice(end(), other);
        return *this;
    }
    ~nlist() { clear(); }

    nidx_t len() const { return length; }
    bool empty() const { return !length; }
    iterator begin() { return iterator(sentinel.next); }
    iterator end() { return iterator(&sentinel); }
    const_iterator begin() const { return const_iterator(sentinel.next); }
    const_iterator end() const { return const_iterator(const_cast<link*>(&sentinel)); }
    T& front() { return *begin(); }
    const T& front() const { return *begin(); }
    T& back() { return *--end(); }
    const T& back() const { return *--end(); }

    /* O(1) links plus allocation/construction; throwing construction leaves the list intact. */
    template <class... A>
    iterator emplace(const_iterator position, A&&... args) {
        auto* added = new node(forward<A>(args)...);
        join(position.current->prev, added);
        join(added, position.current);
        ++length;
        return iterator(added);
    }
    template <class U>
    iterator insert(const_iterator position, U&& value) {
        return emplace(position, forward<U>(value));
    }
    template <class... A>
    T& emplace_front(A&&... args) { return *emplace(begin(), forward<A>(args)...); }
    template <class... A>
    T& emplace_back(A&&... args) { return *emplace(end(), forward<A>(args)...); }
    template <class U>
    void push_front(U&& value) { emplace_front(forward<U>(value)); }
    template <class U>
    void push_back(U&& value) { emplace_back(forward<U>(value)); }

    iterator erase(const_iterator position) {
        link* removed = position.current;
        iterator after(removed->next);
        join(removed->prev, removed->next);
        delete static_cast<node*>(removed);
        --length;
        return after;
    }
    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) first = erase(first);
        return iterator(last.current);
    }
    void pop_front() { erase(begin()); }
    void pop_back() { erase(--end()); }
    void clear() noexcept { while (!empty()) pop_front(); }

    /* Whole-list and single-element transfer are O(1), with no allocation or T operation. */
    void splice(const_iterator position, nlist& other) noexcept {
        if (this == &other) return;
        transfer(position.current, other.sentinel.next, &other.sentinel);
        length += exchange(other.length, 0);
    }
    void splice(const_iterator position, nlist& other, const_iterator element) noexcept {
        auto after = element;
        ++after;
        transfer(position.current, element.current, after.current);
        if (this != &other) ++length, --other.length;
    }
    /* Same-list range transfer is O(1); cross-list transfer counts k elements in O(k). */
    void splice(const_iterator position, nlist& other,
                const_iterator first, const_iterator last) noexcept {
        if (this != &other) {
            nidx_t count = distance(first, last);
            length += count;
            other.length -= count;
        }
        transfer(position.current, first.current, last.current);
    }

    /* O(n), preserves every element address and iterator. */
    void reverse() noexcept {
        link* current = &sentinel;
        do {
            swap(current->prev, current->next);
            current = current->prev;
        } while (current != &sentinel);
    }
};

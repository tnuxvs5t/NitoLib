#include "common.hpp"

// This tiny owner deliberately recycles a live resource without publishing a
// topology epoch.  The high-level DS owners always touch after mutation; this
// test protects the lower resource boundary as well, where generation is the
// only remaining defense against an ABA handle.
class recycled_node_owner {
  public:
    using value_type = int;
    using info_type = int;
    using node_view = nnode_view<recycled_node_owner>;

  private:
    nnode_domain<int> domain_;
    int root_ = 0;

  public:
    recycled_node_owner() : root_(domain_.make(0)) {}

    void recycle(int value) {
        npre(root_ && domain_.alive(root_));
        domain_.erase(root_);
        root_ = domain_.make(value);
    }

    node_view root() const { return node_view(this, root_, domain_.epoch()); }

    uint64_t nnode_epoch() const noexcept { return domain_.epoch(); }
    const void* nnode_domain_token() const noexcept { return domain_.domain_token(); }
    nnode_identity nnode_identity_of(int handle) const noexcept { return domain_.identity(handle); }
    bool nnode_alive(int handle) const noexcept { return domain_.alive(handle); }
    const int& nnode_val(int handle) const { return domain_[handle]; }
    int nnode_count(int handle) const { return handle ? 1 : 0; }
    int nnode_len(int handle) const { return handle ? 1 : 0; }
    int nnode_info(int handle) const { return handle ? domain_[handle] : 0; }
    int nnode_left(int) const noexcept { return 0; }
    int nnode_right(int) const noexcept { return 0; }
};

int main() {
    recycled_node_owner owner;
    auto old = owner.root();
    auto old_identity = old.identity();
    ntest(old && old.current() && old_identity);

    owner.recycle(7);
    auto fresh = owner.root();
    ntest(fresh && fresh.current() && fresh.val() == 7);
    ntest(old.handle() == fresh.handle());
    ntest(old_identity != fresh.identity());
    ntest(!old.current() && !old.ok());

    nseq_fhq<int> source{1, 2, 3};
    auto source_view = source.root();
    nseq_fhq<int> copy = source;
    ntest(source_view.current() && !source_view.same_domain(copy.root()));
    nseq_fhq<int> moved = move(source);
    ntest(!source_view.current() && moved.root().current());
    ntest(!source.root().current());
}

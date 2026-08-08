#include "common.hpp"

class recycled_node_owner_for_death {
  public:
    using value_type = int;
    using info_type = int;
    using node_view = nnode_view<recycled_node_owner_for_death>;

  private:
    nnode_domain<int> domain_;
    int root_ = 0;

  public:
    recycled_node_owner_for_death() : root_(domain_.make(1)) {}

    void recycle() {
        domain_.erase(root_);
        root_ = domain_.make(2);
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
    recycled_node_owner_for_death owner;
    auto stale = owner.root();
    owner.recycle();
    stale.val();
}

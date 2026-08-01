template <int Alphabet>
    requires(Alphabet > 0)
class ntrie {
    struct node {
        array<int, Alphabet> next;
        int parent = npos, symbol = npos, terminal = 0, passing = 0;
        node(int parent = npos, int symbol = npos) : parent(parent), symbol(symbol) { next.fill(npos); }
    };
    nvector<node> nodes_{node{}};

  public:
    int len() const noexcept { return nodes_.len(); }

    template <nindexed A> int add(const A& sequence) {
        int vertex = 0;
        ++nodes_[vertex].passing;
        for (int i = 0; i < nlen(sequence); ++i) {
            int symbol = int(sequence[i]);
            npre(0 <= symbol && symbol < Alphabet);
            int next = nodes_[vertex].next[symbol];
            if (next == npos) {
                next = nodes_.len();
                nodes_[vertex].next[symbol] = next;
                nodes_.push(vertex, symbol);
            }
            vertex = next;
            ++nodes_[vertex].passing;
        }
        ++nodes_[vertex].terminal;
        return vertex;
    }

    template <nindexed A> int find(const A& sequence, int fallback = npos) const {
        int vertex = 0;
        for (int i = 0; i < nlen(sequence); ++i) {
            int symbol = int(sequence[i]);
            if (symbol < 0 || symbol >= Alphabet || nodes_[vertex].next[symbol] == npos)
                return fallback;
            vertex = nodes_[vertex].next[symbol];
        }
        return vertex;
    }
    template <nindexed A> int count(const A& sequence) const {
        int vertex = find(sequence);
        return vertex == npos ? 0 : nodes_[vertex].terminal;
    }
    template <nindexed A> int count_prefix(const A& prefix) const {
        int vertex = find(prefix);
        return vertex == npos ? 0 : nodes_[vertex].passing;
    }
    int parent(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return nodes_[vertex].parent;
    }
    int symbol(int vertex) const {
        npre(0 <= vertex && vertex < len());
        return nodes_[vertex].symbol;
    }
};

template <int Alphabet>
    requires(Alphabet > 0)
class nac {
    struct node {
        array<int, Alphabet> next;
        int failure = 0, output = npos;
        nvector<int> patterns;
        node() { next.fill(npos); }
    };
    nvector<node> nodes_{node{}};
    int patterns_ = 0;
    bool built_ = false;

  public:
    int len() const noexcept { return nodes_.len(); }
    int patterns() const noexcept { return patterns_; }

    template <nindexed A> int add(const A& pattern) {
        npre(!built_ && nlen(pattern) > 0);
        int vertex = 0;
        for (int i = 0; i < nlen(pattern); ++i) {
            int symbol = int(pattern[i]);
            npre(0 <= symbol && symbol < Alphabet);
            int next = nodes_[vertex].next[symbol];
            if (next == npos) {
                next = nodes_.len();
                nodes_[vertex].next[symbol] = next;
                nodes_.push();
            }
            vertex = next;
        }
        nodes_[vertex].patterns.push(patterns_);
        return patterns_++;
    }

    void build() {
        npre(!built_);
        built_ = true;
        queue<int> queue;
        for (int symbol = 0; symbol < Alphabet; ++symbol) {
            int child = nodes_[0].next[symbol];
            if (child == npos)
                nodes_[0].next[symbol] = 0;
            else {
                nodes_[child].failure = 0;
                queue.push(child);
            }
        }
        while (!queue.empty()) {
            int vertex = queue.front();
            queue.pop();
            int failure = nodes_[vertex].failure;
            nodes_[vertex].output = !nodes_[failure].patterns.empty() ? failure : nodes_[failure].output;
            for (int symbol = 0; symbol < Alphabet; ++symbol) {
                int child = nodes_[vertex].next[symbol];
                if (child == npos)
                    nodes_[vertex].next[symbol] = nodes_[failure].next[symbol];
                else {
                    nodes_[child].failure = nodes_[failure].next[symbol];
                    queue.push(child);
                }
            }
        }
    }

    int step(int state, int symbol) const {
        npre(built_ && 0 <= state && state < len() && 0 <= symbol && symbol < Alphabet);
        return nodes_[state].next[symbol];
    }

    // Callback receives the inclusive text position and the pattern id.
    template <nindexed A, class F> void match(const A& text, F callback) const {
        npre(built_);
        int state = 0;
        for (int position = 0; position < nlen(text); ++position) {
            int symbol = int(text[position]);
            npre(0 <= symbol && symbol < Alphabet);
            state = nodes_[state].next[symbol];
            for (int vertex = state; vertex != npos; vertex = nodes_[vertex].output)
                for (int i = 0; i < nodes_[vertex].patterns.len(); ++i)
                    invoke(callback, position, nodes_[vertex].patterns[i]);
        }
    }

    template <nindexed A> long long count(const A& text) const {
        long long result = 0;
        match(text, [&](int, int) { ++result; });
        return result;
    }
};

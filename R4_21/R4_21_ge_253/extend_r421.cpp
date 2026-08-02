#include <cadical.hpp>

#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int MAX_N = 256;
constexpr int WORDS = 4;
using Bits = std::array<std::uint64_t, WORDS>;

struct Graph {
  int n = 0;
  std::vector<unsigned char> a;
  std::vector<Bits> complement;
};

Graph read_graph(const std::string& path) {
  std::ifstream input(path);
  if (!input) throw std::runtime_error("cannot open matrix: " + path);
  std::string text((std::istreambuf_iterator<char>(input)), {});
  std::vector<unsigned char> raw;
  for (char c : text) {
    if (c == '0' || c == '1') raw.push_back(static_cast<unsigned char>(c - '0'));
  }
  int n = std::lround(std::sqrt(static_cast<double>(raw.size())));
  if (n <= 0 || n >= MAX_N || static_cast<std::size_t>(n) * n != raw.size())
    throw std::runtime_error("matrix is not square or is too large");

  Graph g;
  g.n = n;
  g.a = std::move(raw);
  g.complement.resize(n);
  for (int i = 0; i < n; ++i) {
    if (g.a[static_cast<std::size_t>(i) * n + i])
      throw std::runtime_error("matrix diagonal is nonzero");
    for (int j = i + 1; j < n; ++j) {
      if (g.a[static_cast<std::size_t>(i) * n + j] !=
          g.a[static_cast<std::size_t>(j) * n + i])
        throw std::runtime_error("matrix is not symmetric");
      if (!g.a[static_cast<std::size_t>(i) * n + j]) {
        g.complement[i][j >> 6] |= std::uint64_t{1} << (j & 63);
        g.complement[j][i >> 6] |= std::uint64_t{1} << (i & 63);
      }
    }
  }
  return g;
}

inline bool empty(const Bits& x) {
  return !(x[0] | x[1] | x[2] | x[3]);
}

inline int popcount(const Bits& x) {
  return std::popcount(x[0]) + std::popcount(x[1]) +
         std::popcount(x[2]) + std::popcount(x[3]);
}

inline int take_first(Bits& x) {
  for (int w = 0; w < WORDS; ++w) {
    if (!x[w]) continue;
    int b = std::countr_zero(x[w]);
    x[w] &= x[w] - 1;
    return 64 * w + b;
  }
  return -1;
}

inline void erase(Bits& x, int v) {
  x[v >> 6] &= ~(std::uint64_t{1} << (v & 63));
}

inline Bits intersection(const Bits& x, const Bits& y) {
  return {x[0] & y[0], x[1] & y[1], x[2] & y[2], x[3] & y[3]};
}

// Target-clique search with the standard greedy-coloring upper bound.
// It is run on the complement graph, so a target clique is an independent
// set in the original graph.
class TargetClique {
 public:
  explicit TargetClique(const std::vector<Bits>& adjacency)
      : adjacency_(adjacency) {}

  bool find(Bits candidates, int target, std::vector<int>& witness) {
    target_ = target;
    nodes_ = 0;
    current_.clear();
    witness.clear();
    return expand(candidates, witness);
  }

  std::uint64_t enumerate(Bits candidates, int target,
                          std::vector<std::vector<int>>& witnesses) {
    target_ = target;
    nodes_ = 0;
    current_.clear();
    witnesses.clear();
    enumerate_expand(candidates, witnesses);
    return nodes_;
  }

  std::uint64_t nodes() const { return nodes_; }

 private:
  void enumerate_expand(Bits candidates,
                        std::vector<std::vector<int>>& witnesses) {
    ++nodes_;
    if (static_cast<int>(current_.size()) + popcount(candidates) < target_)
      return;
    std::vector<int> order;
    std::vector<int> colors;
    color_sort(candidates, order, colors);
    for (int i = static_cast<int>(order.size()) - 1; i >= 0; --i) {
      if (static_cast<int>(current_.size()) + colors[i] < target_) return;
      int v = order[i];
      current_.push_back(v);
      if (static_cast<int>(current_.size()) == target_) {
        witnesses.push_back(current_);
      } else {
        Bits next = intersection(candidates, adjacency_[v]);
        if (!empty(next)) enumerate_expand(next, witnesses);
      }
      current_.pop_back();
      erase(candidates, v);
    }
  }

  bool expand(Bits candidates, std::vector<int>& witness) {
    ++nodes_;
    if (static_cast<int>(current_.size()) + popcount(candidates) < target_)
      return false;

    std::vector<int> order;
    std::vector<int> colors;
    color_sort(candidates, order, colors);
    for (int i = static_cast<int>(order.size()) - 1; i >= 0; --i) {
      if (static_cast<int>(current_.size()) + colors[i] < target_)
        return false;
      int v = order[i];
      current_.push_back(v);
      if (static_cast<int>(current_.size()) == target_) {
        witness = current_;
        return true;
      }
      Bits next = intersection(candidates, adjacency_[v]);
      if (!empty(next) && expand(next, witness)) return true;
      current_.pop_back();
      erase(candidates, v);
    }
    return false;
  }

  void color_sort(Bits candidates, std::vector<int>& order,
                  std::vector<int>& colors) const {
    int color = 0;
    Bits uncolored = candidates;
    order.reserve(popcount(candidates));
    colors.reserve(order.capacity());
    while (!empty(uncolored)) {
      ++color;
      Bits available = uncolored;
      while (!empty(available)) {
        int v = take_first(available);
        order.push_back(v);
        colors.push_back(color);
        erase(uncolored, v);
        for (int w = 0; w < WORDS; ++w)
          available[w] &= ~adjacency_[v][w];
      }
    }
  }

  const std::vector<Bits>& adjacency_;
  int target_ = 0;
  std::uint64_t nodes_ = 0;
  std::vector<int> current_;
};

void add_clause(CaDiCaL::Solver& solver, const std::vector<int>& clause) {
  for (int literal : clause) solver.add(literal);
  solver.add(0);
}

void write_extension(const Graph& g, const std::vector<unsigned char>& neighbor,
                     const std::string& path) {
  std::ofstream out(path);
  if (!out) throw std::runtime_error("cannot write matrix: " + path);
  int n2 = g.n + 1;
  for (int i = 0; i < n2; ++i) {
    for (int j = 0; j < n2; ++j) {
      int value = 0;
      if (i < g.n && j < g.n)
        value = g.a[static_cast<std::size_t>(i) * g.n + j];
      else if (i == g.n && j < g.n)
        value = neighbor[j];
      else if (j == g.n && i < g.n)
        value = neighbor[i];
      out << value << (j + 1 == n2 ? '\n' : ' ');
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3 || argc > 6) {
    std::cerr << "usage: extend_r421 BASE_MATRIX OUTPUT_MATRIX [SEED] "
                 "[TEMPLATE_VERTEX] [ENUMERATE_ALL]\n";
    return 2;
  }
  try {
    Graph g = read_graph(argv[1]);
    int seed = argc >= 4 ? std::stoi(argv[3]) : 0;
    int template_vertex = argc >= 5 ? std::stoi(argv[4]) : -1;
    bool enumerate_all = argc == 6 && std::stoi(argv[5]);
    if (template_vertex >= g.n)
      throw std::runtime_error("template vertex is outside the graph");
    CaDiCaL::Solver solver;
    solver.set("quiet", 1);
    if (seed) solver.set("seed", seed);
    for (int i = 0; i < g.n; ++i) {
      bool preferred = template_vertex >= 0 &&
          g.a[static_cast<std::size_t>(template_vertex) * g.n + i];
      solver.phase(preferred ? i + 1 : -(i + 1));
    }

    std::uint64_t triangles = 0;
    for (int i = 0; i < g.n; ++i) {
      for (int j = i + 1; j < g.n; ++j) {
        if (!g.a[static_cast<std::size_t>(i) * g.n + j]) continue;
        for (int k = j + 1; k < g.n; ++k) {
          if (g.a[static_cast<std::size_t>(i) * g.n + k] &&
              g.a[static_cast<std::size_t>(j) * g.n + k]) {
            add_clause(solver, {-(i + 1), -(j + 1), -(k + 1)});
            ++triangles;
          }
        }
      }
    }
    std::cerr << "base_order=" << g.n << " triangle_clauses=" << triangles
              << " seed=" << seed << " template=" << template_vertex
              << " enumerate_all=" << enumerate_all << '\n';

    TargetClique finder(g.complement);
    auto start = std::chrono::steady_clock::now();
    if (enumerate_all) {
      Bits all{};
      for (int i = 0; i < g.n; ++i)
        all[i >> 6] |= std::uint64_t{1} << (i & 63);
      std::vector<std::vector<int>> independent20s;
      std::uint64_t enumeration_nodes = finder.enumerate(all, 20, independent20s);
      for (const auto& set : independent20s) {
        std::vector<int> clause;
        clause.reserve(set.size());
        for (int v : set) clause.push_back(v + 1);
        add_clause(solver, clause);
      }
      auto seconds = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - start).count();
      std::cerr << "independent20_clauses=" << independent20s.size()
                << " enumeration_nodes=" << enumeration_nodes
                << " enumeration_seconds=" << seconds << '\n';
      std::string cnf_path = std::string(argv[2]) + ".cnf";
      const char* write_error = solver.write_dimacs(cnf_path.c_str());
      if (write_error) throw std::runtime_error(write_error);
      std::cerr << "wrote_cnf=" << cnf_path << '\n';
    }
    std::uint64_t total_clique_nodes = 0;
    for (std::uint64_t cuts = 0;; ++cuts) {
      int result = solver.solve();
      if (result == 20) {
        std::cerr << "UNSAT: base has no one-vertex (4,21) extension; cuts="
                  << cuts << '\n';
        return 1;
      }
      if (result != 10) {
        std::cerr << "UNKNOWN from SAT solver; cuts=" << cuts << '\n';
        return 3;
      }

      std::vector<unsigned char> neighbor(g.n);
      Bits nonneighbors{};
      int degree = 0;
      for (int i = 0; i < g.n; ++i) {
        neighbor[i] = solver.val(i + 1) > 0;
        degree += neighbor[i];
        if (!neighbor[i])
          nonneighbors[i >> 6] |= std::uint64_t{1} << (i & 63);
      }

      std::vector<int> independent20;
      bool found = finder.find(nonneighbors, 20, independent20);
      total_clique_nodes += finder.nodes();
      if (!found) {
        write_extension(g, neighbor, argv[2]);
        auto seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        std::cout << "SAT_EXTENSION order=" << (g.n + 1)
                  << " degree=" << degree << " cuts=" << cuts
                  << " clique_nodes=" << total_clique_nodes
                  << " seconds=" << seconds << " output=" << argv[2]
                  << '\n';
        std::cout << "neighbors";
        for (int i = 0; i < g.n; ++i) if (neighbor[i]) std::cout << ' ' << i;
        std::cout << '\n';
        return 0;
      }

      std::vector<int> clause;
      clause.reserve(independent20.size());
      for (int v : independent20) clause.push_back(v + 1);
      add_clause(solver, clause);
      if (cuts < 10 || (cuts + 1) % 25 == 0) {
        auto seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        std::cerr << "cut=" << (cuts + 1) << " degree=" << degree
                  << " last_clique_nodes=" << finder.nodes()
                  << " total_clique_nodes=" << total_clique_nodes
                  << " seconds=" << seconds << '\n';
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
  }
}

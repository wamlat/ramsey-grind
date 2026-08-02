#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Clause {
  std::vector<int> lits;
  int true_count = 0;
  int weight = 1;
  int unsat_pos = -1;
};

struct Formula {
  int nvars = 0;
  std::vector<Clause> clauses;
  std::vector<std::vector<int>> occurrence;
};

Formula read_cnf(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open CNF: " + path);
  Formula f;
  std::string line;
  int declared_clauses = -1;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == 'c') continue;
    std::istringstream input(line);
    if (line[0] == 'p') {
      std::string p, cnf;
      input >> p >> cnf >> f.nvars >> declared_clauses;
      if (p != "p" || cnf != "cnf" || f.nvars <= 0)
        throw std::runtime_error("bad DIMACS header");
      continue;
    }
    if (f.nvars <= 0) throw std::runtime_error("clause before header");
    Clause clause;
    int lit;
    while (input >> lit && lit) {
      if (std::abs(lit) > f.nvars)
        throw std::runtime_error("literal exceeds variable count");
      clause.lits.push_back(lit);
    }
    if (clause.lits.empty()) throw std::runtime_error("empty clause");
    f.clauses.push_back(std::move(clause));
  }
  if (declared_clauses != static_cast<int>(f.clauses.size()))
    throw std::runtime_error("DIMACS clause count mismatch");
  f.occurrence.resize(f.nvars);
  for (int c = 0; c < static_cast<int>(f.clauses.size()); ++c)
    for (int lit : f.clauses[c].lits)
      f.occurrence[std::abs(lit) - 1].push_back(c);
  return f;
}

std::vector<std::vector<unsigned char>> read_matrix(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open matrix: " + path);
  std::vector<unsigned char> raw;
  char ch;
  while (in.get(ch))
    if (ch == '0' || ch == '1') raw.push_back(ch - '0');
  int n = std::lround(std::sqrt(static_cast<double>(raw.size())));
  if (n <= 0 || static_cast<std::size_t>(n) * n != raw.size())
    throw std::runtime_error("matrix is not square");
  std::vector<std::vector<unsigned char>> rows(
      n, std::vector<unsigned char>(n));
  for (int i = 0; i < n; ++i)
    std::copy_n(raw.begin() + static_cast<std::size_t>(i) * n, n,
                rows[i].begin());
  return rows;
}

std::vector<unsigned char> read_assignment(const std::string& path, int nvars) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open assignment: " + path);
  std::vector<unsigned char> value(nvars);
  std::vector<unsigned char> seen(nvars);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == 'c' || line[0] == 's') continue;
    std::istringstream input(line);
    std::string token;
    while (input >> token) {
      if (token == "v") continue;
      char* end = nullptr;
      long lit = std::strtol(token.c_str(), &end, 10);
      if (*token.c_str() && !*end && lit && std::abs(lit) <= nvars) {
        value[std::abs(lit) - 1] = lit > 0;
        seen[std::abs(lit) - 1] = 1;
      }
    }
  }
  if (std::count(seen.begin(), seen.end(), 1) != nvars)
    throw std::runtime_error("assignment misses an original variable");
  return value;
}

bool lit_true(int lit, const std::vector<unsigned char>& value) {
  bool v = value[std::abs(lit) - 1];
  return lit > 0 ? v : !v;
}

class Search {
 public:
  Search(Formula formula, std::vector<std::vector<unsigned char>> matrix,
         std::uint64_t seed, double seconds, int target_degree,
         const std::string& output, int noise_percent, int bump_interval,
         int tabu_tenure)
      : f_(std::move(formula)), matrix_(std::move(matrix)), rng_(seed),
        seconds_(seconds), target_degree_(target_degree), output_(output),
        noise_percent_(noise_percent), bump_interval_(bump_interval),
        tabu_tenure_(tabu_tenure),
        value_(f_.nvars), score_(f_.nvars), last_flip_(f_.nvars),
        best_value_(f_.nvars) {}

  bool run(int restarts, std::uint64_t flips_per_restart, int template_vertex,
           double perturb_or_density) {
    start_ = std::chrono::steady_clock::now();
    best_unsat_ = std::numeric_limits<int>::max();
    for (int restart = 0; restart < restarts && elapsed() < seconds_; ++restart) {
      initialize(restart, template_vertex, perturb_or_density);
      rebuild();
      report_if_best(restart, 0);
      std::uint64_t stagnation = 0;
      int local_best = static_cast<int>(unsat_.size());
      for (std::uint64_t flip = 1;
           flip <= flips_per_restart && elapsed() < seconds_; ++flip) {
        if (unsat_.empty()) {
          write_assignment(value_, "SATISFIABLE");
          std::cout << "SAT seed_solution=" << output_ << " degree=" << degree_
                    << " restart=" << restart << " flips=" << flip - 1
                    << " seconds=" << elapsed() << '\n';
          return true;
        }
        int var = choose_variable(flip);
        do_flip(var, flip);
        int now = static_cast<int>(unsat_.size());
        if (now < local_best) {
          local_best = now;
          stagnation = 0;
        } else {
          ++stagnation;
        }
        report_if_best(restart, flip);
        if (stagnation >= static_cast<std::uint64_t>(bump_interval_)) {
          bump_unsatisfied_weights();
          stagnation = 0;
        }
      }
    }
    write_assignment(best_value_, "UNKNOWN");
    std::cout << "NO_SOLUTION best_unsat=" << best_unsat_
              << " best_degree=" << best_degree_ << " seconds=" << elapsed()
              << '\n';
    return false;
  }

 private:
  double elapsed() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_)
        .count();
  }

  void initialize(int restart, int template_vertex, double q) {
    std::fill(last_flip_.begin(), last_flip_.end(), 0);
    for (Clause& c : f_.clauses) c.weight = 1;
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    if (template_vertex >= 0) {
      int t = (template_vertex + restart * 73) % static_cast<int>(matrix_.size());
      for (int v = 0; v < f_.nvars; ++v) {
        value_[v] = matrix_[t][v];
        if (unit(rng_) < q) value_[v] ^= 1;
      }
    } else {
      // Cycle through densities around the successful insertion degrees.
      static const double densities[] = {0.15, 0.17, 0.19, 0.21, 0.23};
      double density = q > 0 ? q : densities[restart % 5];
      for (int v = 0; v < f_.nvars; ++v) value_[v] = unit(rng_) < density;
    }
  }

  int sole_true_variable(const Clause& c) const {
    for (int lit : c.lits)
      if (lit_true(lit, value_)) return std::abs(lit) - 1;
    throw std::runtime_error("true_count said one but no literal is true");
  }

  void add_unsat(int c) {
    Clause& clause = f_.clauses[c];
    if (clause.unsat_pos >= 0) return;
    clause.unsat_pos = static_cast<int>(unsat_.size());
    unsat_.push_back(c);
  }

  void remove_unsat(int c) {
    Clause& clause = f_.clauses[c];
    int pos = clause.unsat_pos;
    if (pos < 0) return;
    int tail = unsat_.back();
    unsat_[pos] = tail;
    f_.clauses[tail].unsat_pos = pos;
    unsat_.pop_back();
    clause.unsat_pos = -1;
  }

  void remove_score_contribution(const Clause& c) {
    if (c.true_count == 0) {
      for (int lit : c.lits) score_[std::abs(lit) - 1] -= c.weight;
    } else if (c.true_count == 1) {
      score_[sole_true_variable(c)] += c.weight;
    }
  }

  void add_score_contribution(const Clause& c) {
    if (c.true_count == 0) {
      for (int lit : c.lits) score_[std::abs(lit) - 1] += c.weight;
    } else if (c.true_count == 1) {
      score_[sole_true_variable(c)] -= c.weight;
    }
  }

  void rebuild() {
    unsat_.clear();
    std::fill(score_.begin(), score_.end(), 0);
    degree_ = std::count(value_.begin(), value_.end(), 1);
    for (int ci = 0; ci < static_cast<int>(f_.clauses.size()); ++ci) {
      Clause& c = f_.clauses[ci];
      c.true_count = 0;
      c.unsat_pos = -1;
      for (int lit : c.lits) c.true_count += lit_true(lit, value_);
      if (!c.true_count) add_unsat(ci);
    }
    for (const Clause& c : f_.clauses) add_score_contribution(c);
  }

  int choose_variable(std::uint64_t flip) {
    std::uniform_int_distribution<int> pick_clause(0, unsat_.size() - 1);
    const Clause& c = f_.clauses[unsat_[pick_clause(rng_)]];
    std::uniform_int_distribution<int> pick_lit(0, c.lits.size() - 1);
    // A small random-walk component prevents deterministic short cycles.
    if ((rng_() % 100) < static_cast<std::uint64_t>(noise_percent_))
      return std::abs(c.lits[pick_lit(rng_)]) - 1;

    long long best = std::numeric_limits<long long>::min();
    std::vector<int> candidates;
    for (int lit : c.lits) {
      int v = std::abs(lit) - 1;
      bool tabu = flip > last_flip_[v] &&
                  flip - last_flip_[v] <= static_cast<std::uint64_t>(tabu_tenure_);
      int new_degree = degree_ + (value_[v] ? -1 : 1);
      long long degree_tie = -std::abs(new_degree - target_degree_);
      long long quality = static_cast<long long>(score_[v]) * 1024 + degree_tie;
      if (tabu && static_cast<int>(unsat_.size()) - score_[v] >= best_unsat_)
        quality -= (1LL << 40);
      if (quality > best) {
        best = quality;
        candidates.clear();
        candidates.push_back(v);
      } else if (quality == best) {
        candidates.push_back(v);
      }
    }
    return candidates[rng_() % candidates.size()];
  }

  void do_flip(int var, std::uint64_t flip) {
    const bool old_value = value_[var];
    for (int ci : f_.occurrence[var])
      remove_score_contribution(f_.clauses[ci]);
    value_[var] ^= 1;
    degree_ += old_value ? -1 : 1;
    for (int ci : f_.occurrence[var]) {
      Clause& c = f_.clauses[ci];
      int old_count = c.true_count;
      int lit = 0;
      for (int candidate : c.lits)
        if (std::abs(candidate) - 1 == var) { lit = candidate; break; }
      bool was_true = lit > 0 ? old_value : !old_value;
      c.true_count += was_true ? -1 : 1;
      if (old_count == 0 && c.true_count > 0) remove_unsat(ci);
      if (old_count > 0 && c.true_count == 0) add_unsat(ci);
    }
    for (int ci : f_.occurrence[var]) add_score_contribution(f_.clauses[ci]);
    last_flip_[var] = flip;
  }

  void bump_unsatisfied_weights() {
    for (int ci : unsat_) {
      Clause& c = f_.clauses[ci];
      ++c.weight;
      for (int lit : c.lits) ++score_[std::abs(lit) - 1];
    }
  }

  void report_if_best(int restart, std::uint64_t flip) {
    int now = static_cast<int>(unsat_.size());
    if (now >= best_unsat_) return;
    best_unsat_ = now;
    best_degree_ = degree_;
    best_value_ = value_;
    if (now <= 20) write_assignment(best_value_, "UNKNOWN");
    if (now <= 50 || flip == 0 || now % 25 == 0) {
      std::cerr << "best=" << now << " degree=" << degree_
                << " restart=" << restart << " flip=" << flip
                << " seconds=" << std::fixed << std::setprecision(2)
                << elapsed() << '\n';
    }
  }

  void write_assignment(const std::vector<unsigned char>& assignment,
                        const char* status) const {
    std::ofstream out(output_);
    if (!out) throw std::runtime_error("cannot write solution: " + output_);
    out << "c best local-search assignment; verify against the CNF\n";
    out << "s " << status << "\n";
    out << "v";
    for (int v = 0; v < f_.nvars; ++v)
      out << ' ' << (assignment[v] ? v + 1 : -(v + 1));
    out << " 0\n";
  }

  Formula f_;
  std::vector<std::vector<unsigned char>> matrix_;
  std::mt19937_64 rng_;
  double seconds_;
  int target_degree_;
  std::string output_;
  int noise_percent_;
  int bump_interval_;
  int tabu_tenure_;
  std::vector<unsigned char> value_;
  std::vector<long long> score_;
  std::vector<std::uint64_t> last_flip_;
  std::vector<unsigned char> best_value_;
  std::vector<int> unsat_;
  int degree_ = 0;
  int best_unsat_ = std::numeric_limits<int>::max();
  int best_degree_ = 0;
  std::chrono::steady_clock::time_point start_;
};

}  // namespace

int main(int argc, char** argv) {
  if (argc != 11 && argc != 14) {
    std::cerr << "usage: walksat_extension CNF MATRIX OUTPUT_SOL SEED SECONDS "
                 "RESTARTS FLIPS_PER_RESTART TEMPLATE_VERTEX "
                 "PERTURB_OR_DENSITY TARGET_DEGREE "
                 "[NOISE_PERCENT BUMP_INTERVAL TABU_TENURE]\n"
                 "  TEMPLATE_VERTEX=-1 uses random assignments at the given "
                 "density; otherwise it perturbs that matrix row.\n";
    return 2;
  }
  try {
    Formula f = read_cnf(argv[1]);
    int template_vertex = std::stoi(argv[8]);
    std::vector<std::vector<unsigned char>> matrix;
    if (template_vertex == -2) {
      matrix.push_back(read_assignment(argv[2], f.nvars));
      template_vertex = 0;
    } else if (template_vertex >= 0) {
      matrix = read_matrix(argv[2]);
    }
    bool assignment_template =
        matrix.size() == 1 && static_cast<int>(matrix[0].size()) == f.nvars;
    if (template_vertex >= 0 && !assignment_template &&
        f.nvars != static_cast<int>(matrix.size()))
      throw std::runtime_error(
          "matrix templates require CNF variable count to equal matrix order");
    int noise_percent = argc == 14 ? std::stoi(argv[11]) : 12;
    int bump_interval = argc == 14 ? std::stoi(argv[12]) : 2000;
    int tabu_tenure = argc == 14 ? std::stoi(argv[13]) : 7;
    if (noise_percent < 0 || noise_percent > 100 || bump_interval <= 0 ||
        tabu_tenure < 0)
      throw std::runtime_error("invalid local-search tuning parameter");
    Search search(std::move(f), std::move(matrix), std::stoull(argv[4]),
                  std::stod(argv[5]), std::stoi(argv[10]), argv[3],
                  noise_percent, bump_interval, tabu_tenure);
    bool sat = search.run(std::stoi(argv[6]), std::stoull(argv[7]),
                          template_vertex, std::stod(argv[9]));
    return sat ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
  }
}

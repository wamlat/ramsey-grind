#include <cadical.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

void add_clause(CaDiCaL::Solver& solver, std::initializer_list<int> literals) {
  for (int literal : literals) solver.add(literal);
  solver.add(0);
}

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "usage: independent_set_transversal EXTENSION_CNF MAX_DELETIONS OUTPUT\n";
    return 2;
  }
  try {
    int max_deletions = std::stoi(argv[2]);
    std::ifstream input(argv[1]);
    if (!input) throw std::runtime_error("cannot open extension CNF");
    std::string line;
    int n = 0;
    std::vector<std::vector<int>> independent_sets;
    while (std::getline(input, line)) {
      if (line.empty() || line[0] == 'c') continue;
      if (line[0] == 'p') {
        std::istringstream header(line);
        std::string p, cnf;
        int clauses;
        header >> p >> cnf >> n >> clauses;
        continue;
      }
      std::istringstream clause_stream(line);
      std::vector<int> clause;
      int literal;
      bool all_positive = true;
      while (clause_stream >> literal && literal) {
        clause.push_back(literal);
        if (literal < 0) all_positive = false;
      }
      if (all_positive && clause.size() == 20)
        independent_sets.push_back(std::move(clause));
    }
    if (n <= 0 || independent_sets.empty())
      throw std::runtime_error("no independent-set clauses found");

    CaDiCaL::Solver solver;
    solver.set("quiet", 1);
    for (const auto& set : independent_sets) {
      for (int vertex : set) solver.add(vertex);
      solver.add(0);
    }

    // Forward sequential counter.  s[i][j] is forced true whenever at least
    // j of deletion variables 1..i are true.  Asserting !s[n][K+1] imposes
    // an at-most-K cardinality bound.
    int next_variable = n + 1;
    std::vector<std::vector<int>> s(n + 1,
        std::vector<int>(max_deletions + 2));
    for (int i = 1; i <= n; ++i)
      for (int j = 1; j <= max_deletions + 1 && j <= i; ++j)
        s[i][j] = next_variable++;

    for (int i = 1; i <= n; ++i) {
      add_clause(solver, {-i, s[i][1]});
      if (i == 1) continue;
      for (int j = 1; j <= max_deletions + 1 && j <= i - 1; ++j)
        add_clause(solver, {-s[i - 1][j], s[i][j]});
      for (int j = 2; j <= max_deletions + 1 && j <= i; ++j) {
        if (s[i - 1][j - 1])
          add_clause(solver, {-i, -s[i - 1][j - 1], s[i][j]});
      }
    }
    solver.add(-s[n][max_deletions + 1]);
    solver.add(0);

    std::string cnf_path = std::string(argv[3]) + ".cnf";
    const char* write_error = solver.write_dimacs(cnf_path.c_str());
    if (write_error) throw std::runtime_error(write_error);
    std::cerr << "sets=" << independent_sets.size() << " wrote_cnf="
              << cnf_path << '\n';

    int result = solver.solve();
    if (result == 20) {
      std::cout << "UNSAT no transversal of size <= " << max_deletions
                << " for " << independent_sets.size() << " independent 20-sets\n";
      return 1;
    }
    if (result != 10) {
      std::cout << "UNKNOWN\n";
      return 3;
    }
    std::ofstream out(argv[3]);
    int deleted = 0;
    std::cout << "SAT transversal";
    for (int i = 1; i <= n; ++i) {
      if (solver.val(i) > 0) {
        ++deleted;
        out << (i - 1) << '\n';
        std::cout << ' ' << (i - 1);
      }
    }
    std::cout << "\nsize=" << deleted << " sets=" << independent_sets.size()
              << " order_after_deletion=" << (n - deleted) << '\n';
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
  }
}

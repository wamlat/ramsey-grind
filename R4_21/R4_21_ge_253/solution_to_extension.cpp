#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "usage: solution_to_extension BASE_MATRIX SAT_SOLUTION OUTPUT_MATRIX\n";
    return 2;
  }
  try {
    std::ifstream matrix_in(argv[1]);
    std::string matrix_text((std::istreambuf_iterator<char>(matrix_in)), {});
    std::vector<unsigned char> a;
    for (char c : matrix_text) if (c == '0' || c == '1') a.push_back(c - '0');
    int n = std::lround(std::sqrt(static_cast<double>(a.size())));
    if (n <= 0 || static_cast<std::size_t>(n) * n != a.size())
      throw std::runtime_error("bad base matrix");

    std::ifstream solution_in(argv[2]);
    std::string token;
    std::vector<unsigned char> neighbor(n);
    std::vector<unsigned char> seen(n);
    bool satisfiable = false;
    while (solution_in >> token) {
      if (token == "SATISFIABLE") {
        satisfiable = true;
      } else if (token == "s" || token == "v") {
        continue;
      } else {
        int literal = std::stoi(token);
        if (!literal) continue;
        int variable = std::abs(literal);
        if (variable < 1 || variable > n)
          throw std::runtime_error("solution variable outside base order");
        if (seen[variable - 1]) throw std::runtime_error("duplicate solution variable");
        seen[variable - 1] = 1;
        neighbor[variable - 1] = literal > 0;
      }
    }
    if (!satisfiable) throw std::runtime_error("solution is not marked SATISFIABLE");
    for (int i = 0; i < n; ++i) if (!seen[i])
      throw std::runtime_error("solution omits a variable");

    std::ofstream out(argv[3]);
    int n2 = n + 1;
    for (int i = 0; i < n2; ++i) {
      for (int j = 0; j < n2; ++j) {
        int value = 0;
        if (i < n && j < n) value = a[static_cast<std::size_t>(i) * n + j];
        else if (i == n && j < n) value = neighbor[j];
        else if (j == n && i < n) value = neighbor[i];
        out << value << (j + 1 == n2 ? '\n' : ' ');
      }
    }
    int degree = 0;
    for (unsigned char x : neighbor) degree += x;
    std::cout << "wrote order=" << n2 << " degree=" << degree << '\n';
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
  }
}

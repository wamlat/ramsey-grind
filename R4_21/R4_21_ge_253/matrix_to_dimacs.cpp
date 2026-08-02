#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  if (argc != 4 || (std::string(argv[2]) != "graph" &&
                    std::string(argv[2]) != "complement")) {
    std::cerr << "usage: matrix_to_dimacs MATRIX graph|complement OUTPUT\n";
    return 2;
  }
  try {
    std::ifstream input(argv[1]);
    std::string text((std::istreambuf_iterator<char>(input)), {});
    std::vector<unsigned char> a;
    for (char c : text) if (c == '0' || c == '1') a.push_back(c - '0');
    int n = std::lround(std::sqrt(static_cast<double>(a.size())));
    if (n <= 0 || static_cast<std::size_t>(n) * n != a.size())
      throw std::runtime_error("bad matrix size");
    bool complement = std::string(argv[2]) == "complement";
    std::uint64_t edges = 0;
    for (int i = 0; i < n; ++i) {
      if (a[static_cast<std::size_t>(i) * n + i])
        throw std::runtime_error("nonzero diagonal");
      for (int j = i + 1; j < n; ++j) {
        if (a[static_cast<std::size_t>(i) * n + j] !=
            a[static_cast<std::size_t>(j) * n + i])
          throw std::runtime_error("nonsymmetric matrix");
        if (static_cast<bool>(a[static_cast<std::size_t>(i) * n + j]) != complement)
          ++edges;
      }
    }
    std::ofstream out(argv[3]);
    std::string output_path = argv[3];
    bool matrix_market = output_path.size() >= 4 &&
        output_path.substr(output_path.size() - 4) == ".mtx";
    bool open_mcs_edges = output_path.size() >= 6 &&
        output_path.substr(output_path.size() - 6) == ".edges";
    if (open_mcs_edges) {
      out << n << '\n' << (2 * edges) << '\n';
    } else if (matrix_market) {
      out << "%%MatrixMarket matrix coordinate pattern symmetric\n";
      out << n << ' ' << n << ' ' << edges << '\n';
    } else {
      out << "p edge " << n << ' ' << edges << '\n';
    }
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        if (static_cast<bool>(a[static_cast<std::size_t>(i) * n + j]) != complement)
          if (open_mcs_edges) {
            out << i << ',' << j << '\n' << j << ',' << i << '\n';
          } else if (matrix_market) {
            out << (j + 1) << ' ' << (i + 1) << '\n';
          } else {
            out << "e " << (i + 1) << ' ' << (j + 1) << '\n';
          }
    std::cout << "wrote vertices=" << n << " edges=" << edges << '\n';
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
  }
}

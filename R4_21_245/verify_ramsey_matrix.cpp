#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

extern "C" int cliquer_find_target(int n, const std::uint64_t* adjacency,
                                    int words, const std::uint64_t* candidate,
                                    int target, int* out);

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "usage: verify_ramsey_matrix MATRIX K L\n";
    return 2;
  }
  int k = std::stoi(argv[2]), l = std::stoi(argv[3]);
  std::ifstream input(argv[1]);
  std::string text((std::istreambuf_iterator<char>(input)), {});
  std::vector<unsigned char> raw;
  for (char c : text) if (c == '0' || c == '1') raw.push_back(c - '0');
  int n = std::lround(std::sqrt(static_cast<double>(raw.size())));
  if (n <= 0 || n > 256 || static_cast<std::size_t>(n) * n != raw.size()) {
    std::cerr << "bad matrix size\n";
    return 2;
  }
  constexpr int words = 4;
  std::vector<std::uint64_t> graph(static_cast<std::size_t>(n) * words);
  std::vector<std::uint64_t> complement(static_cast<std::size_t>(n) * words);
  std::array<std::uint64_t, words> all{};
  for (int i = 0; i < n; ++i) all[i >> 6] |= std::uint64_t(1) << (i & 63);
  for (int i = 0; i < n; ++i) {
    if (raw[n * i + i]) return 2;
    for (int j = i + 1; j < n; ++j) {
      if (raw[n * i + j] != raw[n * j + i]) return 2;
      auto& target = raw[n * i + j] ? graph : complement;
      target[static_cast<std::size_t>(i) * words + (j >> 6)] |=
          std::uint64_t(1) << (j & 63);
      target[static_cast<std::size_t>(j) * words + (i >> 6)] |=
          std::uint64_t(1) << (i & 63);
    }
  }
  std::array<int, 256> witness{};
  int red = cliquer_find_target(n, graph.data(), words, all.data(), k,
                                witness.data());
  if (red) {
    std::cout << "FAIL: K_" << k << " found";
    for (int i = 0; i < red; ++i) std::cout << ' ' << witness[i];
    std::cout << '\n';
    return 1;
  }
  int blue = cliquer_find_target(n, complement.data(), words, all.data(), l,
                                 witness.data());
  if (blue) {
    std::cout << "FAIL: independent-" << l << " found";
    for (int i = 0; i < blue; ++i) std::cout << ' ' << witness[i];
    std::cout << '\n';
    return 1;
  }
  std::cout << "PASS: n=" << n << ", no K_" << k
            << " and no independent set of size " << l << "\n";
  return 0;
}

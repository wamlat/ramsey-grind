#include <array>
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
  if (argc != 5) {
    std::cerr << "usage: verify_one_vertex_extension BASE NSET K L\n";
    return 2;
  }
  int k = std::stoi(argv[3]), l = std::stoi(argv[4]);
  if (k != 4) {
    std::cerr << "this compact verifier currently specializes to k=4\n";
    return 2;
  }
  std::ifstream input(argv[1]);
  std::string text((std::istreambuf_iterator<char>(input)), {});
  std::vector<unsigned char> raw;
  for (char c : text) if (c == '0' || c == '1') raw.push_back(c - '0');
  int n = std::lround(std::sqrt(static_cast<double>(raw.size())));
  if (n <= 0 || n > 256 || static_cast<std::size_t>(n) * n != raw.size()) return 2;

  constexpr int words = 4;
  std::vector<std::uint64_t> complement(static_cast<std::size_t>(n) * words);
  std::vector<unsigned char> neighbor(n);
  std::ifstream ns(argv[2]);
  int v;
  while (ns >> v) {
    if (v < 0 || v >= n || neighbor[v]) return 2;
    neighbor[v] = 1;
  }
  for (int i = 0; i < n; ++i) {
    if (raw[n * i + i]) return 2;
    for (int j = i + 1; j < n; ++j) {
      if (raw[n * i + j] != raw[n * j + i]) return 2;
      if (!raw[n * i + j]) {
        complement[static_cast<std::size_t>(i) * words + (j >> 6)] |=
            std::uint64_t(1) << (j & 63);
        complement[static_cast<std::size_t>(j) * words + (i >> 6)] |=
            std::uint64_t(1) << (i & 63);
      }
    }
  }

  // Any newly created K4 would consist of the new vertex and a triangle
  // entirely inside its chosen neighborhood.
  for (int i = 0; i < n; ++i) if (neighbor[i])
    for (int j = i + 1; j < n; ++j) if (neighbor[j] && raw[n * i + j])
      for (int h = j + 1; h < n; ++h)
        if (neighbor[h] && raw[n * i + h] && raw[n * j + h]) {
          std::cout << "FAIL: neighborhood triangle " << i << ' ' << j << ' ' << h << '\n';
          return 1;
        }

  // Any newly created independent l-set consists of the new vertex and an
  // independent (l-1)-set among its nonneighbors.
  std::array<std::uint64_t, words> nonneighbors{};
  for (int i = 0; i < n; ++i)
    if (!neighbor[i]) nonneighbors[i >> 6] |= std::uint64_t(1) << (i & 63);
  std::array<int, 256> witness{};
  int found = cliquer_find_target(n, complement.data(), words,
                                  nonneighbors.data(), l - 1,
                                  witness.data());
  if (found) {
    std::cout << "FAIL: independent-" << (l - 1) << " among nonneighbors";
    for (int i = 0; i < found; ++i) std::cout << ' ' << witness[i];
    std::cout << '\n';
    return 1;
  }
  std::cout << "PASS: triangle-free neighborhood of size ";
  int degree = 0;
  for (auto x : neighbor) degree += x;
  std::cout << degree << "; nonneighbors contain no independent set of size "
            << (l - 1) << '\n';
  return 0;
}

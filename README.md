# Certificate for R(4,21) >= 253

`R4_21_ge_253_matrix.txt` is the adjacency matrix of a simple graph `G` on
252 vertices. Exact maximum-clique computations give

- `omega(G) = 3`, so `G` contains no `K4`; and
- `omega(complement(G)) = 20`, so `G` contains no independent set of size 21.

Therefore this certificate proves

```text
R(4,21) >= 253.
```

The SHA-256 digest of the matrix is

```text
d420e1e494af36b6526235d27f997d8fd1d3be838379a7c05ecf9327db0a834c
```

## Construction

The first 247 vertices are byte-for-byte the graph in
`../older/R4_21_ge_248_matrix.txt`. Five vertices were then added by exact
one-vertex extension.

For a `(4,21)` graph `G`, a candidate neighborhood `N` for a new vertex must
satisfy both of the following exact constraints:

1. Every triangle of `G` has at least one vertex outside `N`, preventing a new
   `K4`.
2. Every independent 20-set of `G` intersects `N`, preventing a new independent
   21-set.

All independent 20-sets were enumerated before each SAT solve. The extension
chain was:

| Base order | Triangles | Independent 20-sets | Output order | New degree at insertion |
|---:|---:|---:|---:|---:|
| 247 | 33,000 | 3,420 | 248 | 35 |
| 248 | 33,100 | 4,789 | 249 | 37 |
| 249 | 33,210 | 6,248 | 250 | 42 |
| 250 | 33,355 | 9,143 | 251 | 40 |
| 251 | 33,487 | 12,100 | 252 | 41 |


## Independent verification

Two unrelated exact maximum-clique implementations checked the complete final
matrix and its complement:

- Parallel Maximum Clique (PMC) returned clique numbers 3 and 20.
- Open-MCS returned clique numbers 3 and 20.

See `R4_21_ge_253_verification.txt` for source commits, build details, commands,
hashes, and output summaries.

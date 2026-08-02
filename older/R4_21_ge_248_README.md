# Certificate for R(4,21) >= 248

`R4_21_ge_248_matrix.txt` is the adjacency matrix of a graph on 247
vertices. Its diagonal is zero and the matrix is symmetric.

Two independent maximum-clique implementations verify

- `omega(G) = 3`, so the graph contains no `K4`; and
- `omega(complement(G)) = 20`, so the graph has no independent set of
  size 21.

Consequently this graph proves `R(4,21) >= 248`.

## Construction

The graph was obtained by repeated exact one-vertex extension. For a fixed
`(4,21)` graph `G`, a new vertex neighborhood `N` must satisfy two types of
constraints:

1. `N` cannot contain a triangle of `G`, or the new vertex would complete a
   `K4`.
2. `N` must meet every independent 20-set of `G`, or that set together with
   the new vertex would be an independent 21-set.

The final extension enumerated all 1,672 independent 20-sets in its
246-vertex base and solved the resulting SAT instance exactly. The preceding
extensions used the same constraints with lazy independent-set separation.

## Verification

The full outputs are in `R4_21_ge_248_verification.txt`. The certificate's
SHA-256 digest is

```text
82e0c28b8e46939e343ad5bcec8950fbaf33d2255404b3408a2b3f7188e5c7bf
```

Any maximum-clique program can independently check the claim by computing
the clique number of the matrix and of its complement.

# Certificate for R(4,21) >= 254

`R4_21_ge_254_matrix.txt` is the adjacency matrix of a simple graph `G` on
253 vertices. Two unrelated exact maximum-clique implementations give

- `omega(G) = 3`, so `G` contains no `K4`; and
- `omega(complement(G)) = 20`, so `G` contains no independent set of size 21.

Consequently,

```text
R(4,21) >= 254.
```

The SHA-256 digest of the matrix is

```text
e768558e142eb176d2eb2a129840c18e5908dc057571a0354922363eae53e996
```

## Construction

Start with the certified 252-vertex graph in
`R4_21_ge_253_base_matrix.txt`. Delete zero-based vertices 244 and 246,
leaving a 250-vertex core `C`. Add three new vertices `a,b,c`, make them a
clique, and SAT-search all 750 incidences between `{a,b,c}` and `C`. The three
core-neighborhood sizes in the satisfying assignment are 41, 39, and 39, so
the final degrees of `a,b,c` are 43, 41, and 41.

For each new vertex `t` and core vertex `v`, Boolean variable `x(t,v)` says
that `tv` is an edge. The complete formula `three_clique_tail.cnf` has 750
variables and 154,498 clauses:

| Constraint family | Count |
|---|---:|
| A tail vertex cannot cover a core triangle | `3 * 33,370 = 100,110` |
| Every core independent 20-set meets each tail neighborhood | `3 * 10,373 = 31,119` |
| A tail edge and a core edge cannot form a `K4` | `3 * 7,673 = 23,019` |
| The three tail vertices and one core vertex cannot form a `K4` | `250` |
| **Total** | **154,498** |

`three_clique_tail.sol` assigns all 750 variables and satisfies every clause.
`verify_cnf_assignment.py` checks this directly. The reconstruction command is

```bash
python3 solution_to_three_clique_tail.py \
  R4_21_ge_253_base_matrix.txt 244 246 three_clique_tail.sol \
  reconstructed_matrix.txt
```

The reconstructed output is byte-for-byte identical to
`R4_21_ge_254_matrix.txt`.

## Independent verification

Parallel Maximum Clique (PMC) and Open-MCS were each run on the complete final
graph and its complement. Both returned clique numbers 3 and 20. See
`R4_21_ge_254_verification.txt` and `verification/` for hashes, commands,
software commits, timings, and output logs.

## Files

- `R4_21_ge_254_matrix.txt`: the 253-vertex certificate.
- `R4_21_ge_253_base_matrix.txt`: the certified 252-vertex starting graph.
- `three_clique_tail.cnf`: complete fixed-core tail formula.
- `three_clique_tail.sol`: satisfying assignment found by weighted local search.
- `make_three_clique_tail.py`: formula generator.
- `make_two_adjacent_tail.py`: shared strict matrix/CNF parser used by the scripts.
- `solution_to_three_clique_tail.py`: strict graph reconstruction.
- `verify_cnf_assignment.py`: strict assignment/CNF checker.
- `walksat_extension.cpp`: incremental weighted local-search implementation.
- `verification/`: raw PMC and Open-MCS logs.

This is a computational certificate, not a peer-reviewed publication.

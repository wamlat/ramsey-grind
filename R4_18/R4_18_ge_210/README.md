# Certificate for R(4,18) >= 210

`R4_18_ge_210_matrix.txt` is the adjacency matrix of a simple graph `G` on
209 vertices. Two independent exact maximum-clique implementations give

- `omega(G) = 3`, so `G` contains no `K4`; and
- `omega(complement(G)) = 17`, so `G` contains no independent set of size 18.

Consequently,

```text
R(4,18) >= 210.
```

The SHA-256 digest of the matrix is

```text
526f170569418326bedfd08d68221440b2c457a4e0eca112580884d990384445
```

## Construction

Start with the public 208-vertex `(4,18)` graph in
`R4_18_ge_209_base_matrix.txt`. Delete zero-based vertices 203 and 207,
leaving a 206-vertex core `C`. Add three vertices `a,b,c`, make them a
clique, and search all 618 incidences between `{a,b,c}` and `C`.

For each new vertex `t` and core vertex `v`, Boolean variable `x(t,v)` says
that `tv` is an edge. The complete formula `three_clique_tail.cnf` has 618
variables and 96,323 clauses:

| Constraint family | Count |
|---|---:|
| A tail vertex cannot cover a core triangle | `3 * 22,766 = 68,298` |
| Every core independent 17-set meets each tail neighborhood | `3 * 3,617 = 10,851` |
| A tail edge and a core edge cannot form a `K4` | `3 * 5,656 = 16,968` |
| The three tail vertices and one core vertex cannot form a `K4` | `206` |
| **Total** | **96,323** |

The three core-neighborhood sizes are 35, 29 and 34. Their final degrees are
37, 31 and 36. The final graph has 5,757 edges.

`three_clique_tail.sol` assigns all 618 variables and satisfies every clause.
`verify_cnf_assignment.py` checks the assignment directly. Reconstruction is

```bash
python3 solution_to_three_clique_tail.py \
  R4_18_ge_209_base_matrix.txt 203 207 three_clique_tail.sol \
  reconstructed_matrix.txt
```

The reconstructed output has the matrix hash shown above.

## Independent verification

Parallel Maximum Clique (PMC) and Open-MCS were each run on the complete
final graph and its complement. Both returned clique numbers 3 and 17. The
`verification/` directory contains the converted graph files and raw logs.

## Files

- `R4_18_ge_210_matrix.txt`: the 209-vertex certificate.
- `R4_18_ge_209_base_matrix.txt`: the 208-vertex starting graph.
- `three_clique_tail.cnf`: complete fixed-core tail formula.
- `three_clique_tail.sol`: satisfying assignment.
- `make_three_clique_tail.py`, `make_two_adjacent_tail.py`: formula generator
  and strict shared parser.
- `solution_to_three_clique_tail.py`: strict graph reconstruction.
- `verify_cnf_assignment.py`: strict assignment/CNF checker.
- `walksat_extension.cpp`: weighted local-search implementation.
- `verification/`: raw exact-checker inputs and logs.

This is a computational certificate, not a peer-reviewed publication.

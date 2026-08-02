# ramsey-grind

Computational certificates and reproducibility artifacts for lower bounds on
off-diagonal Ramsey numbers `R(4,k)`.

## Current certified lower bounds

| Ramsey number | Prev lower bound | New lower bound | `omega(G)` | `omega(complement(G))` | Latest certificate |
|---|---:|---:|---:|---:|---|
| `R(4,18)` | **`>= 209`** | **`>= 210`** | 3 | 17 | [`R4_18_ge_210`](R4_18/R4_18_ge_210/) |
| `R(4,21)` | **`>= 244`** | **`>= 254`** | 3 | 20 | [`R4_21_ge_254`](R4_21/R4_21_ge_254/) |

A graph `G` on `n` vertices with `omega(G) <= 3` and
`omega(complement(G)) <= k-1` contains neither a `K4` nor an independent set
of size `k`; therefore it certifies `R(4,k) >= n+1`.

The strongest witness matrices currently in the repository are:

- [`R4_18_ge_210_matrix.txt`](R4_18/R4_18_ge_210/R4_18_ge_210_matrix.txt),
  SHA-256
  `526f170569418326bedfd08d68221440b2c457a4e0eca112580884d990384445`.
- [`R4_21_ge_254_matrix.txt`](R4_21/R4_21_ge_254/R4_21_ge_254_matrix.txt),
  SHA-256
  `e768558e142eb176d2eb2a129840c18e5908dc057571a0354922363eae53e996`.

Each latest-certificate directory contains the witness, its construction
provenance, a complete SAT instance and satisfying assignment, a strict
assignment checker, deterministic reconstruction code, and logs from two
independent exact maximum-clique programs (PMC and Open-MCS).

## Repository layout

- [`R4_18/`](R4_18/) contains the `R(4,18)` certificates.
- [`R4_21/`](R4_21/) contains the `R(4,21)` certificates, including earlier
  milestones retained for provenance.

Large experimental searches are kept out of version control. When a search
produces a new fully checked bound, its reproducibility bundle should be added
to the appropriate family directory and the table above updated.

For comparison with previously catalogued bounds, see Stanisław
Radziszowski's [Small Ramsey Numbers](https://doi.org/10.37236/21) dynamic
survey. The results here are computational certificates and have not yet been
peer reviewed.

# Candidate lower bound: R(4,21) >= 245

This bundle contains a 244-vertex graph with no 4-clique and no independent
set of size 21. Consequently it certifies

    R(4,21) >= 245.

The construction extends Google Research's published 243-vertex certificate
for `R(4,21) >= 244` by one vertex, numbered 243. The new vertex is adjacent
to the 40 zero-based vertex indices in `R4_21_ge_245_neighborhood.txt`.

## Why the extension works

Let `G` be the published 243-vertex graph and let `x` be the new vertex.
The published certificate establishes that `G` has no K4 and no independent
21-set. Two additional exact checks establish that:

1. `G[N(x)]` is triangle-free. Therefore no K4 containing `x` exists.
2. The nonneighbors of `x` in `G` contain no independent 20-set. Therefore no
   independent 21-set containing `x` exists.

All forbidden sets not containing `x` would already lie in `G`. This proves
the claim.

The neighborhood was found by an exact SAT/lazy-separation search. It required
95 independent-set cuts. A separate Cliquer-based check returned:

    PASS: triangle-free neighborhood of size 40; nonneighbors contain no independent set of size 20

## Files

- `R4_21_ge_245_matrix.txt`: normalized 244 by 244 adjacency matrix.
- `R4_21_ge_244_google_base.txt`: normalized published base matrix.
- `R4_21_ge_245_neighborhood.txt`: new vertex's neighborhood (zero-based).
- `verify_one_vertex_extension.cpp`: verifier for the two extension conditions.
- `verify_ramsey_matrix.cpp`: direct whole-matrix verifier.
- `cliquer_wrapper.c`: adapter used to call Cliquer from either verifier.
- `R4_21_ge_245_verification.txt`: search output, checks, and hashes.

The C/C++ verifiers expect a Cliquer-compatible `nautycliquer.h` and Cliquer
implementation. Invoke the extension verifier as:

    verify_one_vertex_extension R4_21_ge_244_google_base.txt R4_21_ge_245_neighborhood.txt 4 21

The direct verifier is invoked as:

    verify_ramsey_matrix R4_21_ge_245_matrix.txt 4 21

## Provenance and status

Published base certificate and explanation:
https://github.com/google-research/google-research/tree/master/ramsey_number_bounds/improved_bounds

The construction and verification here were completed on 2026-08-01. This is
a computational certificate, not a peer-reviewed publication. Searches of the
current survey, the Google Research certificate collection, and exact web
queries did not locate a prior `R(4,21) >= 245` result.

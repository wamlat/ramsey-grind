#!/usr/bin/env python3
"""Replace one vertex by two adjacent flexible vertices in an R(4,s) graph.

The input one-vertex extension CNF must contain every triangle clause and every
independent-(s-1)-set clause of the original graph.  Fixing the two new
vertices adjacent removes the independent sets containing both of them; the
extra four-literal clauses forbid a K4 using both new vertices and a core edge.
"""

from __future__ import annotations

import argparse
from math import isqrt
from pathlib import Path


def read_matrix(path: Path) -> list[list[int]]:
    raw = [int(ch) for ch in path.read_text() if ch in "01"]
    n = isqrt(len(raw))
    if n * n != len(raw):
        raise ValueError("matrix is not square")
    return [raw[i * n : (i + 1) * n] for i in range(n)]


def read_cnf(path: Path) -> tuple[int, list[list[int]]]:
    nvars = 0
    declared = None
    clauses: list[list[int]] = []
    for line in path.read_text().splitlines():
        if not line or line.startswith("c"):
            continue
        if line.startswith("p"):
            _, kind, ns, cs = line.split()
            if kind != "cnf":
                raise ValueError("input is not DIMACS CNF")
            nvars, declared = int(ns), int(cs)
            continue
        values = [int(x) for x in line.split()]
        if not values or values[-1] != 0:
            raise ValueError("unterminated clause")
        clauses.append(values[:-1])
    if declared != len(clauses):
        raise ValueError("clause count mismatch")
    return nvars, clauses


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("matrix", type=Path)
    parser.add_argument("extension_cnf", type=Path)
    parser.add_argument("deleted_vertex", type=int)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    matrix = read_matrix(args.matrix)
    n, clauses = read_cnf(args.extension_cnf)
    d = args.deleted_vertex
    if n != len(matrix) or not 0 <= d < n:
        raise ValueError("order/deleted vertex mismatch")

    core = [v for v in range(n) if v != d]
    core_index = {old: new for new, old in enumerate(core)}
    m = n - 1

    def y(old: int) -> int:
        return core_index[old] + 1

    def z(old: int) -> int:
        return m + core_index[old] + 1

    output: list[list[int]] = []
    triangles = independent_sets = 0
    for clause in clauses:
        old_vertices = [abs(lit) - 1 for lit in clause]
        if d in old_vertices:
            continue
        if all(lit < 0 for lit in clause):
            output.append([-y(v) for v in old_vertices])
            output.append([-z(v) for v in old_vertices])
            triangles += 1
        elif all(lit > 0 for lit in clause):
            output.append([y(v) for v in old_vertices])
            output.append([z(v) for v in old_vertices])
            independent_sets += 1
        else:
            raise ValueError("extension CNF contains a mixed-sign clause")

    core_edges = 0
    for ai, i in enumerate(core):
        for j in core[ai + 1 :]:
            if matrix[i][j]:
                output.append([-y(i), -y(j), -z(i), -z(j)])
                core_edges += 1

    with args.output.open("w") as out:
        out.write(
            f"c delete old vertex {d}; add two adjacent flexible vertices\n"
        )
        out.write(
            f"c y variables 1..{m}; z variables {m + 1}..{2 * m}\n"
        )
        out.write(f"p cnf {2 * m} {len(output)}\n")
        for clause in output:
            out.write(" ".join(map(str, clause)) + " 0\n")

    print(
        f"wrote {args.output}: vars={2*m} clauses={len(output)} "
        f"triangles={triangles} independent_sets={independent_sets} "
        f"core_edges={core_edges}"
    )


if __name__ == "__main__":
    main()

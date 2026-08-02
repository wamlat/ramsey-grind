#!/usr/bin/env python3
"""Delete two vertices and add a fully flexible three-vertex clique."""

from __future__ import annotations

import argparse
from pathlib import Path

from make_two_adjacent_tail import read_cnf, read_matrix


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("matrix", type=Path)
    parser.add_argument("extension_cnf", type=Path)
    parser.add_argument("deleted_vertex_a", type=int)
    parser.add_argument("deleted_vertex_b", type=int)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    matrix = read_matrix(args.matrix)
    n, clauses = read_cnf(args.extension_cnf)
    deleted = {args.deleted_vertex_a, args.deleted_vertex_b}
    if n != len(matrix) or len(deleted) != 2 or not all(0 <= v < n for v in deleted):
        raise ValueError("order/deleted vertex mismatch")
    core = [v for v in range(n) if v not in deleted]
    core_index = {old: new for new, old in enumerate(core)}
    m = len(core)

    def tail(t: int, old: int) -> int:
        return t * m + core_index[old] + 1

    output: list[list[int]] = []
    triangles = independent_sets = 0
    for clause in clauses:
        old_vertices = [abs(lit) - 1 for lit in clause]
        if any(v in deleted for v in old_vertices):
            continue
        if all(lit < 0 for lit in clause):
            for t in range(3):
                output.append([-tail(t, v) for v in old_vertices])
            triangles += 1
        elif all(lit > 0 for lit in clause):
            for t in range(3):
                output.append([tail(t, v) for v in old_vertices])
            independent_sets += 1
        else:
            raise ValueError("extension CNF contains a mixed-sign clause")

    core_edges = 0
    for ai, i in enumerate(core):
        for j in core[ai + 1 :]:
            if not matrix[i][j]:
                continue
            for first, second in ((0, 1), (0, 2), (1, 2)):
                output.append(
                    [
                        -tail(first, i),
                        -tail(first, j),
                        -tail(second, i),
                        -tail(second, j),
                    ]
                )
            core_edges += 1

    # The three new vertices plus one core vertex must not form a K4.
    for v in core:
        output.append([-tail(0, v), -tail(1, v), -tail(2, v)])

    with args.output.open("w") as out:
        out.write(
            f"c delete vertices {sorted(deleted)}; add a flexible 3-clique\n"
        )
        out.write(f"c three blocks of {m} neighborhood variables\n")
        out.write(f"p cnf {3 * m} {len(output)}\n")
        for clause in output:
            out.write(" ".join(map(str, clause)) + " 0\n")
    print(
        f"wrote {args.output}: vars={3*m} clauses={len(output)} "
        f"triangles={triangles} independent_sets={independent_sets} "
        f"core_edges={core_edges}"
    )


if __name__ == "__main__":
    main()

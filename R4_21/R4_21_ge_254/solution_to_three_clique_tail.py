#!/usr/bin/env python3
"""Reconstruct a graph from make_three_clique_tail.py's SAT assignment."""

from __future__ import annotations

import argparse
from pathlib import Path

from make_two_adjacent_tail import read_matrix


def read_assignment(path: Path, nvars: int) -> list[int]:
    values: list[int | None] = [None] * nvars
    for token in path.read_text().split():
        try:
            lit = int(token)
        except ValueError:
            continue
        if lit and abs(lit) <= nvars:
            values[abs(lit) - 1] = int(lit > 0)
    if any(value is None for value in values):
        raise ValueError("solution misses a tail variable")
    return [int(value) for value in values]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("matrix", type=Path)
    parser.add_argument("deleted_vertex_a", type=int)
    parser.add_argument("deleted_vertex_b", type=int)
    parser.add_argument("solution", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    old = read_matrix(args.matrix)
    deleted = {args.deleted_vertex_a, args.deleted_vertex_b}
    n = len(old)
    if len(deleted) != 2 or not all(0 <= v < n for v in deleted):
        raise ValueError("invalid deleted vertices")
    core = [v for v in range(n) if v not in deleted]
    m = len(core)
    assignment = read_assignment(args.solution, 3 * m)
    result_n = m + 3
    result = [[0] * result_n for _ in range(result_n)]
    for i, old_i in enumerate(core):
        for j, old_j in enumerate(core):
            result[i][j] = old[old_i][old_j]
    for t in range(3):
        new_vertex = m + t
        for i in range(m):
            value = assignment[t * m + i]
            result[new_vertex][i] = result[i][new_vertex] = value
    for first in range(3):
        for second in range(first + 1, 3):
            result[m + first][m + second] = result[m + second][m + first] = 1
    with args.output.open("w") as out:
        for row in result:
            out.write(" ".join(map(str, row)) + "\n")
    print(f"wrote order {result_n} matrix to {args.output}")


if __name__ == "__main__":
    main()

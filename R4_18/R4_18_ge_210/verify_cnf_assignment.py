#!/usr/bin/env python3
"""Strictly verify that a DIMACS assignment satisfies every CNF clause."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("cnf", type=Path)
    parser.add_argument("solution", type=Path)
    args = parser.parse_args()
    values: dict[int, bool] = {}
    for token in args.solution.read_text().split():
        try:
            lit = int(token)
        except ValueError:
            continue
        if lit:
            values[abs(lit)] = lit > 0

    nvars = declared = clauses = violated = 0
    for line in args.cnf.read_text().splitlines():
        if not line or line.startswith("c"):
            continue
        if line.startswith("p"):
            _, kind, ns, cs = line.split()
            if kind != "cnf":
                raise ValueError("not a DIMACS CNF")
            nvars, declared = int(ns), int(cs)
            continue
        literals = [int(token) for token in line.split()]
        if not literals or literals[-1] != 0:
            raise ValueError("unterminated clause")
        clauses += 1
        if not any(values.get(abs(lit), False) == (lit > 0)
                   for lit in literals[:-1]):
            violated += 1
    assigned = sum(v in values for v in range(1, nvars + 1))
    print(
        f"variables={nvars} assigned={assigned} clauses={clauses} "
        f"declared={declared} violated={violated}"
    )
    if assigned != nvars or clauses != declared or violated:
        raise SystemExit(1)


if __name__ == "__main__":
    main()

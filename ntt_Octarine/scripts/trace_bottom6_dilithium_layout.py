#!/usr/bin/env python3
"""Trace Dilithium levels2t7 register layout for one 64-coefficient block."""

from __future__ import annotations

import argparse
from dataclasses import dataclass


REG_NAMES = ("4", "5", "6", "7", "8", "9", "10", "11")


@dataclass(frozen=True)
class Butterfly:
    level: int
    bid: int
    low_reg: str
    high_reg: str
    low: tuple[int, ...]
    high: tuple[int, ...]
    r: tuple[int, ...]
    k_expr: tuple[str, ...]


def shuffle8(a: list[int], b: list[int]) -> tuple[list[int], list[int]]:
    return a[:4] + b[:4], a[4:] + b[4:]


def shuffle4(a: list[int], b: list[int]) -> tuple[list[int], list[int]]:
    return [a[0], a[1], b[0], b[1], a[4], a[5], b[4], b[5]], [
        a[2],
        a[3],
        b[2],
        b[3],
        a[6],
        a[7],
        b[6],
        b[7],
    ]


def shuffle2(a: list[int], b: list[int]) -> tuple[list[int], list[int]]:
    return [a[0], b[0], a[2], b[2], a[4], b[4], a[6], b[6]], [
        a[1],
        b[1],
        a[3],
        b[3],
        a[5],
        b[5],
        a[7],
        b[7],
    ]


def scalar_r(level: int, low: int, high: int) -> int:
    length = 1 << (9 - level)
    if high - low != length:
        raise ValueError(f"L{level}: pair ({low},{high}) is not distance {length}")
    start = (low // (2 * length)) * (2 * length)
    if not (start <= low < start + length and start + length <= high < start + 2 * length):
        raise ValueError(f"L{level}: pair ({low},{high}) is not in one scalar group")
    if high != low + length:
        raise ValueError(f"L{level}: pair ({low},{high}) is not scalar butterfly pair")
    return start // (2 * length)


def k_expr(level: int, r: int) -> str:
    base = {4: 16, 5: 32, 6: 64, 7: 128, 8: 256, 9: 512}[level]
    scale = {4: 1, 5: 2, 6: 4, 7: 8, 8: 16, 9: 32}[level]
    return f"{base}+{scale}*block+{r}"


def record(records: list[Butterfly], regs: dict[str, list[int]], level: int, low: str, high: str) -> None:
    lows = regs[low]
    highs = regs[high]
    rs: list[int] = []
    for l, h in zip(lows, highs):
        rs.append(scalar_r(level, l, h))
    records.append(
        Butterfly(
            level=level,
            bid=sum(1 for x in records if x.level == level),
            low_reg=low,
            high_reg=high,
            low=tuple(lows),
            high=tuple(highs),
            r=tuple(rs),
            k_expr=tuple(k_expr(level, r) for r in rs),
        )
    )


def trace() -> tuple[list[Butterfly], list[int]]:
    regs = {
        "4": list(range(0, 8)),
        "5": list(range(8, 16)),
        "6": list(range(16, 24)),
        "7": list(range(24, 32)),
        "8": list(range(32, 40)),
        "9": list(range(40, 48)),
        "10": list(range(48, 56)),
        "11": list(range(56, 64)),
    }
    records: list[Butterfly] = []

    for low, high in (("4", "8"), ("5", "9"), ("6", "10"), ("7", "11")):
        record(records, regs, 4, low, high)

    regs["3"], regs["8"] = shuffle8(regs["4"], regs["8"])
    regs["4"], regs["9"] = shuffle8(regs["5"], regs["9"])
    regs["5"], regs["10"] = shuffle8(regs["6"], regs["10"])
    regs["6"], regs["11"] = shuffle8(regs["7"], regs["11"])

    for low, high in (("3", "5"), ("8", "10"), ("4", "6"), ("9", "11")):
        record(records, regs, 5, low, high)

    regs["7"], regs["5"] = shuffle4(regs["3"], regs["5"])
    regs["3"], regs["10"] = shuffle4(regs["8"], regs["10"])
    regs["8"], regs["6"] = shuffle4(regs["4"], regs["6"])
    regs["4"], regs["11"] = shuffle4(regs["9"], regs["11"])

    for low, high in (("7", "8"), ("5", "6"), ("3", "4"), ("10", "11")):
        record(records, regs, 6, low, high)

    regs["9"], regs["8"] = shuffle2(regs["7"], regs["8"])
    regs["7"], regs["6"] = shuffle2(regs["5"], regs["6"])
    regs["5"], regs["4"] = shuffle2(regs["3"], regs["4"])
    regs["3"], regs["11"] = shuffle2(regs["10"], regs["11"])

    for low, high in (("9", "5"), ("8", "4"), ("7", "3"), ("6", "11")):
        record(records, regs, 7, low, high)
    for low, high in (("9", "7"), ("8", "6"), ("5", "3"), ("4", "11")):
        record(records, regs, 8, low, high)
    for low, high in (("9", "8"), ("7", "6"), ("5", "4"), ("3", "11")):
        record(records, regs, 9, low, high)

    final = regs["9"] + regs["8"] + regs["7"] + regs["6"] + regs["5"] + regs["4"] + regs["3"] + regs["11"]
    return records, final


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()
    try:
        records, final = trace()
    except ValueError as exc:
        print(f"TRACE ASSERTION FAILED: {exc}")
        return 1

    if not args.quiet:
        for rec in records:
            print(
                f"L{rec.level} bfly {rec.bid:02d}: {rec.low_reg}[{list(rec.low)}] "
                f"x {rec.high_reg}[{list(rec.high)}] r={list(rec.r)} k={list(rec.k_expr)}"
            )
        print("final_store_permutation:")
        for pos, src in enumerate(final):
            print(f"  out[{pos:02d}] <- natural[{src:02d}]")
        inv = [0] * 64
        for pos, src in enumerate(final):
            inv[src] = pos
        print("inverse_permutation:")
        for natural, pos in enumerate(inv):
            print(f"  natural[{natural:02d}] is at out[{pos:02d}]")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

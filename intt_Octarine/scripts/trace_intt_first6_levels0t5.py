#!/usr/bin/env python3
from __future__ import annotations

from collections import defaultdict, deque

BLOCKS = 16
BLOCK = 64
STRIDE = 80


def shuffle2(regs: dict[int, list[str]], r0: int, r1: int, r2: int, r3: int) -> None:
    a = regs[r0][:]
    b = regs[r1][:]
    regs[r2] = [a[0], b[0], a[2], b[2], a[4], b[4], a[6], b[6]]
    regs[r3] = [a[1], b[1], a[3], b[3], a[5], b[5], a[7], b[7]]


def shuffle4(regs: dict[int, list[str]], r0: int, r1: int, r2: int, r3: int) -> None:
    a = regs[r0][:]
    b = regs[r1][:]
    regs[r2] = a[0:2] + b[0:2] + a[4:6] + b[4:6]
    regs[r3] = a[2:4] + b[2:4] + a[6:8] + b[6:8]


def shuffle8(regs: dict[int, list[str]], r0: int, r1: int, r2: int, r3: int) -> None:
    a = regs[r0][:]
    b = regs[r1][:]
    regs[r2] = a[:4] + b[:4]
    regs[r3] = a[4:] + b[4:]


def initial_packed_regs() -> dict[int, list[int]]:
    return {
        4: [0, 8, 16, 24, 32, 40, 48, 56],
        5: [1, 9, 17, 25, 33, 41, 49, 57],
        6: [2, 10, 18, 26, 34, 42, 50, 58],
        7: [3, 11, 19, 27, 35, 43, 51, 59],
        8: [4, 12, 20, 28, 36, 44, 52, 60],
        9: [5, 13, 21, 29, 37, 45, 53, 61],
        10: [6, 14, 22, 30, 38, 46, 54, 62],
        11: [7, 15, 23, 31, 39, 47, 55, 63],
    }


def k_for(block: int, level: int, low: int) -> int:
    if level == 0:
        return block * 32 + low // 2
    if level == 1:
        return 512 + block * 16 + low // 4
    if level == 2:
        return 768 + block * 8 + low // 8
    if level == 3:
        return 896 + block * 4 + low // 16
    if level == 4:
        return 960 + block * 2 + low // 32
    if level == 5:
        return 992 + block
    raise ValueError(level)


def root_group_for_pair(regs: dict[int, list[int]], block: int, level: int, low_reg: int) -> list[int]:
    return [k_for(block, level, low) for low in regs[low_reg]]


def butterfly_pairs(regs: dict[int, list[int]], block: int, level: int, l: int, h: int) -> list[tuple[int, int, int]]:
    out = []
    length = 1 << level
    for low, high in zip(regs[l], regs[h]):
        if high != low + length:
            raise SystemExit(f"bad pair level={level} regs={l},{h}: ({low},{high})")
        out.append((block * BLOCK + low, block * BLOCK + high, k_for(block, level, low)))
    return out


def levels0t5_source_k(block: int) -> list[int]:
    regs = initial_packed_regs()
    groups: list[list[int]] = []
    pairs: list[tuple[int, int, int]] = []

    for l, h in [(4, 5), (6, 7), (8, 9), (10, 11)]:
        groups.append(root_group_for_pair(regs, block, 0, l))
        pairs.extend(butterfly_pairs(regs, block, 0, l, h))

    for l, h in [(4, 6), (5, 7)]:
        pairs.extend(butterfly_pairs(regs, block, 1, l, h))
    groups.append(root_group_for_pair(regs, block, 1, 4))
    for l, h in [(8, 10), (9, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 1, l, h))
    groups.append(root_group_for_pair(regs, block, 1, 8))

    for l, h in [(4, 8), (5, 9), (6, 10), (7, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 2, l, h))
    groups.append(root_group_for_pair(regs, block, 2, 4))

    shuffle2({k: [str(x) for x in v] for k, v in regs.items()}, 4, 5, 3, 5)
    r = {k: [str(x) for x in v] for k, v in regs.items()}
    shuffle2(r, 4, 5, 3, 5)
    shuffle2(r, 6, 7, 4, 7)
    shuffle2(r, 8, 9, 6, 9)
    shuffle2(r, 10, 11, 8, 11)
    regs = {k: [int(x) for x in v] for k, v in r.items()}
    for l, h in [(3, 5), (4, 7), (6, 9), (8, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 3, l, h))
    groups.append(root_group_for_pair(regs, block, 3, 3))

    r = {k: [str(x) for x in v] for k, v in regs.items()}
    shuffle4(r, 3, 4, 10, 4)
    shuffle4(r, 6, 8, 3, 8)
    shuffle4(r, 5, 7, 6, 7)
    shuffle4(r, 9, 11, 5, 11)
    regs = {k: [int(x) for x in v] for k, v in r.items()}
    for l, h in [(10, 4), (3, 8), (6, 7), (5, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 4, l, h))
    groups.append(root_group_for_pair(regs, block, 4, 10))

    r = {k: [str(x) for x in v] for k, v in regs.items()}
    shuffle8(r, 10, 3, 9, 3)
    shuffle8(r, 6, 5, 10, 5)
    shuffle8(r, 4, 8, 6, 8)
    shuffle8(r, 7, 11, 4, 11)
    regs = {k: [int(x) for x in v] for k, v in r.items()}
    for l, h in [(9, 3), (10, 5), (6, 8), (4, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 5, l, h))
    groups.append(root_group_for_pair(regs, block, 5, 9))

    flat = [k for group in groups for k in group]
    if len(groups) != 10 or len(flat) != STRIDE:
        raise SystemExit("bad group count")
    if len(set(flat)) != 63:
        raise SystemExit(f"block {block}: unique k count {len(set(flat))}")
    expected_pairs = set()
    for level in range(6):
        length = 1 << level
        for start in range(0, BLOCK, 2 * length):
            for j in range(start, start + length):
                expected_pairs.add((block * BLOCK + j, block * BLOCK + j + length, k_for(block, level, j)))
    if set(pairs) != expected_pairs:
        raise SystemExit(f"block {block}: butterfly coverage mismatch")
    final_order = [x for reg in [9, 10, 6, 4, 3, 5, 8, 11] for x in regs[reg]]
    if final_order != list(range(64)):
        raise SystemExit(f"block {block}: final layout not natural via planned store order")
    return flat


def main() -> int:
    print("INTT first6 levels0t5 trace")
    print("initial pack after natural loads: transpose 8x8 dwords")
    print("packed input:")
    for r, vals in initial_packed_regs().items():
        print(f"  ymm{r} = {vals}")
    first = levels0t5_source_k(0)
    for group in range(10):
        if group < 4:
            level = 0
            use = "vmovdqu + vmovshdup odd roots; one butterfly"
        elif group < 6:
            level = 1
            use = "vmovdqu + vmovshdup odd roots; two butterflies"
        elif group == 6:
            level = 2
            use = "vmovdqu + vmovshdup odd roots; four butterflies"
        else:
            level = group - 4
            use = "vmovdqu + vmovshdup odd roots; four butterflies after shuffle"
        print(f"group {group}: offset={group * 32} level={level} source_k={first[group*8:group*8+8]} use={use}")
    for block in range(BLOCKS):
        levels0t5_source_k(block)
    print("final store order: ymm9, ymm10, ymm6, ymm4, ymm3, ymm5, ymm8, ymm11")
    print("OK: levels0t5 covers scalar first6 pairs; 80 slots/block; 63 unique k/block")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

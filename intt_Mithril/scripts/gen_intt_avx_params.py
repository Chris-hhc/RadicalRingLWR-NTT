#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re

BLOCKS = 2
BLOCK = 64
FIRST6_STRIDE = 80
FIRST6_WORDS = BLOCKS * FIRST6_STRIDE
TAIL_WORDS = 2
META_WORDS = 8
FIRST6_ZETAS_OFFSET = 0
FIRST6_PRE_OFFSET = FIRST6_ZETAS_OFFSET + FIRST6_WORDS
TAIL_ZETAS_OFFSET = FIRST6_PRE_OFFSET + FIRST6_WORDS
TAIL_PRE_OFFSET = TAIL_ZETAS_OFFSET + TAIL_WORDS
META_OFFSET = TAIL_PRE_OFFSET + TAIL_WORDS
PARAM_WORDS = META_OFFSET + META_WORDS


def parse_array(text: str, name: str) -> list[int]:
    pattern = rf"static\s+const\s+int32_t\s+{re.escape(name)}\s*\[RRLWR_N\].*?=\s*\{{(.*?)\}};"
    match = re.search(pattern, text, re.S)
    if not match:
        raise SystemExit(f"could not find {name}")
    return [int(tok, 0) for tok in re.findall(r"[-+]?(?:0x[0-9a-fA-F]+|\d+)", match.group(1))]


def parse_macro(text: str, name: str) -> int:
    pattern = rf"#define\s+{re.escape(name)}\s*\(([-+]?(?:0x[0-9a-fA-F]+|\d+))\)"
    match = re.search(pattern, text)
    if not match:
        raise SystemExit(f"could not find {name}")
    return int(match.group(1), 0)


def shuffle2(regs: dict[int, list[int]], r0: int, r1: int, r2: int, r3: int) -> None:
    a = regs[r0][:]
    b = regs[r1][:]
    regs[r2] = [a[0], b[0], a[2], b[2], a[4], b[4], a[6], b[6]]
    regs[r3] = [a[1], b[1], a[3], b[3], a[5], b[5], a[7], b[7]]


def shuffle4(regs: dict[int, list[int]], r0: int, r1: int, r2: int, r3: int) -> None:
    a = regs[r0][:]
    b = regs[r1][:]
    regs[r2] = a[0:2] + b[0:2] + a[4:6] + b[4:6]
    regs[r3] = a[2:4] + b[2:4] + a[6:8] + b[6:8]


def shuffle8(regs: dict[int, list[int]], r0: int, r1: int, r2: int, r3: int) -> None:
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
        return 64 + block * 16 + low // 4
    if level == 2:
        return 96 + block * 8 + low // 8
    if level == 3:
        return 112 + block * 4 + low // 16
    if level == 4:
        return 120 + block * 2 + low // 32
    if level == 5:
        return 124 + block
    raise ValueError(level)


def group_for_pair(regs: dict[int, list[int]], block: int, level: int, low_reg: int) -> list[int]:
    return [k_for(block, level, low) for low in regs[low_reg]]


def butterfly_pairs(regs: dict[int, list[int]], block: int, level: int, l: int, h: int) -> list[tuple[int, int, int]]:
    length = 1 << level
    out: list[tuple[int, int, int]] = []
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
        groups.append(group_for_pair(regs, block, 0, l))
        pairs.extend(butterfly_pairs(regs, block, 0, l, h))

    for l, h in [(4, 6), (5, 7)]:
        pairs.extend(butterfly_pairs(regs, block, 1, l, h))
    groups.append(group_for_pair(regs, block, 1, 4))
    for l, h in [(8, 10), (9, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 1, l, h))
    groups.append(group_for_pair(regs, block, 1, 8))

    for l, h in [(4, 8), (5, 9), (6, 10), (7, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 2, l, h))
    groups.append(group_for_pair(regs, block, 2, 4))

    shuffle2(regs, 4, 5, 3, 5)
    shuffle2(regs, 6, 7, 4, 7)
    shuffle2(regs, 8, 9, 6, 9)
    shuffle2(regs, 10, 11, 8, 11)
    for l, h in [(3, 5), (4, 7), (6, 9), (8, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 3, l, h))
    groups.append(group_for_pair(regs, block, 3, 3))

    shuffle4(regs, 3, 4, 10, 4)
    shuffle4(regs, 6, 8, 3, 8)
    shuffle4(regs, 5, 7, 6, 7)
    shuffle4(regs, 9, 11, 5, 11)
    for l, h in [(10, 4), (3, 8), (6, 7), (5, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 4, l, h))
    groups.append(group_for_pair(regs, block, 4, 10))

    shuffle8(regs, 10, 3, 9, 3)
    shuffle8(regs, 6, 5, 10, 5)
    shuffle8(regs, 4, 8, 6, 8)
    shuffle8(regs, 7, 11, 4, 11)
    for l, h in [(9, 3), (10, 5), (6, 8), (4, 11)]:
        pairs.extend(butterfly_pairs(regs, block, 5, l, h))
    groups.append(group_for_pair(regs, block, 5, 9))

    flat = [k for group in groups for k in group]
    if len(flat) != FIRST6_STRIDE or len(set(flat)) != 63:
        raise SystemExit(f"bad first6 map for block {block}")
    expected_pairs = set()
    for level in range(6):
        length = 1 << level
        for start in range(0, BLOCK, 2 * length):
            for j in range(start, start + length):
                expected_pairs.add((block * BLOCK + j, block * BLOCK + j + length, k_for(block, level, j)))
    if set(pairs) != expected_pairs:
        raise SystemExit(f"butterfly coverage mismatch for block {block}")
    return flat


def build_first6(source: list[int]) -> list[int]:
    out: list[int] = []
    for block in range(BLOCKS):
        out.extend(source[k] for k in levels0t5_source_k(block))
    if len(out) != FIRST6_WORDS:
        raise SystemExit("bad first6 table length")
    return out


def build_param(first6_zeta: list[int],
                first6_pre: list[int],
                tail_zeta: list[int],
                tail_pre: list[int],
                fused_flag: int) -> list[int]:
    table = first6_zeta + first6_pre + tail_zeta + tail_pre + [fused_flag, 0, 0, 0, 0, 0, 0, 0]
    if len(table) != PARAM_WORDS:
        raise SystemExit(f"bad param length {len(table)} != {PARAM_WORDS}")
    return table


def format_array(name: str, values: list[int], length_macro: str) -> list[str]:
    lines = [f"static const int32_t {name}[{length_macro}] __attribute__((aligned(32))) = {{"]
    for i in range(0, len(values), 8):
        lines.append("  " + ", ".join(str(v) for v in values[i : i + 8]) + ",")
    lines[-1] = lines[-1].rstrip(",")
    lines.append("};")
    return lines


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--precalcu", required=True)
    parser.add_argument("--fused", required=True)
    parser.add_argument("--parameters", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    precalcu_text = pathlib.Path(args.precalcu).read_text()
    fused_text = pathlib.Path(args.fused).read_text()
    parameters_text = pathlib.Path(args.parameters).read_text()

    z = parse_array(precalcu_text, "RRLWR_KEM_ZETAS_INTT")
    p = parse_array(precalcu_text, "RRLWR_KEM_ZETAS_INTT_PRE")
    fz = parse_array(fused_text, "RRLWR_KEM_ZETAS_INTT_FINALFUSED")
    fp = parse_array(fused_text, "RRLWR_KEM_ZETAS_INTT_FINALFUSED_PRE")
    finalconst = parse_macro(parameters_text, "RRLWR_NTTINV_FINALCONST")
    finalconst_pre = parse_macro(precalcu_text, "RRLWR_KEM_NTTINV_FINALCONST_PRE")

    first6_z = build_first6(z)
    first6_p = build_first6(p)
    first6_fz = build_first6(fz)
    first6_fp = build_first6(fp)

    normal = build_param(first6_z, first6_p, [z[126], finalconst], [p[126], finalconst_pre], 0)
    fused = build_param(first6_fz, first6_fp, [fz[126], finalconst], [fp[126], finalconst_pre], 1)

    lines: list[str] = []
    lines.append("/* Auto-generated by scripts/gen_intt_avx_params.py */")
    lines.append("#ifndef INTT_PARAM_H")
    lines.append("#define INTT_PARAM_H")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("#include \"parameters.h\"")
    lines.append("")
    lines.append(f"#define INTT_AVX_PARAM_WORDS {PARAM_WORDS}")
    lines.append(f"#define INTT_AVX_FIRST6_BLOCKS {BLOCKS}")
    lines.append(f"#define INTT_AVX_FIRST6_BLOCK_STRIDE {FIRST6_STRIDE}")
    lines.append(f"#define INTT_AVX_FIRST6_WORDS {FIRST6_WORDS}")
    lines.append("")
    lines.extend(format_array("RRLWR_KEM_INTT_AVX_PARAMS", normal, "INTT_AVX_PARAM_WORDS"))
    lines.append("")
    lines.extend(format_array("RRLWR_KEM_INTT_FUSED_AVX_PARAMS", fused, "INTT_AVX_PARAM_WORDS"))
    lines.append("")
    lines.append("#endif")
    lines.append("")
    pathlib.Path(args.output).write_text("\n".join(lines))
    print(f"generated {args.output}: words={PARAM_WORDS} first6_words={FIRST6_WORDS}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

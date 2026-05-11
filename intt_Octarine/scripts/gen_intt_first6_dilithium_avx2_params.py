#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re

from trace_intt_first6_levels0t5 import levels0t5_source_k

COMPACT_BLOCK_STRIDE = 64
NTTSTYLE_BLOCK_STRIDE = 80
BLOCKS = 16
COMPACT_FIRST6_WORDS = BLOCKS * COMPACT_BLOCK_STRIDE
NTTSTYLE_FIRST6_WORDS = BLOCKS * NTTSTYLE_BLOCK_STRIDE
TAIL_WORDS = 16
META_WORDS = 8
FIRST6_ZETAS_OFFSET = 0
FIRST6_PRE_OFFSET = FIRST6_ZETAS_OFFSET + NTTSTYLE_FIRST6_WORDS
TAIL_ZETAS_OFFSET = FIRST6_PRE_OFFSET + NTTSTYLE_FIRST6_WORDS
TAIL_PRE_OFFSET = TAIL_ZETAS_OFFSET + TAIL_WORDS
META_OFFSET = TAIL_PRE_OFFSET + TAIL_WORDS
PARAM_WORDS = META_OFFSET + META_WORDS
COMPACT_TAIL_ZETAS_OFFSET = COMPACT_FIRST6_WORDS * 2
COMPACT_TAIL_PRE_OFFSET = COMPACT_TAIL_ZETAS_OFFSET + TAIL_WORDS
COMPACT_META_OFFSET = COMPACT_TAIL_PRE_OFFSET + TAIL_WORDS
COMPACT_PARAM_WORDS = COMPACT_META_OFFSET + META_WORDS


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


def build_compact_block_table(source: list[int]) -> tuple[list[int], list[int]]:
    out: list[int] = []
    mapping: list[int] = []
    for block in range(BLOCKS):
        before = len(out)
        k = block * 32
        out.extend(source[k : k + 32])
        mapping.extend(range(k, k + 32))

        k = 512 + block * 16
        out.extend(source[k : k + 16])
        mapping.extend(range(k, k + 16))

        k = 768 + block * 8
        out.extend(source[k : k + 8])
        mapping.extend(range(k, k + 8))

        k = 896 + block * 4
        out.extend(source[k : k + 4])
        mapping.extend(range(k, k + 4))

        k = 960 + block * 2
        out.extend(source[k : k + 2])
        mapping.extend(range(k, k + 2))

        k = 992 + block
        out.append(source[k])
        mapping.append(k)
        out.append(0)
        mapping.append(0xFFFF)
        if len(out) - before != COMPACT_BLOCK_STRIDE:
            raise SystemExit(f"internal stride mismatch for block {block}: {len(out) - before}")
    return out, mapping


def build_nttstyle_block_table(source: list[int]) -> tuple[list[int], list[int]]:
    out: list[int] = []
    mapping: list[int] = []
    for block in range(BLOCKS):
        before = len(out)

        k = block * 32
        out.extend(source[k : k + 32])
        mapping.extend(range(k, k + 32))

        k = 512 + block * 16
        out.extend(source[k : k + 16])
        mapping.extend(range(k, k + 16))

        k = 768 + block * 8
        out.extend(source[k : k + 8])
        mapping.extend(range(k, k + 8))

        k = 896 + block * 4
        for r in range(4):
            out.extend([source[k + r], source[k + r]])
            mapping.extend([k + r, k + r])

        k = 960 + block * 2
        for r in range(2):
            out.extend([source[k + r]] * 4)
            mapping.extend([k + r] * 4)

        k = 992 + block
        out.extend([source[k]] * 8)
        mapping.extend([k] * 8)

        if len(out) - before != NTTSTYLE_BLOCK_STRIDE:
            raise SystemExit(f"internal nttstyle stride mismatch for block {block}: {len(out) - before}")
    return out, mapping


def build_levels0t5_block_table(source: list[int]) -> tuple[list[int], list[int]]:
    out: list[int] = []
    mapping: list[int] = []
    for block in range(BLOCKS):
        block_map = levels0t5_source_k(block)
        if len(block_map) != NTTSTYLE_BLOCK_STRIDE:
            raise SystemExit(f"levels0t5 stride mismatch for block {block}")
        out.extend(source[k] for k in block_map)
        mapping.extend(block_map)
    return out, mapping


def expected_unique_mapping() -> list[int]:
    mapping: list[int] = []
    for block in range(BLOCKS):
        k = block * 32
        mapping.extend(range(k, k + 32))

        k = 512 + block * 16
        mapping.extend(range(k, k + 16))

        k = 768 + block * 8
        mapping.extend(range(k, k + 8))

        k = 896 + block * 4
        mapping.extend(range(k, k + 4))

        k = 960 + block * 2
        mapping.extend(range(k, k + 2))

        k = 992 + block
        mapping.append(k)
    return mapping


def build_tail_table(zetas: list[int], pre: list[int], finalconst: int, finalconst_pre: int) -> tuple[list[int], list[int]]:
    tail_zeta: list[int] = []
    tail_pre: list[int] = []
    tail_zeta.extend(zetas[1008:1016])
    tail_pre.extend(pre[1008:1016])
    tail_zeta.extend(zetas[1016:1020])
    tail_pre.extend(pre[1016:1020])
    tail_zeta.extend(zetas[1020:1022])
    tail_pre.extend(pre[1020:1022])
    tail_zeta.append(zetas[1022])
    tail_pre.append(pre[1022])
    tail_zeta.append(finalconst)
    tail_pre.append(finalconst_pre)
    if len(tail_zeta) != TAIL_WORDS or len(tail_pre) != TAIL_WORDS:
        raise SystemExit("unexpected tail table length")
    return tail_zeta, tail_pre


def build_param(first6_zeta: list[int],
                first6_pre: list[int],
                tail_zeta: list[int],
                tail_pre: list[int],
                fused_flag: int) -> list[int]:
    table: list[int] = []
    table.extend(first6_zeta)
    table.extend(first6_pre)
    table.extend(tail_zeta)
    table.extend(tail_pre)
    table.extend([fused_flag, 0, 0, 0, 0, 0, 0, 0])
    if len(table) != PARAM_WORDS:
        raise SystemExit(f"unexpected param length {len(table)} != {PARAM_WORDS}")
    return table


def build_compact_param(first6_zeta: list[int],
                        first6_pre: list[int],
                        tail_zeta: list[int],
                        tail_pre: list[int],
                        fused_flag: int) -> list[int]:
    table: list[int] = []
    table.extend(first6_zeta)
    table.extend(first6_pre)
    table.extend(tail_zeta)
    table.extend(tail_pre)
    table.extend([fused_flag, 0, 0, 0, 0, 0, 0, 0])
    if len(table) != COMPACT_PARAM_WORDS:
        raise SystemExit(f"unexpected compact param length {len(table)} != {COMPACT_PARAM_WORDS}")
    return table


def format_array(name: str, values: list[int], length_macro: str) -> list[str]:
    lines = [f"static const int32_t {name}[{length_macro}] __attribute__((aligned(32))) = {{"]
    for i in range(0, len(values), 8):
        lines.append("  " + ", ".join(str(v) for v in values[i : i + 8]) + ",")
    lines[-1] = lines[-1].rstrip(",")
    lines.append("};")
    return lines


def format_mapping(name: str, values: list[int], length_macro: str) -> list[str]:
    lines = [f"static const uint16_t {name}[{length_macro}] = {{"]
    for i in range(0, len(values), 16):
        lines.append("  " + ", ".join(str(v) for v in values[i : i + 16]) + ",")
    lines[-1] = lines[-1].rstrip(",")
    lines.append("};")
    return lines


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--fused", required=True)
    parser.add_argument("--parameters", default="parameters.h")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    text = pathlib.Path(args.input).read_text()
    fused_text = pathlib.Path(args.fused).read_text()
    parameters_text = pathlib.Path(args.parameters).read_text()
    z1 = parse_array(text, "RRLWR_SIGN_ZETAS1_INTT")
    p1 = parse_array(text, "RRLWR_SIGN_ZETAS1_INTT_PRE")
    z2 = parse_array(text, "RRLWR_SIGN_ZETAS2_INTT")
    p2 = parse_array(text, "RRLWR_SIGN_ZETAS2_INTT_PRE")
    fz1 = parse_array(fused_text, "RRLWR_SIGN_ZETAS1_INTT_FINALFUSED")
    fp1 = parse_array(fused_text, "RRLWR_SIGN_ZETAS1_INTT_FINALFUSED_PRE")
    fz2 = parse_array(fused_text, "RRLWR_SIGN_ZETAS2_INTT_FINALFUSED")
    fp2 = parse_array(fused_text, "RRLWR_SIGN_ZETAS2_INTT_FINALFUSED_PRE")
    finalconst1 = parse_macro(parameters_text, "RRLWR_SIGN_NTTINV_FINALCONST1")
    finalconst2 = parse_macro(parameters_text, "RRLWR_SIGN_NTTINV_FINALCONST2")
    finalconst1_pre = parse_macro(text, "RRLWR_SIGN_NTTINV_FINALCONST1_PRE")
    finalconst2_pre = parse_macro(text, "RRLWR_SIGN_NTTINV_FINALCONST2_PRE")

    z1_levels0t5, levels0t5_mapping = build_levels0t5_block_table(z1)
    p1_levels0t5, levels0t5_mapping_pre = build_levels0t5_block_table(p1)
    z2_levels0t5, levels0t5_mapping2 = build_levels0t5_block_table(z2)
    p2_levels0t5, levels0t5_mapping2_pre = build_levels0t5_block_table(p2)
    if levels0t5_mapping != levels0t5_mapping_pre or levels0t5_mapping != levels0t5_mapping2 or levels0t5_mapping != levels0t5_mapping2_pre:
        raise SystemExit("levels0t5 zeta/pre mapping mismatch")
    unique_mapping = expected_unique_mapping()
    for block in range(BLOCKS):
        used_levels = sorted(set(levels0t5_mapping[block * NTTSTYLE_BLOCK_STRIDE : (block + 1) * NTTSTYLE_BLOCK_STRIDE]))
        expected = list(range(block * 32, block * 32 + 32))
        expected += list(range(512 + block * 16, 512 + block * 16 + 16))
        expected += list(range(768 + block * 8, 768 + block * 8 + 8))
        expected += list(range(896 + block * 4, 896 + block * 4 + 4))
        expected += list(range(960 + block * 2, 960 + block * 2 + 2))
        expected += [992 + block]
        if used_levels != sorted(expected) or len(used_levels) != 63:
            raise SystemExit(f"levels0t5 source-k mismatch for block {block}")
        if unique_mapping[block * 63 : (block + 1) * 63] != expected:
            raise SystemExit(f"unique mapping mismatch for block {block}")

    tail1_zeta, tail1_pre = build_tail_table(z1, p1, finalconst1, finalconst1_pre)
    tail2_zeta, tail2_pre = build_tail_table(z2, p2, finalconst2, finalconst2_pre)

    tail1_fused_zeta, tail1_fused_pre = build_tail_table(fz1, fp1, finalconst1, finalconst1_pre)
    tail2_fused_zeta, tail2_fused_pre = build_tail_table(fz2, fp2, finalconst2, finalconst2_pre)

    params1 = build_param(z1_levels0t5, p1_levels0t5, tail1_zeta, tail1_pre, 0)
    params2 = build_param(z2_levels0t5, p2_levels0t5, tail2_zeta, tail2_pre, 0)
    params1_fused = build_param(z1_levels0t5, p1_levels0t5, tail1_fused_zeta, tail1_fused_pre, 1)
    params2_fused = build_param(z2_levels0t5, p2_levels0t5, tail2_fused_zeta, tail2_fused_pre, 1)
    lines: list[str] = []
    lines.append("/* Auto-generated by scripts/gen_intt_first6_dilithium_avx2_params.py */")
    lines.append("#ifndef INTT_PARAM_H")
    lines.append("#define INTT_PARAM_H")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("#include \"parameters.h\"")
    lines.append("")
    lines.append(f"#define INTT_AVX_PARAM_WORDS {PARAM_WORDS}")
    lines.append("")
    lines.extend(format_array("RRLWR_SIGN_INTT_AVX_PARAMS1", params1, "INTT_AVX_PARAM_WORDS"))
    lines.append("")
    lines.extend(format_array("RRLWR_SIGN_INTT_AVX_PARAMS2", params2, "INTT_AVX_PARAM_WORDS"))
    lines.append("")
    lines.extend(format_array("RRLWR_SIGN_INTT_FUSED_AVX_PARAMS1", params1_fused, "INTT_AVX_PARAM_WORDS"))
    lines.append("")
    lines.extend(format_array("RRLWR_SIGN_INTT_FUSED_AVX_PARAMS2", params2_fused, "INTT_AVX_PARAM_WORDS"))
    lines.append("")
    lines.append("#endif")
    lines.append("")

    pathlib.Path(args.output).write_text("\n".join(lines))
    print(f"generated {args.output}: param_words={len(params1)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

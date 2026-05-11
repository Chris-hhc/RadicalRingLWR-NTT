#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re


def parse_final_perm(header: pathlib.Path) -> list[int]:
    text = header.read_text()
    match = re.search(r"NTT1024_BOT6_DILITHIUM_AVX2_FINAL_PERM\[64\]\s*=\s*\{(.*?)\};", text, re.S)
    if not match:
        raise SystemExit(f"could not find NTT1024_BOT6_DILITHIUM_AVX2_FINAL_PERM in {header}")
    values = [int(tok, 0) for tok in re.findall(r"\d+", match.group(1))]
    if len(values) != 64:
        raise SystemExit(f"expected 64 FINAL_PERM values, got {len(values)}")
    if sorted(values) != list(range(64)):
        raise SystemExit("FINAL_PERM is not a permutation of 0..63")
    return values


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


def transpose_network(packed: list[list[int]]) -> list[list[int]]:
    t0, t1 = shuffle2(packed[0], packed[1])
    t2, t3 = shuffle2(packed[2], packed[3])
    t4, t5 = shuffle2(packed[4], packed[5])
    t6, t7 = shuffle2(packed[6], packed[7])

    u0, u2 = shuffle4(t0, t2)
    u1, u3 = shuffle4(t1, t3)
    u4, u6 = shuffle4(t4, t6)
    u5, u7 = shuffle4(t5, t7)

    out0, out4 = shuffle8(u0, u4)
    out1, out5 = shuffle8(u1, u5)
    out2, out6 = shuffle8(u2, u6)
    out3, out7 = shuffle8(u3, u7)
    return [out0, out1, out2, out3, out4, out5, out6, out7]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--header", default="ntt1024_bot6_dilithium_zetas.h")
    args = parser.parse_args()

    final = parse_final_perm(pathlib.Path(args.header))
    packed = [final[i : i + 8] for i in range(0, 64, 8)]

    print("FINAL_PERM:")
    print("  " + ", ".join(str(v) for v in final))
    print("")
    for chunk in range(8):
      needed = [pos for pos, natural in enumerate(final) if chunk * 8 <= natural < chunk * 8 + 8]
      print(f"natural chunk {chunk} needs packed positions {needed}")

    print("")
    vector_reorder = True
    store_order = []
    for chunk in range(8):
        found = None
        for pv in range(8):
            if all(final[pv * 8 + lane] == chunk * 8 + lane for lane in range(8)):
                found = pv
                break
        if found is None:
            vector_reorder = False
        store_order.append(found)
    print(f"simple whole-YMM vector reorder: {vector_reorder}")
    if vector_reorder:
        print(f"suggested packed vector store order: {store_order}")
    else:
        print("whole-YMM reorder is insufficient; lane-level shuffle is required")

    natural = [list(range(i * 8, i * 8 + 8)) for i in range(8)]
    transposed = transpose_network(packed)
    print(f"shuffle2/shuffle4/shuffle8 transpose network reaches natural order: {transposed == natural}")
    if transposed != natural:
        print("network output:")
        for idx, vec in enumerate(transposed):
            print(f"  out{idx}: {vec}")
        return 1

    print("recommended plan: in-register 8x8 int32 transpose, then store out0..out7 to natural chunks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

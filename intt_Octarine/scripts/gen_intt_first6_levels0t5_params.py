#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import runpy
import sys


def main() -> int:
    script = pathlib.Path(__file__).with_name("gen_intt_first6_dilithium_avx2_params.py")
    sys.argv[0] = str(script)
    runpy.run_path(str(script), run_name="__main__")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

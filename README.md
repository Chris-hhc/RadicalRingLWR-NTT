# RadicalRingLWR-NTT

Small, self-contained benchmarks for **forward NTT** and **inverse NTT** paths used in Radical Ring-LWR style parameter sets. The tree holds four sibling projects:

| Directory       | Role |
|-----------------|------|
| `ntt_Mithril/`  | KEM forward NTT: scalar `poly_ntt32` vs `ntt_avx` |
| `intt_Mithril/` | KEM inverse NTT: scalar `poly_invntt32` vs fused `intt_avx` |
| `ntt_Octarine/` | Sign forward NTT (two primes): scalar vs AVX |
| `intt_Octarine/` | Sign inverse NTT (two primes): scalar `poly_invntt32` vs fused `intt_avx` |

---

## Ring dimension, NTT moduli, and AVX layer grouping

Unless you override `-DRRLWR_SECURITY_LEVEL`, the numbers below are the **128-bit** branch in each tree’s `parameters.h`.

### Mithril (`ntt_Mithril/`, `intt_Mithril/`)

- **Ring length** `N` = **128** → **7** radix-2 NTT layers (`log2 N`).
- **NTT modulus** (Montgomery field for butterflies): **`RRLWR_PKE_PRIME`** = `0x3fff7801` (single prime for this KEM-sized demo).

**Forward `ntt_avx`** (`ntt.S`): **1 + 6** stage split — `poly_ntt32_top1_redlow_avx2` (first layer) then `poly_ntt32_bottom6_dilithium_redlow_avx2` (layers 1–6), with twiddles laid out for AVX2.

**Inverse `intt_avx`** (`intt.S`): **6 + 1** — `poly_intt32_first6_levels0t5_redlow_avx2` (inverse layers **0–5**), then the **last** inverse layer (**layer 6**) via either `poly_intt32_last1_final_normal_avx2` or `poly_intt32_last1_final_fused_avx2`. A **meta word** at the end of the packed INTT parameter blob (`INTT_AVX_PARAM_META_BYTES`) chooses **normal vs fused** tail; the fused path uses a tighter `gs_final_cross_fused` pattern for that final stage.

### Octarine (`ntt_Octarine/`, `intt_Octarine/`)

- **Ring length** `N` = **1024** → **10** radix-2 layers.
- **Two NTT primes** (independent Montgomery fields, two root tables): **`RRLWR_SIGN_PRIME1`** = `0x3fff7801`, **`RRLWR_SIGN_PRIME2`** = `0x3fff5801`. Signing uses separate coefficient moduli `Q = 2^24`, `P = 2^21` at this level; those are **not** the NTT primes above.

**Forward `ntt_avx`** (`ntt.S`): **2 + 2 + 6** — `poly_ntt32_top2_redlow_avx2`, `poly_ntt32_middle2_redlow_avx2`, then `poly_ntt32_bottom6_dilithium_redlow_avx2` (the generator comment in `ntt_param.h` maps packed constants to **L0–L3** in the head block and **L4–L9** in the bottom-6 block).

**Inverse `intt_avx`** (`intt.S`): **6 + 2 + 2** — `poly_intt32_first6_levels0t5_redlow_avx2` (layers **0–5**), `poly_intt32_layers6_7_redlow_avx2` (**6–7**), then **layers 8–9** as either `poly_intt32_layers8_9_final_normal_avx2` or `poly_intt32_layers8_9_final_fused_avx2`, again selected by the parameter **meta** word. The **fused** tail merges work across the last two inverse layers (`gs_final_cross_fused` on register pairs); fused twiddles are produced from `intt_final_fused_tables.h` into the `*_FUSED_AVX_PARAMS*` tables in `intt_param.h`.

---

## Prerequisites

- **GCC** (or compatible) with **AVX2** (`-mavx2`)
- **Python 3** (only if you regenerate headers from `scripts/`)
- **Linux** recommended for `clock_gettime` / stable tooling (cycle benchmarks use **RDTSC**)

---

## How to build

Each subproject has its own `Makefile`. From the repository root:

```bash
make -C ntt_Mithril test
make -C intt_Mithril test
make -C ntt_Octarine test
make -C intt_Octarine test
```

This compiles the corresponding test binary under that project’s `test/` directory (see each `Makefile` for the exact output path).

To force a full rebuild:

```bash
make -C ntt_Mithril clean test
# …same pattern for the others
```

Regenerating auto-generated headers (only when you change parameters or scripts) is documented in each subproject’s `Makefile` (`gen_*` targets).

---

## How to run tests and read performance numbers

1. **Build** with `make -C <dir> test` as above.
2. **Run** the produced executable, for example:

```bash
./ntt_Mithril/test/ntt_test
./intt_Mithril/test/test_invntt_precalcu
./ntt_Octarine/test/test_ntt32_final_avx_vs_original
./intt_Octarine/test/test_intt_avx_fused_vs_poly_invntt
```

Each program:

1. Runs a **correctness** phase (random inputs, comparing reference vs AVX under the project’s equivalence rule).
2. Runs a **microbenchmark** that measures **CPU cycles** (not wall-clock nanoseconds):
   - **Warm-up:** 1000 iterations of `copy + transform` per implementation (stabilizes caches and branch predictors as far as possible on a shared host).
   - **Timed samples:** 100000 iterations; each iteration records timestamps from **`rdtsc`** around `copy + transform`.
   - **Processing:** consecutive timestamp differences are converted to per-iteration cycle deltas and corrected with a measured **RDTSC overhead** (same scheme as typical SUPERCOP-style helpers in this repo’s `test/cpucycles.c` / `test/speed_print.c`).
   - **Reporting:** **median** and **average** cycles per iteration are printed, plus **speedup** = (reference cycles) / (AVX cycles) using both median- and average-based ratios where applicable.

**Important:** On **burstable** instances (e.g. `t2.medium`), absolute cycle counts and TSC behaviour can vary with CPU credits and noisy neighbours. Treat the table below as **one representative run** on the stated machine, not a formal SLA.

---

## Measurement environment

- **Cloud:** Amazon Web Services **EC2 `t2.medium`** (2 vCPUs, burstable general-purpose).
- **CPU (as reported by `lscpu` on that instance):**  
  **Intel(R) Xeon(R) CPU E5-2686 v4 @ 2.30GHz**  
  **2** logical CPUs, **AVX2** present (`avx2` in flags), **Xen** hypervisor, little-endian **x86_64**.

The following table was filled by running the four binaries once after `make test` in each directory (same host as above).

---

## Performance comparison (representative run)

All figures are **cycles per iteration** of **`memcpy`-style copy + NTT/INTT`**, after overhead correction, with **1000** warm-up iterations and **100000** timed samples. **Speedup** = reference / AVX (higher is better for AVX).

### `ntt_Mithril` — KEM forward NTT (`ntt_test`)

| Implementation | Median (cycles) | Average (cycles) |
|----------------|-----------------|-------------------|
| `poly_ntt32` (reference) | 3159 | 4189 |
| `ntt_avx` | 306 | 491 |
| **Speedup (median / average)** | **10.32×** | **8.53×** |

### `intt_Mithril` — KEM inverse NTT (`test_invntt_precalcu`)

| Implementation | Median (cycles) | Average (cycles) |
|----------------|-----------------|-------------------|
| `poly_invntt32` (reference) | 3626 | 4705 |
| `intt_avx` (fused tail) | 310 | 474 |
| **Speedup (median / average)** | **11.70×** | **9.93×** |

### `ntt_Octarine` — Sign forward NTT (`test_ntt32_final_avx_vs_original`)

| Prime | `poly_ntt32` median | `ntt_avx` median | Speedup (median) | Speedup (average) |
|-------|---------------------|------------------|------------------|-------------------|
| prime1 | 31245 | 4181 | 7.47× | 7.38× |
| prime2 | 31217 | 4194 | 7.44× | 7.86× |

### `intt_Octarine` — Sign inverse NTT, fused AVX (`test_intt_avx_fused_vs_poly_invntt`)

| Prime | `poly_invntt32` median | `intt_avx` median | Speedup (median) | Speedup (average) |
|-------|------------------------|-------------------|------------------|-------------------|
| prime1 | 36868 | 3662 | 10.07× | 10.66× |
| prime2 | 36768 | 3670 | 10.02× | 8.96× |

---

## Licence / upstream

Source files in subprojects retain their original headers and licences where present (e.g. NXP Apache-2.0 boilerplate in some components). See each subtree for details.

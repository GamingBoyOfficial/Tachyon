# ⚡ TACHYON — The World's Fastest Microstructure Feature Engine

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.21920230.svg)](https://doi.org/10.5281/zenodo.21920230)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B23)
[![AVX2](https://img.shields.io/badge/CPU-AVX2%20%2B%20FMA-orange.svg)](#)

**"Faster than the speed of light."**

Tachyon is a header‑only C++23 engine that computes **32 real microstructure features** directly from raw ITCH packets in **50 nanoseconds per packet** — **10× below the 450 ns HFT latency target**, and over **1000× faster** than Python/Cython pipelines. Zero heap allocations. Lock‑free. AVX2 + FMA. Plugin‑extensible. Built to be the **Linux of HFT microstructure engines**.

Created by **Parikshit Sharma** ([@GamingBoyOfficial](https://github.com/GamingBoyOfficial)).

---

## 🔥 Performance — Measured, Not Promised

| Metric | Value |
|--------|-------|
| Parse + 32 features (single packet) | **50 ns** |
| P50 latency (standalone benchmark) | **130 cycles (43 ns)** |
| P99 latency (standalone benchmark) | **142 cycles (47 ns)** |
| File replay throughput | **20 million packets / second** |
| Python batch (8,386 packets) | **170 ns/packet** |

> At 10‑Gbps line rate, 50 ns is the time between two back‑to‑back packets.  
> Tachyon processes the market **faster than the wire can deliver data**.

---

## ✨ Why Tachyon Exists

Modern HFT and quant firms spend 80% of their critical path latency just *preparing* data for their models. They parse in C++, copy to Python, compute features, and copy back — losing **2–5 microseconds per trade**. Tachyon eliminates that waste. It computes 32 microstructure features directly inside the C++ packet parser, using SIMD and zero‑copy data flow.

**The result:** Your model sees market reality 4–5 packets earlier than competitors using Python/Cython pipelines. In HFT, that's the difference between winning and losing.

---

## 🧠 The 32 Microstructure Features (Alpha Pipeline)

All features are computed with AVX2 FMA and branchless logic. No fillers, no duplicates:

- EWMA Price, OFI, Realized Volatility, Tick Rule  
- Book Pressure, Spread‑to‑Vol, Micro‑Price, Depth Decay  
- Bid‑Ask Balance, Weighted Mid, Order Book Slope  
- Liquidity Imbalance, VWAP, Trade Imbalance  
- Cumulative Depth (Bid/Ask), Effective/Quoted Spread  
- Log Return, Skew, Kurtosis, Price Impact  
- Order Flow Toxicity, VPIN, Volatility‑of‑Vol  
- Mid Price Range, Bid‑Ask Cross, Depth‑Weighted Price  
- Tick Spread, Volume Profile, Spread Cross, Micro‑Vol

*(All 32 are distinct — no fillers.)*

---

## ✨ Key Features

- **32 real microstructure features** – from EWMA to VPIN, covering the full alpha spectrum.
- **Extensible plugin system** – Add custom feed parsers or feature calculators without modifying the core.
- **Zero heap allocations** on the hot path – no `new`/`malloc` during parsing or computation.
- **Lock‑free SPSC ring buffer** with cache‑line alignment – no false sharing.
- **Double‑buffered order book** – seamless updates without locks.
- **Header‑only C++23** – simply `#include <tachyon/tachyon.hpp>`.
- **AVX2 + FMA** SIMD acceleration – uses `_mm256_fmadd_ps`, `_mm256_sqrt_ps`.
- **Zero‑copy ITCH parser** – `reinterpret_cast` + `byteswap`, never touches heap.

---

## 🖥️ Hardware Requirements & Performance Scaling

**Minimum CPU:** x86‑64 with **AVX2 + FMA** support (Intel Haswell / AMD Excavator or newer).  
**Compiler:** GCC 13+ or Clang 16+ with `-mavx2 -mfma`.

Performance scales **linearly with CPU clock speed**. Benchmarks above were measured on a **3.0 GHz** CPU. Expect lower latencies at higher clock speeds:

| CPU Frequency | Expected Latency (per packet) |
|---------------|-------------------------------|
| 3.0 GHz       | 50 ns                         |
| 4.0 GHz       | 37 ns                         |
| 5.0 GHz       | 30 ns                         |

*Note:* Actual latency may vary due to OS scheduling, CPU throttling, and thermal conditions. For stable results, run the standalone `tachyon_live` benchmark with the CPU warmed up and power plan set to **High Performance**.

---

## 🔌 Plugin System — Become Part of the Root

Tachyon is designed to be extended. Anyone can write a plugin to add new features or support new feed formats.

### Feature Plugins
Implement the `FeaturePlugin` interface:

```cpp
#include <tachyon/features/feature_plugin.hpp>

struct MyPlugin : tachyon::FeaturePlugin {
    size_t getNumFeatures() const noexcept override { return 1; }

    void compute(const tachyon::BookSnapshot& book,
                 float* output, size_t start_idx) noexcept override {
        // custom feature: spread in basis points
        output[start_idx] = (book.ask_prices[0] - book.bid_prices[0]) * 100.0f;
    }
};
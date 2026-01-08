# Intro to Data‑Oriented Design for Games – Summary

## 1️⃣ Why CPU performance matters
- Modern games still suffer from **CPU‑limited frames** (e.g., Starfield, Cyberpunk 2077).  
- GPUs get huge speed‑ups each generation; single‑core CPU performance has **plateaued**.  
- A slow CPU routine today will be just as slow on future hardware → we must write fast, cache‑friendly code.

## 2️⃣ The memory hierarchy (cost in CPU cycles)

| Level          | Typical Size | Latency (≈) |
|----------------|--------------|------------|
| **Register**   | < 8 bytes    | 0 cycles (already there) |
| **L1 Cache**   | ~64 KB       | ~3 cycles |
| **L2 Cache**   | ~2 MB        | ~20 cycles |
| **L3 Cache**   | ~32 MB       | ~100 cycles |
| **Main RAM**   | GBs          | 200‑300 cycles |
| **SSD / Disk** | TBs          | ~300 000 cycles |

*Data moves upward through this chain; each step is dramatically more expensive.*

## 3️⃣ Core data‑oriented rules of thumb

| Rule                | What it means for code |
|---------------------|------------------------|
| **Pack data tightly** | Use the smallest types, keep related fields together → maximize useful bytes per 64‑byte cache line. |
| **Do work in bulk**   | Set up once (e.g., load a cache line) then process many items before moving on. |
| **Pre‑compute / “bake”** | Anything that can be calculated ahead of time (lighting, navigation meshes, etc.) should be – it removes per‑frame work. |
| **Parallelise when possible** | Use multiple cores; each core has its own L1 cache, so keep tasks independent to avoid contention. |

## 4️⃣ Practical optimizations demonstrated

| # | Change | Why it helps | Typical speed‑up |
|---|--------|--------------|-----------------|
| **1** | Bulk processing – replace per‑entity `Update()` with a function that processes an array of enemies. | Removes virtual‑call overhead, improves data locality. | ~10 % |
| **2** | Remove callbacks – inline the “on death” logic instead of calling a delegate. | Prevents unpredictable cache evictions and extra indirection. | ~10 % |
| **3** | Hoist invariants – cache global config values outside the loop. | Fewer loads from memory each iteration. | ~10 % |
| **4** | Use `struct`s (value types) for enemies. | Data becomes contiguous → better cache‑line utilization. | ~30 % |
| **5** | Align fields – order members largest→smallest (`string id` first, then booleans). | Reduces padding waste (24 B → 16 B struct). | ~10 % |
| **6** | Replace bools with an enum/byte. | Keeps struct size small; still fits nicely after alignment. | negligible alone but helpful when scaling. |
| **7** | Drop strings for IDs – use `uint` instead of `string`. | Removes large heap allocations and extra pointer chasing. | ~20 % |
| **8** | Swap‑back delete – copy the last element over a removed slot rather than shifting all following items. | Eliminates O(N) moves per removal. | 3‑4× faster deletions. |
| **9** | Separate arrays by state (normal, frenzy, dead). | No per‑entity `if` checks; state is implicit in the container. | ~2× overall speedup. |

Combining all of the above turned a naïve O(N) loop (~300 µs) into a **~40× faster** implementation (≈7–8 µs).

## 5️⃣ Take‑away concepts

1. **Cache‑line awareness** – think “what 64 bytes will I load?” and make those bytes all useful work.  
2. **Data layout matters more than algorithmic Big‑O** for tight loops; a well‑packed struct can beat a fancy algorithm that thrashes memory.  
3. **Eliminate indirection** (pointers, virtual calls, strings) whenever possible.  
4. **Batch and pre‑compute** – set up once, then iterate over contiguous data many times.  
5. **Leverage the hardware** – use multiple cores, keep each core’s working set in its own L1/L2 caches.

## 6️⃣ Quick checklist for a new system

- [ ] Use `struct`s (or tightly packed POD types) for per‑entity data.  
- [ ] Order fields by size to minimize padding.  
- [ ] Replace booleans with a single small enum/byte if they represent mutually exclusive states.  
- [ ] Store IDs as integers, not strings.  
- [ ] Process entities in **arrays**; avoid `List<T>.RemoveAt(i)` inside loops – use swap‑back or bulk removal.  
- [ ] Split data into separate containers when states diverge (e.g., active vs. inactive).  
- [ ] Cache any constant look‑ups outside inner loops.  
- [ ] Pre‑compute anything that can be baked offline.  
- [ ] Profile on target hardware; watch cache‑miss counters.

---

**Bottom line:** Data‑oriented design is about **matching the way the CPU works** – keep data together, minimize random memory accesses, and do as much work as possible per cache line. Applying these principles can turn a simple enemy‑update loop into an order‑of‑magnitude faster system, freeing cycles for richer gameplay or higher frame rates.

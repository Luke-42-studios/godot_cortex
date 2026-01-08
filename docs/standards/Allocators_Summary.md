# Memory Allocators – Concise Summary

---

## 📦 What Is an Allocator?
*An allocator **splits a limited resource** (memory) into smaller pieces and hands those pieces out on request.*  
Just like a government treasurer allocates portions of a budget, the operating system allocates chunks of RAM to programs, and inside a program other allocators further subdivide that memory.

---

## 1️⃣ Linear (Stack) Allocator – The Simplest One
```c
size_t next_offset = 0;               // start at address 0 in the arena
void *linear_alloc(size_t bytes){
    void *ptr = base + next_offset;   // give out current pointer
    next_offset += bytes;             // move forward
    return ptr;
}
void linear_free_to(size_t offset){   // rewind to a previous point
    next_offset = offset;
}
```
*Properties*
- **Only one variable** (`next_offset`).
- Allocation is O(1) – just bump the pointer.
- Deallocation can only happen **in reverse order** (last‑in, first‑out).  
- Perfect for **stacks**, function call frames, or any data structure that grows/shrinks at its tail.

*Fragmentation problem*: If an earlier allocation is freed while later ones remain, a *hole* appears that the linear allocator cannot reuse. This leads to internal fragmentation and eventual out‑of‑memory.

---

## 2️⃣ General OS‑Level Allocation (Free Lists / Pools)
The operating system does **not** give each process one huge contiguous block. Instead it maintains a pool of many **small blocks** (often called *pages*). When a program asks for, say, 5 GB:
1. The OS collects enough free pages to total the request.
2. It returns a list of those pages – they may be scattered throughout physical RAM.
3. When the process exits, all its pages are marked **free** again.

*Advantages*
- No need for a single contiguous region → less fragmentation.
- The OS can satisfy any size request by stitching together many small blocks.

---

## 3️⃣ Closed‑Addressing vs Open‑Addressing Analogy (for Allocators)
Think of an allocator as a **hash map** where:- *Key* = requested size/identifier. - *Value* = pointer to a free block.
- **Open addressing** would try to find the next free slot linearly – similar to linear allocation.
- **Closed addressing (separate chaining)** stores each free block in a linked list per bucket, allowing many blocks of the same size without linear scans.

---

## 4️⃣ Arena / Region Allocator (Fast Bulk Allocation)
```c
typedef struct {
    uint8_t *base;   // start of arena memory
    size_t   offset; // current top of used region
    size_t   capacity;
} Arena;

void *arena_alloc(Arena *a, size_t sz){
    if (a->offset + sz > a->capacity) return NULL; // out of space
    void *p = a->base + a->offset;
    a->offset += sz;   // bump pointer – O(1)
    return p;
}

void arena_reset(Arena *a){ a->offset = 0; } // free everything in one go
```
*When to use*
- When many objects share the **same lifetime** (e.g., all objects for a game level, or all temporary data during parsing).
- Deallocation is trivial – reset once.

---

## 5️⃣ Stack vs Heap – Where Do They Live?
| Area | Typical allocator |
|------|--------------------|
| **Call stack** (function frames) | *Linear / stack allocator* – each call pushes a frame, return pops it. |
| **Heap** (dynamic objects) | General purpose allocator (free‑list, buddy system, slab allocator, etc.). |

---

## 6️⃣ Common Production Allocators
| Name | Core Idea | Typical Use Cases |
|------|-----------|-------------------|
| **Buddy System** | Splits memory into power‑of‑two blocks; on free, buddies are merged. | General purpose OS kernels – low fragmentation, fast coalescing. |
| **Slab Allocator** | Pre‑creates caches of objects of the same size (slabs). | Kernel object allocation (e.g., file descriptors, sockets). |
| **TLSF (Two‑Level Segregated Fit)** | Constant‑time O(1) allocation/deallocation with low fragmentation. | Real‑time systems where deterministic latency is required. |
| **Memory Pool / Fixed‑Size Block** | Maintains a free list of fixed‑size chunks; very fast. | Embedded/real‑time code, game object pools. |

---

## 7️⃣ Deallocation Nuances
- **Linear allocator** can only “rewind” to a previous offset – you must know the exact point where the last allocation began.
- **General heap allocators** store metadata (size, possibly a back‑pointer) with each block so they can free any block independently.
- **Fragmentation types**
  * *Internal*: wasted space inside an allocated block because of alignment or rounding up.
  * *External*: free holes scattered throughout the arena that cannot satisfy larger requests.

---

## 8️⃣ Quick Cheat‑Sheet (C‑style pseudo code)
```c
// ---------- Linear / Stack Allocator -----------------
static uint8_t arena[1<<20]; // 1 MiB arena
static size_t top = 0;
void *lin_alloc(size_t sz){
    if (top + sz > sizeof(arena)) return NULL; // out of space
    void *p = arena + top;
    top += sz;
    return p;
}
void lin_free_to(size_t mark){ top = mark; } // rewind

// ---------- Simple Free‑List Allocator ---------------
typedef struct Block { size_t size; struct Block *next; } Block;
Block *free_list = NULL;

void *ff_alloc(size_t sz){
    Block **cur = &free_list;
    while (*cur && (*cur)->size < sz) cur = &(*cur)->next;
    if (!*cur) return NULL; // no suitable block
    void *p = (uint8_t*)*cur + sizeof(Block);
    // split block if large enough ...
    return p;
}
void ff_free(void *ptr){ /* add back to free list, possibly coalesce */ }
```
---

## TL;DR – Pick the Right Allocator for Your Use‑Case
| Situation | Recommended allocator |
|-----------|------------------------|
| Need **fast push/pop** at one end (function calls, temporary buffers) | **Linear / stack allocator**. |
| Many objects with **different lifetimes**, need arbitrary free | General purpose **free‑list / buddy / slab** allocator. |
| Bulk allocation for a *single* lifetime (e.g., level load) | **Arena / region allocator** – reset once. |
| Real‑time constraints, deterministic O(1) latency | **TLSF**, **Buddy** with careful tuning. |
| Fixed‑size objects reused frequently (particle systems, entity pools) | **Memory pool / slab** of that size. |

Remember: *allocation = handing out a slice of a pre‑reserved memory arena; deallocation = returning that slice.* Understanding the underlying array and the pointer bump (`next_offset`) is all you need to grasp any allocator’s core behavior.

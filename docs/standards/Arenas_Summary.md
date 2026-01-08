# Memory Arenas – Concise Summary

---

## 📚 What Is a Memory Arena?
An **arena (or region)** is a *large pre‑allocated block of memory* from which you carve out many small objects.  
When you’re done with all those objects, you free the **entire arena in one operation** instead of deallocating each object individually.

### Why Use an Arena?
- **Speed** – Allocation is just a pointer bump (`offset += size`).
- **Simplicity** – No per‑object `free` calls → no risk of forgetting to free (memory leaks).
- **Cache‑friendly** – All objects allocated together are spatially close, improving locality.

---

## 1️⃣ Arenas vs. Traditional Per‑Object Allocation
| Approach | How it works | Typical cost |
|----------|--------------|-------------|
| **Per‑object (`new`/`malloc`)** | Each object gets its own block; you must `delete`/`free` each one. | O(1) allocation, but many calls → fragmentation & bookkeeping overhead. |
| **Arena** | Allocate a big chunk once, then hand out pieces by moving an offset pointer. | Allocation O(1) *and* deallocation is O(1) for the whole arena.

---

## 2️⃣ Stack Frames Are Built‑In Arenas
When you call a function:
```c
void foo(){
    int x = 5;          // local variable – lives on stack
    char buf[64];       // another local – also on stack
}
```
All locals, arguments, and the return address are stored **contiguously** in a *stack frame*.
- The frame is allocated when `foo` is entered (pointer bump).
- It is automatically reclaimed when `foo` returns (pointer rewind).
Thus every function call already uses an arena – just called a **stack allocator**.

---

## 3️⃣ Typical Use‑Case: Per‑Request Arena in a Server
1. A client makes an HTTP request.
2. Allocate a temporary arena for that request.
3. Parse the request, build response strings, allocate any helper objects – all from the same arena.
4. Send the response.
5. **Reset** the arena → all per‑request memory is freed at once.

Benefits:
- No need for a garbage collector.
- Zero risk of leaking request‑specific buffers.
- Very low allocation overhead (just pointer arithmetic).

---

## 4️⃣ Simple C‑style Arena Implementation
```c
typedef struct {
    uint8_t *base;   // start of the memory region
    size_t   offset; // current top of used space
    size_t   capacity;
} Arena;

// initialise with a pre‑allocated buffer (or malloc it)
void arena_init(Arena *a, void *buf, size_t cap){
    a->base = buf;
    a->offset = 0;
    a->capacity = cap;
}

// allocate `sz` bytes – returns NULL if out of space
void *arena_alloc(Arena *a, size_t sz){
    // optional: align to pointer width for safety
    const size_t alignment = sizeof(void*);
    size_t aligned = (sz + alignment-1) & ~(alignment-1);
    if (a->offset + aligned > a->capacity) return NULL; // OOM in arena
    void *p = a->base + a->offset;
    a->offset += aligned;
    return p;
}

// reset – frees everything at once (no destructors run!)
void arena_reset(Arena *a){ a->offset = 0; }
```
*Usage example*
```c
uint8_t buffer[1<<20]; // 1 MiB static arena storage
Arena request_arena;
arena_init(&request_arena, buffer, sizeof(buffer));

char *msg = arena_alloc(&request_arena, 256);   // allocate a string buffer
// … use `msg` …

arena_reset(&request_arena); // all per‑request memory reclaimed instantly
```
---

## 5️⃣ When Not to Use an Arena
| Situation | Reason |
|-----------|--------|
| Objects with **different lifetimes** (e.g., a global cache that lives forever) | An arena frees *all* at once → can't keep long‑lived data. |
| Need **destructors / RAII cleanup** (file handles, sockets) | Arena does not run per‑object destructors; you must manage those manually. |
| Very large, irregular allocations that would waste most of the arena’s space | You may need a more flexible allocator (buddy system, slab). |

---

## 6️⃣ Combining Arenas with Other Allocators
- **Stack + Arena**: The call stack itself is an arena; you can allocate a *sub‑arena* inside a function for temporary work.
- **Arena + General Heap**: Allocate long‑lived objects from the heap, short‑lived groups from an arena.
- **Thread‑Local Arenas**: Each thread gets its own arena to avoid contention on a global allocator.

---

## 7️⃣ Performance Cheat‑Sheet (C‑like pseudo code)
```c
// ---------- Linear Stack Allocator (built‑in) ------------
void *stack_alloc(size_t sz){ return alloca(sz); } // gcc builtin – freed on function exit

// ---------- Custom Arena -------------------------------
Arena a;                     // declare arena struct
uint8_t mem[64*1024];       // 64 KB static buffer
arena_init(&a, mem, sizeof(mem));

int *numbers = arena_alloc(&a, 1000*sizeof(int));   // fast bulk allocation
// ... use numbers ...
aren a_reset(&a);            // O(1) free all
```
---

## TL;DR – When to Reach for an Arena
- **Bulk‑allocate many objects that share the same lifetime** (e.g., per‑frame, per‑request, per‑scene).  
- You want **fast allocation/deallocation** with minimal bookkeeping.  
- You can tolerate *no individual destructors* and are okay with freeing everything at once.

If those conditions hold, just allocate a big block up front, bump an offset for each object, and reset when the task finishes – that’s a memory arena in a nutshell.

# Tips for C Programming – Concise, Readable Summary

---

### 1️⃣ Choose the Right C Standard
| Version | Year | Why Use It |
|---------|------|------------|
| **C89/90** | 1989 | Most portable; compiles as C++ without changes. |
| **C99**   | 1999 | Allows mixed‑declaration & statements, designated initializers (`.field = value`), fixed‑size integer types (`uint32_t`, `uintptr_t`). |
| **C11 / C23** (optional) | newer | Adds atomics, threads, bounds‑checking extensions, etc. |

> *Use C99 for everyday work – the syntax is far cleaner than C89 and still universally supported.*  

### 2️⃣ Compile with Helpful Flags
```bash
clang -std=c99 -Wall -Wextra -Werror -fsanitize=address -g main.c -o prog
```
| Flag | What It Does |
|------|---------------|
| `-std=c99` | Selects the C version. |
| `-Wall -Wextra` | Enables many useful warnings. |
| `-Werror` | Turns *any* warning into a compile‑time error (forces you to fix them). |
| `-fsanitize=address` | Adds AddressSanitizer (detects out‑of‑bounds writes, use‑after‑free, double free, etc.). |
| `-g` | Generates debug info for the debugger. |

> *Keep these flags on during development; they catch bugs early and cost almost nothing in performance while debugging.*

### 3️⃣ Prefer a **Single Translation Unit** (Unity Build)
Instead of compiling many `.c` files separately, create one “master” file:
```c
/* main.c */
#include "foo.c"
#include "bar.c"
#include "baz.c"

int main(void) {
    // your code …
}
```
*Advantages*
- No header‑include guards needed for internal implementation files.
- Faster compile times for small to medium projects.
- Eliminates duplicate symbol/linker errors.

### 4️⃣ Use a Debugger (gdb / lldb / Visual Studio)
Running under a debugger gives you:
| Feature | Benefit |
|---------|--------|
| **Break on segfault** | Shows exact line & stack trace. |
| **Inspect variables**   | See values at crash time. |
| **Watchpoints**        | Detect when a specific memory location is written. |

*Tip*: In VS Code set `"stopOnEntry": true` and `"exceptionBreakpointFilters"` to break on `SIGSEGV`.

### 5️⃣ Detect Memory Errors with **AddressSanitizer (ASan)**
```bash
clang -fsanitize=address -g prog.c -o prog
./prog   # crashes with a clear message if you read/write out of bounds.
```
*What ASan catches*
- Buffer over‑/under‑flows.
- Use‑after‑free, double free.
- Accesses to freed memory.

*Cost*: ~2× slower runtime and extra memory; **never ship with ASan enabled**.

### 6️⃣ Safer Array API (Length + Bounds Check)
```c
typedef struct {
    size_t len;      // number of valid elements
    size_t cap;      // allocated capacity
    int *data;
} IntArray;

int intarray_get(const IntArray *a, size_t i) {
    if (i >= a->len) {  /* optional: breakpoint or abort */
        fprintf(stderr, "out‑of‑bounds access %zu (size=%zu)\n", i, a->len);
        abort();
    }
    return a->data[i];
}
```
*Why*: Guarantees you never read past `a->len`. Works nicely with ASan; the compiler can often eliminate the check in release builds.

### 7️⃣ Use **Indexes** Instead of Raw Pointers for Relationships
```c
typedef struct { /* objects */ } Obj;

typedef struct {
    size_t count;
    Obj *items;          // contiguous storage
} ObjArray;

/* Store relationships as indices */
size_t parent_of[NUM_OBJS];   // parent index, -1 = none (use SIZE_MAX)
```
*Benefits*
- 32‑bit index halves memory vs. a 64‑bit pointer on x86_64.
- Resizing the array doesn’t invalidate “pointers”.
- Easy to serialize – indices stay valid after reload.

### 8️⃣ Treat Strings as **Length‑Tagged Arrays**
```c
typedef struct {
    size_t len;
    char *bytes;   // not null‑terminated
} Str;

Str str_from_cstr(const char *s) {
    Str r = { .len = strlen(s), .bytes = strdup(s) };
    return r;
}
```
*Advantages*
- O(1) length queries (no `strlen` each time).
- No reliance on a terminating `'\0'`; safer slicing.

**String slice example**
```c
Str str_slice(Str s, size_t start, size_t len) {
    if (start + len > s.len) { abort(); }
    return (Str){ .len = len, .bytes = s.bytes + start };
}
```
No extra allocation needed – just a view into existing data.

### 9️⃣ Simple **Arena Allocator** for Shared Lifetimes
```c
typedef struct {
    uint8_t *base;
    size_t   offset;
    size_t   capacity;
} Arena;

void arena_init(Arena *a, size_t cap) {
    a->base = malloc(cap);
    a->offset = 0;
    a->capacity = cap;
}

void *arena_alloc(Arena *a, size_t sz, size_t align) {
    size_t pad = (align - (a->offset & (align-1))) & (align-1);
    if (a->offset + pad + sz > a->capacity) abort(); // out of memory
    void *ptr = a->base + a->offset + pad;
    a->offset += pad + sz;
    return ptr;
}

void arena_reset(Arena *a) { a->offset = 0; }   // frees everything at once
```
*When to use*: All data lives for the same “task” (e.g., parsing a file, one frame of a game). One bulk `free` instead of many tiny frees → far less overhead and better cache locality.

### 🔟 General Memory‑Management Mindset
| Lifetime Category | Typical Allocation Strategy |
|-------------------|-----------------------------|
| **Static** (program lifetime) | Global/static variables, or one large arena allocated at start. |
| **Function/Stack** | Automatic locals (`int x;`). |
| **Task‑Scoped** (e.g., document, frame) | Allocate a *single* arena per task, reset when done. |

> *Avoid scattering `malloc` / `free` all over the codebase. Group allocations by lifetime.*

## 📚 Quick Reference Cheat‑Sheet
```c
// 1️⃣ Compile with safety flags
clang -std=c99 -Wall -Wextra -Werror -fsanitize=address -g src.c -o prog

// 2️⃣ Define a length‑checked array type
typedef struct { size_t len, cap; int *data; } IntArray;

// 3️⃣ Simple get/set with bounds check
int ia_get(const IntArray *a, size_t i) {
    if (i >= a->len) abort();
    return a->data[i];
}

// 4️⃣ Use an arena for task‑scoped memory
Arena frame_arena;
arena_init(&frame_arena, 1<<20);          // 1 MiB per frame
Vertex *verts = arena_alloc(&frame_arena, n*sizeof(Vertex), alignof(Vertex));
...
arena_reset(&frame_arena);                // free all at once

// 5️⃣ Index‑based relationships (no raw pointers)
size_t parent_of[MAX_OBJS];   // use SIZE_MAX for “none”

// 6️⃣ Length‑tagged string
typedef struct { size_t len; char *bytes; } Str;
```

---

### TL;DR
1. **Pick C99**, compile with `-Wall -Wextra -Werror -fsanitize=address -g`.  
2. Use a **debugger** (gdb/lldb) – it turns segfaults into actionable info.  
3. Wrap arrays in a struct that stores *length* and *capacity*, provide safe accessor functions.  
4. Prefer **indexes** over raw pointers for intra‑array links; easier to serialize, safer on realloc.  
5. Treat strings as **length‑tagged buffers**, not null‑terminated C strings.  
6. Allocate per‑lifetime using an **arena allocator** – one bulk `malloc` + single reset instead of many tiny frees.  
7. Keep **ASan on** during development; it catches the hardest bugs (out‑of‑bounds, use‑after‑free).  

Follow these habits and C will feel far less “scary” while staying fast and expressive. Happy coding!

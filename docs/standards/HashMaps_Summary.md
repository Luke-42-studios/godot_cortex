# Hash Maps (Dictionaries) – Concise Summary

---

## 📌 Core Idea
*All hash‑map implementations sit on top of a **plain array**.  
The only thing we need to turn an arbitrary key (e.g., a string username) into an array index is a **hash function**.*

---

## 1️⃣ From Key → Index
| Step | What happens |
|------|---------------|
| **Hash** | `h = hash(key)` – deterministic function that mixes the key’s bytes into a single integer (usually 32‑bit). |
| **Compress** | `idx = h % capacity` – modulo reduces the huge integer to a valid array index (`capacity` is the size of our backing array). |

*Why modulo?* It maps any integer onto `[0, capacity‑1]`. The remainder pattern repeats (e.g., for `%3` we only ever get `0,1,2`).

---

## 2️⃣ Collision Reality
Even a perfect hash can’t guarantee uniqueness because:
- **Finite index space** – with a 32‑bit hash we have ~4 billion possible values.
- **More keys than slots** – real systems often have far fewer slots (e.g., 20 k). 
Thus *collisions* (different keys → same `idx`) are inevitable and must be handled.

---

## 3️⃣ Open Addressing (Linear Probing)
1. **Insert**
   ```c
   idx = hash(key) % cap;
   while (table[idx] occupied) {
       idx = (idx + 1) % cap;   // move to next slot (wrap around)
   }
   table[idx] = value;
   ```
2. **Lookup** – same walk until the key matches or an empty slot is found.
3. **Delete** – mark the slot as *deleted* (cannot simply clear it, otherwise look‑ups would stop early).

### Pros & Cons
| Pro | Con |
|-----|-----|
| Very memory‑compact – only one array. | Clustering: long runs of occupied slots degrade to near‑linear scans when the table gets full. |
| Simple implementation. | Deleting requires special “tombstone” handling; performance drops as **load factor** → 1.

---

## 4️⃣ Closed Addressing (Separate Chaining)
*Two parallel arrays:*
- `buckets[capacity]` – holds the index of the first entry in that bucket or `-1` if empty.
- `entries[maxEntries]` – each entry stores `{keyHash, value, nextIdx}`; `nextIdx` forms a linked list of colliding entries.

### Insert
```c
h   = hash(key);
idx = h % capacity;
int e = allocate_entry();
entries[e] = (Entry){ .keyHash=h, .value=v, .next=buckets[idx] };
buckets[idx] = e;               // new entry becomes head of chain
```
### Lookup / Delete
Follow the linked list (`nextIdx`) until a matching `keyHash`/key is found.

### Pros & Cons
| Pro | Con |
|-----|-----|
| No clustering – each bucket holds its own independent chain. | Requires two arrays + extra pointer (index) per entry, slightly higher memory overhead. |
| Deletion is O(1) once the node is located. | Cache‑locality suffers a bit because chains are scattered.

---

## 5️⃣ Load Factor & Resizing
`loadFactor = size / capacity`
- **Typical target**: ≤ 0.7 (70 %).
- When exceeded, allocate a *larger* backing array (often double the size) and **rehash** every existing entry:
```c
newCap = oldCap * 2;
for each entry e in oldTable {
    newIdx = hash(e.key) % newCap;
    // insert into new table using chosen collision strategy
}
```
Resizing is expensive (O(N)) but amortised over many inserts it’s acceptable.

---

## 6️⃣ Deletion Nuances (Open Addressing)
Simply clearing a slot breaks the probe sequence for later look‑ups. Solutions:
- **Tombstone marker** – mark as “deleted” and treat as occupied during probing, but free on future inserts.
- **Backward shift deletion** – after removing an element, shift subsequent cluster elements backwards while preserving correct lookup order (more complex).

---

## 7️⃣ Performance Cheat‑Sheet (C‑style pseudo code)
```c
// ---------- Open Addressing (Linear Probing) ------------
int table[CAP];          // -1 = empty, -2 = tombstone
int hash(const char *s); // returns uint32_t

int find_slot(const char *key){
    uint32_t h   = hash(key);
    size_t  idx = h % CAP;
    while (table[idx] != -1 && table[idx] != -2 && !key_eq(table[idx].key, key))
        idx = (idx + 1) % CAP; // linear probe
    return idx;
}

void put(const char *k, Value v){
    int i = find_slot(k);
    table[i] = (Entry){.hash=h,.key=k,.val=v,.next=-1};
    ++size;
    if ((float)size / CAP > 0.7) resize();
}

// ---------- Closed Addressing (Separate Chaining) ---------
int bucket[CAP];        // head index of chain, -1 = empty
Entry entries[MAX];
int freeList = 0;      // next free entry slot

void put_sc(const char *k, Value v){
    uint32_t h   = hash(k);
    size_t  b    = h % CAP;
    int e        = freeList++;
    entries[e]   = (Entry){.hash=h,.key=k,.val=v,.next=bucket[b]};
    bucket[b]    = e; // prepend to chain
    ++size;
    if ((float)size / CAP > 0.7) resize_sc();
}
```
---

## TL;DR – Choose the Right Variant
| Situation | Recommended strategy |
|-----------|----------------------|
| Memory‑critical, modest load factor (< 0.5) | **Open addressing** (linear/quadratic probing). |
| Expect many collisions / high load factor or need fast deletions | **Closed addressing** (separate chaining). |
| Need deterministic iteration order & cache friendliness | Open addressing with power‑of‑two capacity (use bitmask instead of `%`). |

Remember: *the hash function is the only thing that turns an arbitrary key into a number; modulo compresses it to a valid index; collision handling decides how you cope when two keys map to the same slot.*

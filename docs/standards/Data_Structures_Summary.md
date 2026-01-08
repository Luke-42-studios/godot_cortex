# Data Structures – Concise Summary

---

### 📚 What All Data Structures Have in Common
* **Underlying storage is always an array of memory** (a contiguous block of bytes).  
* The only operations the hardware provides are **read/write at an index**.  
* Every data structure is a *strategy* for mapping higher‑level concepts (lists, stacks, queues, trees, hash maps, graphs) onto those primitive reads/writes.

---

## 1️⃣ Array List (Dynamic Array)
| Feature | How it works |
|---------|--------------|
| **Storage** | Fixed‑size backing array (`capacity`). |
| **Size** | Separate `count` field – number of valid elements. |
| **Add at end** | Write to `array[count]`, then `count++`. |
| **Remove anywhere** | Shift all later elements left by one (O(N)). |
| **Insert anywhere** | Shift later elements right (O(N)). |

*Key insight*: Adding at the back is cheap; inserting/removing in the middle forces a linear copy.

---

## 2️⃣ Stack (LIFO)
* Same backing array as an Array List.  
* Only uses **the last element** (`top`).
```c
// push
array[count] = value;
++count;   // O(1)
// pop
--count;   // O(1) – no shifting needed
```
*Perfect for undo‑history, expression evaluation, etc.*

---

## 3️⃣ Queue (FIFO) – Circular Buffer
* Backing array + two indices: `start` and `end`.  
* **Dequeue** → `start = (start + 1) % capacity;`  
* **Enqueue** → `array[end] = value; end = (end + 1) % capacity;`
* The modulo (`%`) makes the buffer *wrap around*, eliminating costly shifts.

---

## 4️⃣ Linked List (Built on an Array)
Each element stores:
```c
struct Node {
    T data;
    int nextIdx;   // index of the following node, -1 = none
};
```
* **Insert / Remove** given a known predecessor → O(1) (just change two `nextIdx`).  
* **Find by position** requires walking from the head → O(N).  
* Advantage: No bulk memory moves; disadvantage: poor cache locality and slower random access.

---

## 5️⃣ Hash Map (Dictionary)
1. Compute a **hash** of the key → large integer.
2. Reduce to array index with `idx = hash % capacity` (same modulo trick as circular queue).
3. **Collision handling**:
   * **Open addressing** – linearly probe forward until an empty slot is found.
   * **Separate chaining** – keep two parallel arrays; the first stores the primary bucket, the second forms a linked list of colliding entries (each entry holds `nextIdx`).

*Separate‑chaining version reuses the linked‑list trick above, so many collisions become independent linked lists stored in the same secondary array.*

---

## 6️⃣ Tree (Hierarchical Structure)
A tree is essentially a **linked list where each node can have multiple children**.
```c
struct TreeNode {
    T data;
    int firstChildIdx;   // points into child‑array
    int nextSiblingIdx;  // linked‑list of siblings
};
```
* Traversal (pre‑order, post‑order) follows `firstChild` then `nextSibling`.  
* Memory layout is still two arrays – one for nodes, one for the sibling links.

---

## 7️⃣ Graph (Generalised Tree)
Same underlying representation as a tree; the only difference is **nodes may have multiple incoming edges**.
```c
struct Vertex {
    T data;
    int firstEdgeIdx;   // points into Edge array
};
struct Edge {
    int toVertexIdx;
    int nextEdgeIdx;    // linked list of outgoing edges from a vertex
};
```
* Allows traversal in any direction, useful for path‑finding (A*, Dijkstra).

---

## 8️⃣ Quick Cheat‑Sheet (C‑style pseudo‑code)
```c
// Array List -------------------------------------------------
int *arr = malloc(capacity * sizeof(int));
size_t count = 0;
void push_back(int v) { arr[count++] = v; }

// Stack -----------------------------------------------------
void stack_push(int v)   { push_back(v); }   // same as list add
int  stack_pop(void)    { return arr[--count]; }

// Circular Queue --------------------------------------------
size_t start = 0, end = 0;
void enqueue(int v){ arr[end] = v; end = (end+1)%capacity; }
int dequeue(void){ int v = arr[start]; start = (start+1)%capacity; return v; }

// Linked List (indices) ------------------------------------
struct Node{ int data; int next; };
Node nodes[MAX];
int head = -1; // empty list
void ll_insert_after(int prevIdx, int value){
    int newIdx = allocate_node();
    nodes[newIdx].data = value;
    nodes[newIdx].next = (prevIdx==-1) ? head : nodes[prevIdx].next;
    if(prevIdx==-1) head = newIdx; else nodes[prevIdx].next = newIdx;
}

// Hash Map – Separate chaining -----------------------------
struct Entry{ int keyHash; int value; int next; };
Entry entries[MAX];
int bucketHead[BUCKETS]; // -1 = empty
void hashmap_put(int key, int val){
    int h = hash(key) % BUCKETS;
    int e = allocate_entry();
    entries[e] = (Entry){ .keyHash=hash(key), .value=val, .next=bucketHead[h] };
    bucketHead[h] = e; // new entry becomes head of chain
}
```
---

### TL;DR – Choose the Right Structure for Your Access Pattern
| Access pattern | Best fit |
|----------------|----------|
| Random access by index (O(1)) | **Array / Dynamic Array** |
| Push/pop at one end only | **Stack** |
| FIFO order, constant‑time enqueue/dequeue | **Circular Queue** |
| Frequent inserts/removes *anywhere* with known neighbor | **Linked List** |
| Key → value lookup (average O(1)) | **Hash Map** |
| Hierarchical parent/children relationship | **Tree** |
| General network of relationships, path‑finding | **Graph** |

Remember: every structure is just a clever way to *interpret* the same underlying array memory. Understanding that core idea makes it easy to reason about performance and choose the right tool.

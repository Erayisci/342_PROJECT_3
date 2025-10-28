# PSTree Library 
**Project:** Process-Shared Binary Search Tree (PSTree)  
**Author:** Eray İşçi | Kemal Onur Özkan

---

## 📘 Project Overview
This project implements **`pstree`**, a user-level C library that provides a **shared binary search tree (BST)** abstraction for **multiple concurrent processes** on Linux.  
All data is stored in a **shared memory region**, and processes synchronize access via **POSIX mutexes and condition variables** residing in shared memory.

The aim is to explore:
- **Process-level synchronization**
- **Shared memory management**
- **Inter-process coordination without busy waiting**
---

## 🎯 Learning Objectives
- Build a **multi-process shared memory library** in C using `mmap()`.
- Implement safe **insert, delete, and lookup** operations on a shared BST.
- Design and use **custom allocators** for dynamic memory inside shared memory.
- Apply **mutexes and condition variables** with `PTHREAD_PROCESS_SHARED` attributes.
- Understand and avoid **race conditions** and **deadlocks** in concurrent access.

---

## 🧠 Core Concepts
### Shared Memory Region
- Created via `pst_create()` using `mmap()`.  
- Stores both management structures and BST nodes.  
- Accessible by all processes using the same tree name.

### Synchronization
- Each critical section (e.g., insertion/deletion) is protected by shared mutexes.  
- Read-only operations (like `get` or `findkeys`) can run concurrently.  
- Blocking behavior is implemented when memory is full — processes sleep until space frees.

### Memory Management
The library manually allocates and frees memory inside the shared region (no `malloc()` calls).  
Any strategy (e.g., bitmap or first-fit allocator) can be used internally.

---

## 🧩 Public API (Conceptual Summary)
| Function | Purpose |
|-----------|----------|
| `pst_create(char *treename, int memsize, int maxdatasize)` | Creates a new shared memory region and initializes the tree. |
| `pst_destroy(char *treename)` | Removes a shared tree and its memory region. |
| `pst_open(char *treename)` | Attaches an existing shared tree; returns a tree descriptor. |
| `pst_close(int td)` | Detaches the calling process from the shared tree. |
| `pst_insert(int td, long key, char *buf, int size)` | Inserts a new key–data pair into the tree. |
| `pst_update(int td, long key, char *buf, int size)` | Updates the data for an existing key. |
| `pst_delete(int td, long key)` | Deletes the node with the given key. |
| `pst_get(int td, long key, char *buf)` | Retrieves the data associated with a key. |
| `pst_findkeys(int td, long key1, long key2, int N, long keys[])` | Returns up to `N` keys within the inclusive range `[key1, key2]`. |
| `pst_get_nodecount(int td)` | Returns the current number of nodes. |
| `pst_get_maxdatasize(int td)` | Returns the maximum data length per node. |
| `pst_printerror()` | Prints a message corresponding to the latest library error. |

---

## 🧮 Example Scenario
Multiple processes (insertors, deleters, readers) can operate on the same shared tree:


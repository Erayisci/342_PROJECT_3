# User-Space Thread Library (TSL)
**Project:** – User-Level Thread Library (64-bit)  
**Author:** Eray İşçi | Kemal Onur Özkan

---

## Project Overview
This repository documents the development of a **user-space thread library (TSL)** implemented in **C on Linux (x86-64)**.  
The goal is to design a lightweight cooperative threading system that runs entirely in user space, allowing multiple logical threads to share a single kernel thread.

---

## Objectives
- Implement basic **thread creation, scheduling, and termination** in user space.  
- Understand **context switching** through `getcontext()` and `setcontext()` calls.  
- Manage **thread stacks**, **thread control blocks (TCBs)**, and cooperative scheduling.  
- Experiment with simple scheduling algorithms such as **FCFS** and **RANDOM**.  

---

## Conceptual Highlights
### Thread Control Block (TCB)
Each thread is represented by a TCB storing its ID, CPU context, stack pointer, and current state (READY, RUNNING, WAITING, ENDED).

### Context Switching
The library saves and restores CPU registers using standard C library calls rather than inline assembly, reinforcing low-level understanding of execution contexts.

### Cooperative Scheduling
Threads voluntarily yield the CPU using a `yield()`-style call.  
No kernel pre-emption or timer interrupts are used — only explicit yielding.

### Stub Function Mechanism
All threads start from a small wrapper (“stub”) that invokes the user-defined start function and ensures proper cleanup when it returns.

---

## 🧩 Implementation Summary
- **Language & Environment:** C on Ubuntu 22.04 (x86-64)  
- **Library Type:** Static/Shared C library linked with user programs  
- **Thread Limit:** Up to 128 threads (including main)  
- **Stack Size:** 32 KB per thread  
- **Scheduling Modes:** FCFS (1), RANDOM (2)  
- **Thread API Functions:**  
  - Initialization (`tsl_init`)  
  - Thread creation / termination  
  - Cooperative yield  
  - Join / cancel operations  
  - Thread ID query  


## Experimental Work
Performance experiments explored:
- Average context-switch cost  
- Thread-creation latency  
- Scheduler fairness under FCFS vs RANDOM  

Results and discussion appear in the private `report.pdf` accompanying the submission.

---

## Build and Run
Typical workflow on Linux:

```bash
make          # compile the library and test programs
./tsl_test    # run sample test application
make clean    # remove object and binary files


Repository Layout:
.
├── src/
│   ├── tsl.c           # core library implementation (private)
│   └── tsl.h           # public API header
├── tests/
│   ├── basic_test.c
│   └── scheduler_test.c
├── Makefile
├── report.pdf          # performance discussion (restricted)
└── README.md

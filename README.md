# 🚀 Thread-Safe In-Memory Virtual File System (VFS)

A high-performance, thread-safe in-memory file system implemented in **C++17**. This project simulates core operating system functionalities and is engineered to handle high-concurrency environments by implementing a **Readers-Writer Lock** synchronization pattern.

## 🧠 Core Architecture & System Design

This VFS is designed using the **Composite Design Pattern** to treat files and directories uniformly.

* **Concurrency Architecture:** Implemented a **Readers-Writer Lock pattern** using `std::shared_mutex`. This architecture maximizes throughput by allowing multiple threads to execute read-only operations (`ls`, `cat`, `pwd`) simultaneously, while ensuring exclusive write access (`mkdir`, `touch`, `rm`) to maintain data integrity.
* **N-ary Tree Structure:** Directories map to string keys using `std::unordered_map` for **O(1)** average time complexity for lookups.
* **Manual Memory Management:** The system utilizes raw pointers to simulate disk blocks, with custom recursive destructors ensuring **zero memory leaks** even when tearing down complex, nested directory structures.

## ⚙️ Features

* **Thread-Safe API:** All operations are protected by granular locks to prevent race conditions.
* **Navigation & Manipulation:** Supports `mkdir`, `cd`, `touch`, `ls`, and `pwd`.
* **File I/O:** Supports `write` (file content injection) and `cat` (read).
* **Recursive Deletion:** `rm` command ensures all child nodes are deallocated from heap memory.

## 🚀 Getting Started

### Prerequisites
* A C++17 compatible compiler (`g++`, `clang`, or MSVC).
* No external dependencies required.

### Compilation & Execution
Compile with C++17 standards to enable `std::shared_mutex`:

```bash
# Compile the source code
g++ -std=c++17 k.cpp -o vfs

# Run the concurrency stress test
./vfs
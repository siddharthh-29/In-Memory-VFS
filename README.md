# 🚀 Thread-Safe, Persistent Virtual File System (VFS)

A high-performance, **thread-safe, and persistent** in-memory file system implemented in C++17. This project simulates core OS functionalities, engineered for high-concurrency environments and data durability.

## 🧠 Core Architecture & System Design
* **Composite Design Pattern**: Treats files and directories uniformly, enabling recursive tree navigation and management.
* **Concurrency Architecture**: Implemented a **Readers-Writer Lock** pattern using `std::shared_mutex`. This maximizes throughput by allowing multiple concurrent reads (`ls`, `cat`) while ensuring exclusive access for writes (`mkdir`, `touch`), preventing race conditions.
* **Binary Serialization**: Engineered a custom binary framework to flatten the in-memory tree structure to disk, enabling **full system state reconstruction** across reboots.
* **Memory Management**: Utilizes recursive destructors to manage heap-allocated nodes, ensuring zero memory leaks and robust resource lifecycle management.

## ⚙️ Features
* **Thread-Safe API**: Granular locking for high-concurrency environments.
* **Persistence Layer**: `save()` and `load()` functionality to ensure data survives process termination.
* **Navigation & Manipulation**: Full suite of `mkdir`, `cd`, `touch`, `ls`, `pwd`, `cat`, and `writeFile` commands.
* **Recursive Deletion**: Clean deallocation of nested directory structures.

## 🚀 Getting Started

### Prerequisites
* A C++17 compatible compiler (g++, clang, or MSVC).
* No external dependencies required.

### Compilation & Execution
```bash
# Compile the source code
g++ -std=c++17 k.cpp -o vfs

# Run the system
./vfs
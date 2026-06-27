# 🚀 In-Memory Virtual File System (VFS)

A lightweight, POSIX-style in-memory file system implemented entirely in standard C++. This project simulates core operating system functionalities, including directory navigation, file manipulation, and dynamic path parsing, using a custom-built N-ary tree data structure.

## 🧠 Core Architecture & System Design

This VFS is designed using the **Composite Design Pattern** to treat files and directories uniformly, ensuring highly modular and scalable code.

* **N-ary Tree Structure:** Directories map to string keys using `std::unordered_map` for **O(1) average time complexity** when searching for child nodes.
* **Manual Memory Management:** Since the system relies heavily on raw pointers to simulate disk blocks, the `Directory` class implements custom destructors to perform **recursive memory deallocation**, guaranteeing zero memory leaks during tree teardown (e.g., when running `rm` on a populated directory).
* **Polymorphism:** A base `FileSystemNode` class abstracts the common properties (creation time, name, permissions) while `File` and `Directory` inherit and implement their specific behaviors.

## ⚙️ Features

The interactive shell supports the following core system commands:

* `pwd` - Print the current working directory.
* `ls` - List the contents of the current directory.
* `mkdir <dir_name>` - Create a new directory.
* `touch <file_name>` - Create a new empty file.
* `write <file_name> <text>` - Write string data to a file.
* `cat <file_name>` - Output the contents of a file to the shell.
* `cd <path>` - Navigate the file system (supports relative paths like `..` and `/`).
* `rm <target>` - Recursively delete a file or directory, freeing allocated heap memory.

## 🚀 Getting Started

### Prerequisites
* A C++ compiler (`g++`, `clang`, or MSVC)
* No external libraries or dependencies are required. It uses the C++ Standard Library natively.

### Compilation & Execution
Clone the repository and compile using `g++`:

```bash
# Compile the source code
g++ main.cpp -o vfs

# Run the interactive shell
./vfs
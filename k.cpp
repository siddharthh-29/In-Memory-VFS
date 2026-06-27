#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <chrono> // For simulating delays
#include <fstream>

// 1. Forward declaration so FileSystemNode knows Directory exists
class Directory;

// 2. Abstract Base Class
class FileSystemNode {
protected:
    std::string name;
    std::time_t creation_time;
    bool is_directory;
    Directory* parent; // Pointer to the parent directory for navigation

public:
    FileSystemNode(std::string n, bool is_dir, Directory* p = nullptr) 
        : name(n), is_directory(is_dir), parent(p) {
        creation_time = std::time(nullptr);
    }

    virtual void serialize(std::ostream& os) const = 0;
    
    virtual ~FileSystemNode() = default;

    std::string getName() const { return name; }
    bool isDirectory() const { return is_directory; }
    Directory* getParent() const { return parent; } 
    
    // Pure virtual function to force derived classes to implement their size calculation
    virtual int getSize() const = 0; 
};

// 3. File Class
class File : public FileSystemNode {
private:
    std::string content;

public:
    File(std::string n, Directory* p) : FileSystemNode(n, false, p), content("") {}

    void writeContent(const std::string& data) {
        content = data;
    }

    void serialize(std::ostream& os) const override {
    os << "F" << " " << name << " " << content.length() << " " << content << "\n";
}

    std::string readContent() const {
        return content;
    }

    int getSize() const override {
        return content.length();
    }
};

// 4. Directory Class
class Directory : public FileSystemNode {
private:
    // Maps a string (name) to a pointer of its child node
    std::unordered_map<std::string, FileSystemNode*> children;

public:
    Directory(std::string n, Directory* p = nullptr) : FileSystemNode(n, true, p) {}

    ~Directory() {
        // Automatically cascade deletion to all children to prevent memory leaks
        for (auto const& [key, val] : children) {
            delete val;
        }
    }

    void addNode(FileSystemNode* node) {
        children[node->getName()] = node;
    }
    
    void removeNode(const std::string& nodeName) {
        auto it = children.find(nodeName);
        if (it != children.end()) {
            // 1. Delete the object from RAM (Triggers the recursive destructors!)
            delete it->second; 
            
            // 2. Remove the pointer from our hash map so it disappears from 'ls'
            children.erase(it); 
        }
    }

    void serialize(std::ostream& os) const override {
    os << "D" << " " << name << " " << children.size() << "\n";
    for (auto const& [key, val] : children) {
        val->serialize(os); // Recursive call
    }
}

    FileSystemNode* getNode(const std::string& nodeName) {
        if (children.find(nodeName) != children.end()) {
            return children[nodeName];
        }
        return nullptr;
    }

    int getSize() const override {
        int totalSize = 0;
        for (auto const& [key, val] : children) {
            totalSize += val->getSize();
        }
        return totalSize;
    }

    void listContents() const {
        if (children.empty()) {
            std::cout << "  (empty)\n";
            return;
        }
        for (auto const& [key, val] : children) {
            // Print the name, and append a '/' if it's a directory
            std::cout << "  " << key << (val->isDirectory() ? "/" : "") << "\n";
        }
    }
};

// 5. The FileSystem Engine
class FileSystem {
private:
    Directory* root;
    Directory* current_directory;
    mutable std::shared_mutex vfs_mutex;

    // Helper for reconstruction
    void reconstruct(std::ifstream& ifs, Directory* current) {
        char type;
        std::string name;
        while (ifs >> type >> name) {
            if (type == 'D') {
                int numChildren;
                ifs >> numChildren;
                Directory* newDir = new Directory(name, current);
                current->addNode(newDir);
                for (int i = 0; i < numChildren; ++i) {
                    reconstruct(ifs, newDir);
                }
            } else if (type == 'F') {
                int length;
                std::string content;
                ifs >> length >> content;
                File* newFile = new File(name, current);
                newFile->writeContent(content);
                current->addNode(newFile);
            }
        }
    }

public:
    FileSystem() {
        root = new Directory("/", nullptr);
        current_directory = root;
    }

    ~FileSystem() {
        delete root;
    }

    // --- READ OPERATIONS ---
    void pwd() const {
        std::shared_lock<std::shared_mutex> lock(vfs_mutex);
        std::cout << "Current Directory: " << current_directory->getName() << std::endl;
    }

    void ls() const {
        std::shared_lock<std::shared_mutex> lock(vfs_mutex);
        current_directory->listContents();
    }

    void cat(const std::string& fileName) const {
        std::shared_lock<std::shared_mutex> lock(vfs_mutex);
        FileSystemNode* node = current_directory->getNode(fileName);
        if (node && !node->isDirectory()) {
            std::cout << "[Thread " << std::this_thread::get_id() << "] Read: " 
                      << static_cast<File*>(node)->readContent() << "\n";
        }
    }

    // --- WRITE OPERATIONS ---
    void mkdir(const std::string& dirName) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        if (!current_directory->getNode(dirName)) {
            current_directory->addNode(new Directory(dirName, current_directory));
        }
    }

    void touch(const std::string& fileName) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        if (!current_directory->getNode(fileName)) {
            current_directory->addNode(new File(fileName, current_directory));
        }
    }

    void writeFile(const std::string& fileName, const std::string& content) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        FileSystemNode* node = current_directory->getNode(fileName);
        if (node && !node->isDirectory()) {
            static_cast<File*>(node)->writeContent(content);
            std::cout << "[Thread " << std::this_thread::get_id() << "] Wrote to " << fileName << "\n";
        }
    }

    void rm(const std::string& targetName) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        current_directory->removeNode(targetName);
    }

    void cd(const std::string& path) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        if (path == "/") current_directory = root;
        else if (path == ".." && current_directory->getParent()) current_directory = current_directory->getParent();
        else {
            FileSystemNode* node = current_directory->getNode(path);
            if (node && node->isDirectory()) current_directory = static_cast<Directory*>(node);
        }
    }

    // --- PERSISTENCE ---
    void save(const std::string& filename) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        std::ofstream ofs(filename);
        if (ofs) root->serialize(ofs);
    }

    void load(const std::string& filename) {
        std::unique_lock<std::shared_mutex> lock(vfs_mutex);
        std::ifstream ifs(filename);
        if (!ifs) return;
        delete root;
        root = new Directory("/", nullptr);
        current_directory = root;
        reconstruct(ifs, root);
    }
};

int main() {
    FileSystem vfs;
    
    // 1. Create structure
    vfs.mkdir("projects");
    vfs.touch("projects/todo.txt");
    vfs.writeFile("projects/todo.txt", "Task: Finish persistence layer");

    // 2. Test Persistence: Save
    std::cout << "\n--- Saving State ---\n";
    vfs.save("system_state.dat");

    // 3. Simulate System Shutdown and Reboot
    std::cout << "Simulating Reboot...\n";
    vfs.~FileSystem(); // Manually clear
    
    // 4. Test Persistence: Load
    FileSystem newVfs;
    newVfs.load("system_state.dat");
    std::cout << "Reboot Complete. Checking restored files:\n";
    newVfs.ls(); 

    // 5. Test Concurrency: Stress test the loaded system
    std::cout << "\n--- Starting Concurrent Stress Test on Loaded System ---\n";
    auto writer = [&newVfs]() { newVfs.writeFile("projects/todo.txt", "Task: Done!"); };
    std::thread t1(writer);
    std::thread t2(writer);
    t1.join();
    t2.join();


    

    std::cout << "Test Passed. System is persistent AND thread-safe.\n";

        // Add this right before the final "return 0;"
    std::cout << "Press Enter to exit...";
    std::cin.ignore();
    std::cin.get();
    return 0;
}
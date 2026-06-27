#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>

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

public:
    FileSystem() {
        // Boot up: Create the root directory (it has no parent, so nullptr)
        root = new Directory("/", nullptr);
        current_directory = root; 
    }

    ~FileSystem() {
        delete root; 
    }

    void pwd() {
        // For now, this just prints the current folder name. 
        // Later we can upgrade this to print the full path recursively!
        std::cout << "Current Directory: " << current_directory->getName() << std::endl;
    }

    void ls() {
        current_directory->listContents();
    }

    void mkdir(const std::string& dirName) {
        // Check if it already exists
        if (current_directory->getNode(dirName) != nullptr) {
            std::cout << "mkdir: cannot create directory '" << dirName << "': File/Folder exists\n";
            return;
        }

        // Create the new folder and pass current_directory as the parent
        Directory* newDir = new Directory(dirName, current_directory);
        current_directory->addNode(newDir);
    }

    void cd(const std::string& path) {
        // Case 1: Go straight to root
        if (path == "/") {
            current_directory = root;
            return;
        }
        
        // Case 2: Go up one level (parent)
        if (path == "..") {
            if (current_directory->getParent() != nullptr) {
                current_directory = current_directory->getParent();
            }
            return;
        }

        // Case 3: Go into a child folder
        FileSystemNode* targetNode = current_directory->getNode(path);
        
        if (targetNode == nullptr) {
            std::cout << "cd: no such file or directory: " << path << "\n";
        } 
        else if (!targetNode->isDirectory()) {
            std::cout << "cd: not a directory: " << path << "\n";
        } 
        else {
            // It exists and is a directory, so we cast it and move into it
            current_directory = static_cast<Directory*>(targetNode);
        }
    }


    void touch(const std::string& fileName) {
        // 1. Check if a file or folder with this name already exists
        if (current_directory->getNode(fileName) != nullptr) {
            std::cout << "touch: cannot create '" << fileName << "': File or Directory exists\n";
            return;
        }

        // 2. Create the file and add it to the current directory
        File* newFile = new File(fileName, current_directory);
        current_directory->addNode(newFile);
    }

    void writeFile(const std::string& fileName, const std::string& content) {
        FileSystemNode* node = current_directory->getNode(fileName);

        if (node == nullptr) {
            std::cout << "write: no such file: " << fileName << "\n";
            return;
        }
        
        if (node->isDirectory()) {
            std::cout << "write: cannot write to a directory: " << fileName << "\n";
            return;
        }

        // It exists and is a file, so we safely cast it and write the content
        File* file = static_cast<File*>(node);
        file->writeContent(content);
        std::cout << "Written to " << fileName << " successfully.\n";
    }

    void cat(const std::string& fileName) {
        FileSystemNode* node = current_directory->getNode(fileName);

        if (node == nullptr) {
            std::cout << "cat: no such file: " << fileName << "\n";
            return;
        }
        
        if (node->isDirectory()) {
            std::cout << "cat: " << fileName << " is a directory\n";
            return;
        }

        File* file = static_cast<File*>(node);
        std::cout << file->readContent() << "\n";
    }

    void rm(const std::string& targetName) {
        // 1. Check if the file/folder actually exists in the current directory
        if (current_directory->getNode(targetName) == nullptr) {
            std::cout << "rm: cannot remove '" << targetName << "': No such file or directory\n";
            return;
        }

        // 2. Erase it!
        current_directory->removeNode(targetName);
    }
};

// 6. The Interactive Shell
int main() {
    FileSystem vfs;
    std::string command, argument;

    std::cout << "Virtual File System booted. Type 'exit' to quit.\n";

    while (true) {
        std::cout << "$ "; // The terminal prompt
        std::cin >> command;

        if (command == "exit") {
            break;
        } 
        else if (command == "pwd") {
            vfs.pwd();
        } 
        else if (command == "ls") {
            vfs.ls();
        } 
        else if (command == "mkdir") {
            std::cin >> argument; // Read the folder name
            vfs.mkdir(argument);
        }
        else if (command == "cd") {
            std::cin >> argument; // Read the target path
            vfs.cd(argument);
        }

        else if (command == "touch") {
            std::cin >> argument;
            vfs.touch(argument);
        }
        else if (command == "write") {
            std::cin >> argument; // This gets the filename
            std::string content;
            std::getline(std::cin, content); // This gets the rest of the line (the text to write)
            
            // Remove the leading space that getline picks up
            if (!content.empty() && content[0] == ' ') {
                content = content.substr(1);
            }
            vfs.writeFile(argument, content);
        }
        else if (command == "cat") {
            std::cin >> argument;
            vfs.cat(argument);
        }
        else if (command == "rm") {
            std::cin >> argument;
            vfs.rm(argument);
        }
        else {
            std::cout << "Command not found: " << command << "\n";
            // Clear the rest of the line in case they typed garbage
            std::cin.ignore(256, '\n'); 
        }
    }

    std::cout << "System shutting down safely.\n";
    return 0;
}
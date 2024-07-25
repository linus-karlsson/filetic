
# Lightweight File Management Application

## Overview

A lightweight file management application designed to provide basic file operations with minimal resource consumption. It is ideal for users who need a simple and efficient way to manage files without the overhead of more complex solutions.

## Platform

- **Windows**: Works only on Windows (might expand later).

## Features (to be implemented)

- **File Browsing**: Navigate through directories and view file contents.
- **File Operations**: Perform basic file operations such as copy, paste, read, delete, and rename.
- **Search Functionality**: Quickly find files and directories based on name.
- **Lightweight**: Minimal dependencies and low memory usage.

## Getting Started

### Prerequisites

- OpenGL
- CMake or Ninja

### Installation

1. **Clone the repository**
    ```bash
    git clone --depth 1 --recurse-submodules https://github.com/linus-karlsson/filetic.git
    cd filetic
    ```

2. **Build using CMake**
    ```bash
    .\Commands\Make\config.bat
    cmake --build build --config Debug
    ```
   **Build using Ninja**
    ```bash
    .\Commands\Ninja\configDebug.bat
    cmake --build build
    ```

## Usage

### Running the Application

 **Running using CMake**
 ```bash
 .\build\bin\Debug\FileTic.exe
 ```

 **Running using Ninja**
 ```bash
 .\build\bin\FileTic.exe
 ```

## License

This project is licensed under the [Apache License](LICENSE).

---

Thank you for reading this!

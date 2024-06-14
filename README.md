
# Lightweight File Management Application

## Overview

A lightweight file management application designed to provide basic file operations with minimal resource consumption. It is ideal for users who need a simple and efficient way to manage files without the overhead of more complex solutions.

## Features

- **File Browsing**: Navigate through directories and view file contents.
- **File Operations**: Perform basic file operations such as create, read, update, delete, and rename.
- **Search Functionality**: Quickly find files and directories based on name.
- **Platform**: Works only on Windows (might expand later).
- **Lightweight**: Minimal dependencies and low memory usage.

## Getting Started

### Prerequisites

- OpenGL and CMake

### Installation

1. **Clone the repository**
    ```bash
    git clone https://github.com/linus-karlsson/filetic.git
    cd filetic
    ```

2. **Compile using CMake**
    ```bash
    .\Commands\Make\configDebug.bat
    cmake --build build
    ```

3. **Compile using Ninja**
    ```bash
    .\Commands\Ninja\configDebug.bat
    cmake --build build
    ```

## Usage

### Running the Application

```bash
.\build\bin\FileTic.exe
```

## License

This project is licensed under the [Apache License](LICENSE).

---

Thank you for reading this!

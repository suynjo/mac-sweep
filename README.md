# Mac Sweep

<img width="2348" height="1486" alt="image" src="https://github.com/user-attachments/assets/1b201848-7442-4d90-bf95-9fea2bb6bb0f" />

A macOS cleanup tool that scans for old files, caches, duplicates,
and installers, and safely moves them to the Trash.

## Features

- Detects screenshots and images older than 7 days
- Detects user and developer caches
- Detects duplicate files using SHA-256 hashing
- Detects old installer files (.dmg, .pkg)
- Qt6 GUI with category filters and file preview

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
./mac_sweep
```

## Requirements

- C++20
- Qt6 (Widgets)
- CMake 3.16+

## Credits

- [PicoSHA2](https://github.com/okdshin/PicoSHA2) — SHA-256 hashing library

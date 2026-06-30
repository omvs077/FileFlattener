# FileFlattener

**FileFlattener** is a Windows desktop app that scans a folder tree, deduplicates files by content, and exports a clean, flattened ZIP archive — complete with an ASCII directory structure and a JSON manifest. Built for quickly packaging messy project folders (or any deeply nested directory) into a single, organized archive.

Crafted by **Dvvyom** — [GitHub](https://github.com/omvs077) | [omvs077@gmail.com](mailto:omvs077@gmail.com)

---

## Features (v1.0)

- **Recursive folder scanning** with a 20GB size cap, symlink and depth guards
- **Content-aware deduplication** via SHA-256 hashing (size-grouped for speed)
- **Smart name-collision resolution** using path-based renaming (`folderA__file.txt`)
- **Gitignore-style filter rules** plus quick presets (Standard Backup, AI-Ready Source Only, Media Only, Documents Only) — presets auto-fill sensible default excludes
- **Smart Filter Prompt** — if a scan exceeds the 20GB limit, FileFlattener offers to auto-exclude common heavy folders (`node_modules`, `build`, `.venv`, `target`) and re-scan
- **Split-view dashboard** — file tree (with live search, sort, expand/collapse) alongside an extension/size analytics table
- **Pre-Export Preview** — review scan diagnostics, warnings, and a clear notice before committing to export
- **Background export** — non-blocking UI with a progress dialog showing live stage and the current file being zipped
- **Recent Projects** — quick access to your last 5 scanned folders, persisted across sessions
- **Self-documenting ZIPs** — every export includes a `structure.txt` (ASCII tree) and `manifest.json`

## Download

Grab the latest prebuilt Windows executable from the [Releases](https://github.com/omvs077/FileFlattener/releases) page (tag `v1.0.0`). No installation required — just run `FileFlattenerApp.exe`.

## Building from Source

**Requirements:**
- Visual Studio Community 2026 (or compatible MSVC toolchain)
- CMake
- Qt 6.11.1 (`msvc2022_64`)
- vcpkg (`x64-windows` triplet)

**Build:**
```powershell
cmake --preset default
cmake --build build --target FileFlattenerApp --config Debug
```

**Run:**
```powershell
cd build\Debug
.\FileFlattenerApp.exe
```

A headless CLI test harness (`FileFlattenerCLI`) is also available for scripted/automated testing of the core engine without the GUI.

## Project Structure
'''FileFlattener/
├── core/    — Static library: FileScanner, Deduplicator, Renamer, ZipWriter, StructureExporter, FilterEngine
├── app/     — Qt GUI (MainWindow, PreviewDialog, FlattenWorker) + CLI harness
'''

## Roadmap

Phase 3 (next version) will add an interactive force-directed folder graph and a static UML/call-graph lexer for visualizing project structure and dependencies.

## Feedback

Suggestions and bug reports are welcome — open an issue on GitHub or reach out directly at [omvs077@gmail.com](mailto:omvs077@gmail.com).

## License

No license specified yet — all rights reserved by default until one is added.

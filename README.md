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
- **Background export** — non-blocking UI with a progress dialog showing live stage and the current file being zipped, plus clean cancellation (partial ZIP is deleted)
- **Recent Projects** — quick access to your last 5 scanned folders, persisted across sessions
- **Self-documenting ZIPs** — every export includes a `structure.txt` (ASCII tree) and `manifest.json`

## Features (v1.1 — Phase 3: Code Visualization)

- **Folder Graph tab** — interactive, force-directed visualization of the scanned folder tree, with draggable nodes, zoom/pan, expand/collapse, and file-type icons
- **Code Graph tab** — auto-detects the project type (18 supported languages/ecosystems) and renders a structural graph of files → classes/structs → methods/functions, with `#include`, containment, and inheritance edges
- **Call Graph tab** — C++-only static call-graph analysis: detects which functions/methods call which others and renders a directed graph (with arrowheads) of real call relationships across the codebase
- Both Code Graph and Call Graph run their analysis on a background thread, so the UI never freezes even on large (10k+ file) projects
- Safety caps throughout (node/edge/file-size limits) with on-screen warnings when a graph is truncated

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

A headless CLI test harness (`FileFlattenerCLI`) is also available for scripted/automated testing of the core engine without the GUI. It supports `--graph`, `--code`, and `--calls` modes for testing the Folder Graph, Code Graph, and Call Graph engines independently of the GUI.

## Project Structure 
```structure
FileFlattener/
├── core/   — Static library: FileScanner, Deduplicator, Renamer, ZipWriter, StructureExporter,
│             FilterEngine, GraphModel, ProjectDetector, CodeLexer, CallGraphAnalyzer
└── app/    — Qt GUI (MainWindow, PreviewDialog, FlattenWorker, GraphView, CodeGraphView,
CallGraphView, CodeAnalysisWorker, CallGraphWorker) + CLI harness
```

## Roadmap

Phase 1 (headless engine) and Phase 2 (Qt GUI + security hardening) are complete, with v1.0.0 published on GitHub Releases. Phase 3 (Folder Graph, Code Graph, and Call Graph visualization) is now complete as of this release.

## Feedback

Suggestions and bug reports are welcome — open an issue on GitHub or reach out directly at [omvs077@gmail.com](mailto:omvs077@gmail.com).

## License

MIT License — see [LICENSE](LICENSE) for details.

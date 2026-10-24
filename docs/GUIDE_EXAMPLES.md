# EmbedFS — Example workflow

> 日本語版: [GUIDE_EXAMPLES.ja.md](GUIDE_EXAMPLES.ja.md)

Read files embedded into firmware.

Check installation and platform requirements in the [library README](../README.md), then choose a sketch from the [example index](../examples/README.md). Version/support tables and internal capacity defaults stay in their original reference rather than being duplicated here.

## From setup to a verified result

1. Choose the smallest relevant sketch and check its board, hardware, dependencies and bundled files.
2. Match pins, peers and input files to your actual setup. Each README identifies important placeholders.
3. Open the .ino, select board/port and upload. Match Serial baud where used.
4. Locate the last completed stage: initialization, input, processing or output. Compilation does not test hardware I/O.
5. Reproduce the supplied example before changing one setting at a time.

## API and data handling

EmbedFS is read-only. Build the embedded asset table with the repository workflow and include the generated files with the sketch; changing a file on your PC does not change an already flashed image. `begin()` exposes the embedded tree, and `open()` / file iteration return ordinary FS-like objects whose validity and directory status must be checked.

Read while data is available, then close the file. Write, rename, mkdir and remove operations in the test example exercise expected rejection rather than demonstrating writable flash storage. Keep source assets and generated data consistent; see the root README for the generator invocation instead of hard-coding tool versions here.

## When adapting it

- Distinguish bool status, processed counts and handles; do not use stale output after failure.
- Registered callbacks run later from their documented context. Decide synchronization and retained-data lifetime.
- Keep bytes, samples, frames and pixels distinct; capacity is different from valid length.
- Retain views/pointers/buffers for the required period. Copying a pointer does not copy its payload.
- Add application-specific failure handling, retry and cleanup that a minimal example may omit.

## Read the individual README

Use Prepare and run for hardware and steps, Follow the sketch for the actual APIs and precautions for feature-specific constraints. If output is missing, check initialization first, continued service calls next and input/peer readiness after that.

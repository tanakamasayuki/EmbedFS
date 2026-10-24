# BasicTest

> 日本語版: [README.ja.md](README.ja.md)

Read embedded files and enumerate directories.

## Prepare and run

1. Check the [example guide](../../docs/GUIDE_EXAMPLES.md) for setup and hardware. Use the root README for supported boards and dependencies.
2. Open [BasicTest.ino](BasicTest.ino) and select the actual board/port. Prepare the dependencies used by these includes:

   `<EmbedFS.h>`

Keep these required local files with the sketch: [assets_embed.h](assets_embed.h)

3. Upload and open the Serial monitor at 115200 baud. Check startup messages and input conditions in the source.

## Follow the sketch

These APIs are listed in their first source order. Registering a callback does not execute its body at that point.

1. `EmbedFS.begin(...)` — Initialize this component after its prerequisites; inspect the returned status where provided.
2. `EmbedFS.open(...)` — Check that the requested input/file opened and that its type matches the intended operation.
3. `file.close(...)` — End use of the resource and stop using views or handles derived from it.
4. `dir.openNextFile(...)` — Check each returned entry and close it when finished; directories and ordinary files differ.

## API usage and precautions

EmbedFS is read-only. Build the embedded asset table with the repository workflow and include the generated files with the sketch; changing a file on your PC does not change an already flashed image. `begin()` exposes the embedded tree, and `open()` / file iteration return ordinary FS-like objects whose validity and directory status must be checked.

Read while data is available, then close the file. Write, rename, mkdir and remove operations in the test example exercise expected rejection rather than demonstrating writable flash storage. Keep source assets and generated data consistent; see the root README for the generator invocation instead of hard-coding tool versions here.

## Inputs and observable output

These are selected output formats from the source. printf specifiers are replaced by runtime values; inspect the surrounding conditions rather than assuming these lines always occur in this order.

```text
Embedded files:
- path: %s name: %s size: %u bytes
Directory listing of /directory:
```

## Verify and troubleshoot

Read embedded files and enumerate directories and verify the result on its output. First use the supplied input, then change one configuration or data source at a time.

| Symptom | First check |
|---|---|
| No output | Board/dependencies, initialization results, output destination and Serial baud |
| Starts but stops making progress | Wait conditions, buffer space, peer/input readiness and service calls |
| Unexpected values | Lengths/units, format, data lifetime, shared state and partial results |

See the [guide](../../docs/GUIDE_EXAMPLES.md) for shared principles and the [example index](../README.md) for the next sketch.

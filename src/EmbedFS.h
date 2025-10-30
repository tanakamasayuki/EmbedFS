/*
  EmbedFS.h

  Minimal EmbedFS implementation for Arduino/ESP32 projects.
  - Provides EmbedFSClass with begin(...) accepting generated arrays from assets_embed.h
  - Provides EmbedFSFile for reading embedded data without copying into RAM when possible

  This file is intentionally small and portable. Implementation is in EmbedFS.cpp.
*/

#ifndef EMBEDFS_H
#define EMBEDFS_H

#include <Arduino.h>
// Use FS.h to keep LittleFS-like API
#include "FS.h"
#include <stddef.h>

namespace fs
{

    class EmbedFSFile
    {
    public:
        EmbedFSFile();
        EmbedFSFile(const uint8_t *dataPtr, size_t dataSize);
        ~EmbedFSFile();

        // Minimal File-like API for embedded reads
        operator bool() const;
        size_t size() const;
        size_t position() const;
        bool seek(size_t pos);
        int available() const;
        int read();
        size_t read(uint8_t *buffer, size_t len);
        void close();

    private:
        const uint8_t *dataPtr_;
        size_t dataSize_;
        size_t pos_;
    };

    // EmbedFSFS: LittleFS-like class in fs namespace. Read-only filesystem backed by
    // embedded arrays (assets_file_names, assets_file_data, assets_file_sizes, assets_file_count).
    class EmbedFSFS : public FS
    {
    public:
        EmbedFSFS();
        ~EmbedFSFS();

        // Initialize from generated arrays (example in examples/EmbedFSTest)
        bool begin(const char *const file_names[], const uint8_t *const file_data[], const size_t file_sizes[], size_t file_count);

        // LittleFS-like overload for compatibility (no-op for embedded data)
        bool begin(bool formatOnFail = false, const char *basePath = "/embedfs", uint8_t maxOpenFiles = 10, const char *partitionLabel = nullptr);

        // Formatting / write operations are not supported for embedded read-only FS
        bool format();

        // Return total capacity and used bytes. For embedded FS, totalBytes is the
        // sum of all embedded file sizes; usedBytes equals totalBytes as all data is "used".
        size_t totalBytes();
        size_t usedBytes();

        void end();

        // FS-like helpers (open returns empty File; use openEmbedded() to get EmbedFSFile)
        bool exists(const char *path) const;
        File open(const char *path, const char *mode = "r") const;

        // Direct embedded file reader
        EmbedFSFile openEmbedded(const char *path) const;

    private:
        const char *const *fileNames_;
        const uint8_t *const *fileData_;
        const size_t *fileSizes_;
        size_t fileCount_;
    };

} // namespace fs

// Global instance (similar to LittleFS)
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EMBEDFS)
extern fs::EmbedFSFS EmbedFS;
#endif

#endif // EMBEDFS_H

/*
  EmbedFS.cpp
  Clean implementation for EmbedFS.h without EmbedFSFile helper.
*/

#include "EmbedFS.h"
#include "FSImpl.h"

#if defined(__AVR__)
#include <pgmspace.h>
#endif

#include <cstring>
#include <cstdlib>

using namespace fs;

// Embedded-backed FileImpl: provides read-only access to embedded arrays
class EmbeddedFileImpl : public FileImpl
{
public:
    EmbeddedFileImpl(const char *path, const uint8_t *data, size_t size)
        : _path(nullptr), _name(nullptr), _data(data), _size(size), _pos(0)
    {
        if (path)
            _path = strdup(path);
        const char *p = _path ? strrchr(_path, '/') : nullptr;
        if (p && *(p + 1))
        {
            _name = strdup(p + 1);
        }
        else if (_path)
        {
            _name = strdup(_path);
        }
    }

    virtual ~EmbeddedFileImpl()
    {
        if (_path)
            free((void *)_path);
        if (_name)
            free((void *)_name);
    }

    // write is unsupported for read-only embedded FS
    size_t write(const uint8_t * /*buf*/, size_t /*size*/) override { return 0; }

    size_t read(uint8_t *buf, size_t size) override
    {
        if (!_data || _pos >= _size)
            return 0;
        size_t remaining = _size - _pos;
        size_t toRead = (size < remaining) ? size : remaining;
        for (size_t i = 0; i < toRead; ++i)
        {
#if defined(__AVR__)
            buf[i] = pgm_read_byte(_data + _pos + i);
#else
            buf[i] = _data[_pos + i];
#endif
        }
        _pos += toRead;
        return toRead;
    }

    void flush() override {}

    bool seek(uint32_t pos, SeekMode mode) override
    {
        size_t newpos = _pos;
        if (mode == SeekSet)
            newpos = pos;
        else if (mode == SeekCur)
            newpos = _pos + pos;
        else if (mode == SeekEnd)
            newpos = _size + pos;
        if (newpos > _size)
            return false;
        _pos = newpos;
        return true;
    }

    size_t position() const override { return _pos; }
    size_t size() const override { return _size; }

    bool setBufferSize(size_t) override { return false; }

    void close() override {}

    time_t getLastWrite() override { return 0; }

    const char *path() const override { return _path; }
    const char *name() const override { return _name; }

    boolean isDirectory(void) override { return false; }

    FileImplPtr openNextFile(const char * /*mode*/) override { return FileImplPtr(); }
    boolean seekDir(long) override { return false; }
    String getNextFileName(void) override { return String(); }
    String getNextFileName(bool * /*isDir*/) override { return String(); }
    void rewindDirectory(void) override {}

    operator bool() override { return _data != nullptr; }

private:
    char *_path;
    char *_name;
    const uint8_t *_data;
    size_t _size;
    size_t _pos;
};

// FSImpl that serves embedded arrays
class EmbedFSImpl : public FSImpl
{
public:
    EmbedFSImpl(const char *const file_names[], const uint8_t *const file_data[], const size_t file_sizes[], size_t file_count)
        : names_(file_names), data_(file_data), sizes_(file_sizes), count_(file_count) {}
    virtual ~EmbedFSImpl() {}

    FileImplPtr open(const char *path, const char * /*mode*/, const bool /*create*/) override
    {
        if (!path)
            return FileImplPtr();
        // normalize path (accept with or without leading '/')
        std::string p(path);
        if (!p.empty() && p.front() == '/')
            p.erase(0, 1);
        std::string display = std::string("/") + p;

        for (size_t i = 0; i < count_; ++i)
        {
            if (!names_[i])
                continue;
            std::string nj(names_[i]);
            if (!nj.empty() && nj.front() == '/')
                nj.erase(0, 1);
            if (nj == p)
                return std::make_shared<EmbeddedFileImpl>(display.c_str(), data_[i], sizes_[i]);
        }
        // treat as directory if any entry has this prefix
        std::string prefix = p.empty() ? std::string() : (p + '/');
        for (size_t i = 0; i < count_; ++i)
        {
            if (!names_[i])
                continue;
            std::string fn(names_[i]);
            if (!fn.empty() && fn.front() == '/')
                fn.erase(0, 1);
            if (prefix.empty() || (fn.size() > prefix.size() && fn.compare(0, prefix.size(), prefix) == 0))
                return std::make_shared<EmbeddedFileImpl>(display.c_str(), nullptr, 0); // directory marker
        }
        return FileImplPtr();
    }

    bool exists(const char *path) override
    {
        if (!path)
            return false;
        std::string p(path);
        if (!p.empty() && p.front() == '/')
            p.erase(0, 1);
        for (size_t i = 0; i < count_; ++i)
        {
            if (!names_[i])
                continue;
            std::string nj(names_[i]);
            if (!nj.empty() && nj.front() == '/')
                nj.erase(0, 1);
            if (nj == p)
                return true;
        }
        std::string prefix = p.empty() ? std::string() : (p + '/');
        for (size_t i = 0; i < count_; ++i)
        {
            if (!names_[i])
                continue;
            std::string fn(names_[i]);
            if (!fn.empty() && fn.front() == '/')
                fn.erase(0, 1);
            if (prefix.empty() || (fn.size() > prefix.size() && fn.compare(0, prefix.size(), prefix) == 0))
                return true;
        }
        return false;
    }

    bool rename(const char * /*pathFrom*/, const char * /*pathTo*/) override { return false; }
    bool remove(const char * /*path*/) override { return false; }
    bool mkdir(const char * /*path*/) override { return false; }
    bool rmdir(const char * /*path*/) override { return false; }

private:
    const char *const *names_;
    const uint8_t *const *data_;
    const size_t *sizes_;
    size_t count_;
};

// ---------------- EmbedFSFS (public API) ----------------
EmbedFSFS::EmbedFSFS() : FS(FSImplPtr(nullptr)), fileNames_(nullptr), fileData_(nullptr), fileSizes_(nullptr), fileCount_(0) {}
EmbedFSFS::~EmbedFSFS() { end(); }

bool EmbedFSFS::begin(const char *const file_names[], const uint8_t *const file_data[], const size_t file_sizes[], size_t file_count)
{
    if (!file_names || !file_data || !file_sizes || file_count == 0)
        return false;
    _impl = FSImplPtr(new EmbedFSImpl(file_names, file_data, file_sizes, file_count));
    fileNames_ = file_names;
    fileData_ = file_data;
    fileSizes_ = file_sizes;
    fileCount_ = file_count;
    return true;
}

bool EmbedFSFS::begin(bool /*formatOnFail*/, const char * /*basePath*/, uint8_t /*maxOpenFiles*/, const char * /*partitionLabel*/) { return (_impl != nullptr); }
bool EmbedFSFS::format() { return false; }
void EmbedFSFS::end()
{
    _impl.reset();
    fileNames_ = nullptr;
    fileData_ = nullptr;
    fileSizes_ = nullptr;
    fileCount_ = 0;
}

bool EmbedFSFS::exists(const char *path) const
{
    if (!_impl)
        return false;
    return _impl->exists(path);
}
File EmbedFSFS::open(const char *path, const char *mode) const
{
    if (!_impl)
        return File();
    return File(_impl->open(path, mode, false));
}

size_t EmbedFSFS::totalBytes()
{
    if (!fileSizes_)
        return 0;
    size_t sum = 0;
    for (size_t i = 0; i < fileCount_; ++i)
        sum += fileSizes_[i];
    return sum;
}
size_t EmbedFSFS::usedBytes() { return totalBytes(); }

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EMBEDFS)
EmbedFSFS EmbedFS;
#endif

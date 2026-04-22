#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

namespace nodar {
namespace zmq {

/**
 * Pluggable backing store for Buffer. Default is a std::vector<uint8_t>.
 *
 * Lets a consumer (e.g. a CUDA-aware module) plug in alternative storage
 * such as a cv::cuda::HostMem-backed region, without introducing any
 * dependency here.
 */
struct BufferStorage {
    virtual ~BufferStorage() = default;
    virtual uint8_t* data() = 0;
    [[nodiscard]] virtual size_t size() const = 0;
    [[nodiscard]] virtual size_t capacity() const = 0;
    virtual void resize(size_t n) = 0;
};

/// Default storage: a std::vector<uint8_t>. Behavior-preserving for every
/// existing caller of Buffer.
class VectorBufferStorage : public BufferStorage {
    std::vector<uint8_t> vec{};

public:
    VectorBufferStorage() = default;
    explicit VectorBufferStorage(size_t n) : vec(n) {}
    uint8_t* data() override { return vec.data(); }
    [[nodiscard]] size_t size() const override { return vec.size(); }
    [[nodiscard]] size_t capacity() const override { return vec.capacity(); }
    void resize(size_t n) override { vec.resize(n); }
};

struct BufferPool;

class Buffer {
    std::unique_ptr<BufferStorage> storage;

public:
    BufferPool* buffer_pool;

    explicit Buffer(BufferPool* pool)
        : storage(std::make_unique<VectorBufferStorage>()), buffer_pool(pool) {}
    Buffer(size_t size, BufferPool* pool)
        : storage(std::make_unique<VectorBufferStorage>(size)), buffer_pool(pool) {}
    Buffer(std::unique_ptr<BufferStorage> s, BufferPool* pool)
        : storage(std::move(s)), buffer_pool(pool) {}

    [[nodiscard]] auto size() const { return storage->size(); }
    [[nodiscard]] auto capacity() const { return storage->capacity(); }
    [[nodiscard]] auto data() { return storage->data(); }
    void resize(size_t size) { storage->resize(size); }

    /**
     * Release a buffer back to the pool. This is the signature that is used by ZMQ.
     * We assume that a Buffer * was passed in the hint, and DO NOT check that assumption
     */
    static void release(void* data, void* hint);
};

struct BufferPool {
    /// Factory for per-buffer backing storage. If null, buffers use the
    /// default VectorBufferStorage.
    using StorageFactory = std::function<std::unique_ptr<BufferStorage>()>;
    StorageFactory storage_factory{};

    // Maintain a list of all created buffers. This ensures that all buffers are destroyed when the pool is destroyed.
    std::list<Buffer> created;
    // A list of all the buffers that are available, that is, the buffers that have been returned to the pool.
    std::list<Buffer*> available;
    // Mutexes for thread safety
    std::mutex created_guard;
    std::mutex available_guard;

    BufferPool() = default;
    explicit BufferPool(StorageFactory factory) : storage_factory(std::move(factory)) {}

    /**
     * Get a buffer. If none are available, then make a new one.
     * Note that this function returns a pointer,
     * but the ownership of the buffer is maintained by the buffer pool.
     * When the BufferPool is destroyed, all the buffers created by it are destroyed.
     * You should never call delete on a buffer that was created by a BufferPool.
     */
    auto get() {
        Buffer* buffer = nullptr;
        {
            std::lock_guard<std::mutex> lock(available_guard);
            if (not available.empty()) {
                buffer = available.front();
                available.pop_front();
            }
        }
        if (not buffer) {
            std::lock_guard<std::mutex> lock(created_guard);
            if (storage_factory) {
                created.emplace_back(storage_factory(), this);
            } else {
                created.emplace_back(this);
            }
            buffer = &created.back();
        }
        return buffer;
    }

    /**
     * Return a buffer to the pool so that it can be reused in subsequent calls to get().
     * The buffer pointer must have been created by the same BufferPool.
     */
    void put(Buffer* buffer) {
        std::lock_guard<std::mutex> lock(available_guard);
        available.push_back(buffer);
    }
};

inline void Buffer::release(void* data, void* hint) {
    auto buffer = static_cast<Buffer*>(hint);
    buffer->buffer_pool->put(buffer);
}

}  // namespace zmq
}  // namespace nodar

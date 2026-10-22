#pragma once

#include <functional>
#include <list>
#include <memory>
#include <mutex>

namespace nodar {
namespace zmq {

/**
 * Allocates the storage of a Buffer: `size` bytes, freed by the returned pointer's deleter.
 * The default allocates on the heap. Supply your own to e.g. use page-locked memory,
 * so that GPU downloads into the buffers are faster.
 */
using Allocator = std::function<std::shared_ptr<uint8_t[]>(size_t size)>;
inline std::shared_ptr<uint8_t[]> allocateOnHeap(size_t size) { return std::shared_ptr<uint8_t[]>(new uint8_t[size]); }

struct BufferPool;
class Buffer {
    std::shared_ptr<uint8_t[]> storage{};
    size_t storage_size = 0;
    size_t storage_capacity = 0;

public:
    BufferPool* buffer_pool;
    explicit Buffer(BufferPool* pool) : buffer_pool(pool) {}
    Buffer(size_t size, BufferPool* pool) : buffer_pool(pool) { resize(size); }
    [[nodiscard]] auto size() const { return storage_size; }
    [[nodiscard]] auto capacity() const { return storage_capacity; }
    [[nodiscard]] auto data() { return storage.get(); }

    /**
     * Resize the buffer. The storage is only reallocated (with the pool's allocator) when growing past the capacity.
     * The contents are not preserved.
     */
    void resize(size_t size);

    /**
     * Release a buffer back to the pool. This is the signature that is used by ZMQ.
     * We assume that a Buffer * was passed in the hint, and DO NOT check that assumption
     */
    static void release(void* data, void* hint);
};

struct BufferPool {
    // Allocates the storage of the buffers
    Allocator allocator;
    // Maintain a list of all created buffers. This ensures that all buffers are destroyed when the pool is destroyed.
    std::list<Buffer> created;
    // A list of all the buffers that are available, that is, the buffers that have been returned to the pool.
    std::list<Buffer*> available;
    // Mutexes for thread safety
    std::mutex created_guard;
    std::mutex available_guard;

    explicit BufferPool(Allocator allocator = allocateOnHeap) : allocator(std::move(allocator)) {}

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
            created.emplace_back(this);
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

inline void Buffer::resize(size_t size) {
    if (size > storage_capacity) {
        storage = buffer_pool->allocator(size);
        storage_capacity = size;
    }
    storage_size = size;
}

inline void Buffer::release(void* data, void* hint) {
    auto buffer = static_cast<Buffer*>(hint);
    buffer->buffer_pool->put(buffer);
}

}  // namespace zmq
}  // namespace nodar

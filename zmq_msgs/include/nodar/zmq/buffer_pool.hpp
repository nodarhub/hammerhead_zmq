#pragma once

#include <list>
#include <mutex>
#include <vector>

namespace nodar {
namespace zmq {

struct BufferPool;
class Buffer {
    std::vector<uint8_t> vec{};

public:
    BufferPool* buffer_pool;
    explicit Buffer(BufferPool* pool) : buffer_pool(pool) {}
    Buffer(size_t size, BufferPool* pool) : vec(size), buffer_pool(pool) {}
    [[nodiscard]] auto size() const { return vec.size(); }
    [[nodiscard]] auto capacity() const { return vec.capacity(); }
    [[nodiscard]] auto data() { return vec.data(); }
    void resize(size_t size) { vec.resize(size); }

    /**
     * Release a buffer back to the pool. This is the signature that is used by ZMQ.
     * We assume that a Buffer * was passed in the hint, and DO NOT check that assumption
     */
    static void release(void* data, void* hint);
};

struct BufferPool {
    // Maintain a list of all created buffers. This ensures that all buffers are destroyed when the pool is destroyed.
    std::list<Buffer> created;
    // A list of all the buffers that are available, that is, the buffers that have been returned to the pool.
    std::list<Buffer*> available;
    // Mutexes for thread safety
    std::mutex created_guard;
    std::mutex available_guard;

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

inline void Buffer::release(void* data, void* hint) {
    auto buffer = static_cast<Buffer*>(hint);
    buffer->buffer_pool->put(buffer);
}

}  // namespace zmq
}  // namespace nodar
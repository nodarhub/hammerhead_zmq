#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <nodar/zmq/topic_ports.hpp>
#include <opencv2/core.hpp>
#include <thread>
#include <unordered_set>
#include <vector>
#include <zmq.hpp>

#include "buffer_pool.hpp"

namespace nodar {
namespace zmq {

template <typename Data>
class Publisher {
private:
    Topic topic;
    ::zmq::context_t context;
    ::zmq::socket_t socket;
    std::thread background_thread;
    std::condition_variable condition;
    std::mutex buffer_guard;
    std::atomic_bool running;
    Buffer* queued_buffer = nullptr;
    bool buffer_is_queued = false;
    BufferPool buffer_pool;

public:
    Publisher(const Topic& topic, const std::string& ip)
        : topic(topic), context(1), socket(context, ZMQ_PUB), running(true) {
        socket.set(::zmq::sockopt::sndhwm, 1);  // set maximum queue length to 1 message
        // If the IP is empty, bind on this device.
        if (ip.empty()) {
            const std::string endpoint = "tcp://*:" + std::to_string(topic.port);
            std::cout << "Binding publisher for " << topic.name << " on the endpoint " << endpoint << std::endl;
            socket.bind(endpoint);
            // ZMQ slow joiner mitigation: wait for subscribers to connect after binding
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } else {
            // Otherwise, assume this is a subscriber IP and connect to it
            const std::string endpoint = "tcp://" + ip + ":" + std::to_string(topic.port);
            std::cout << "Connecting publisher for " << topic.name << " on the endpoint " << endpoint << std::endl;
            socket.connect(endpoint);
        }
        background_thread = std::thread(&Publisher::loop, this);
    }

    ~Publisher() {
        running = false;
        condition.notify_all();
        if (background_thread.joinable()) {
            background_thread.join();
        }
    }

    /**
     * Get a buffer from the pool that this publisher owns.
     * Note that the buffer pool still owns the lifetime of the buffer.
     * DO NOT delete the buffer.
     * When you are ready to send the buffer, call the Publisher::send method to queue it.
     * After you call the send method, you should not use the buffer.
     * It will be returned to the buffer pool after it is sent.
     * If you need another buffer, you must call this method again.
     * Note that if you end up not using the buffer or not calling the send method,
     * then you should return it to the pool using either the
     * BufferPool::put or Buffer::release method
     */
    Buffer* getBuffer() { return buffer_pool.get(); }

    /**
     * Queue a buffer to be sent in the next loop iteration.
     * After this call, you should not reuse the buffer.
     * If you need another buffer, you must call the getBuffer method again.
     * Note that the BufferPool maintains ownership of the buffer throughout its lifetime,
     * so if a buffer never gets sent, we are sure that it will not leak.
     */
    void send(Buffer* buffer) {
        if (not running) {
            return;
        }
        // However, if a queued buffer never gets sent, then it will never be returned to the pool.
        {
            std::lock_guard<std::mutex> lock(buffer_guard);
            queued_buffer = buffer;
            buffer_is_queued = true;
        }
        condition.notify_one();
    }

private:
    /**
     * If the application has stopped running, then exit.
     * Otherwise, take the queued buffer and send it.
     * Note that we use the ZMQ free function to ensure that the
     * buffer will be released back to its pool after it is sent.
     * https://linux.die.net/man/3/zmq_msg_init_data
     * Furthermore, the BufferPool maintains ownership of the buffer throughout its lifetime.
     * So if a buffer never gets sent, we are sure that it will not leak.
     */
    void loop() {
        Buffer* buffer;
        while (running) {
            {
                std::unique_lock<std::mutex> lock(buffer_guard);
                condition.wait(lock, [this] { return buffer_is_queued or not running; });
                if (not running) {
                    break;
                }
                buffer = queued_buffer;
                buffer_is_queued = false;
            }

            // Create a message around the buffer and send it.
            ::zmq::message_t msg(buffer->data(), buffer->size(), Buffer::release, buffer);
            socket.send(msg, ::zmq::send_flags::none);
        }
    }
};

}  // namespace zmq
}  // namespace nodar
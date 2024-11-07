This example demonstrates how to toggle recording in Hammerhead using ZMQ. Currently, we expose this using a
Request-Reply pattern. The topic names and ports used for this communication are defined in
the `topic_ports.hpp` header.

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

This will build the binary `toggle_recording`. To run the examples, you need to provide the IP
address of the ZMQ source (almost certainly the Orin's IP address):

    # Linux
    ./toggle_recording src_ip

    # Windows
    ./Release/toggle_recording src_ip

Note that if you specify an incorrect IP address or run this example when Hammerhead is not running, then ZMQ will
attempt to subscribe, and nothing will happen. It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.
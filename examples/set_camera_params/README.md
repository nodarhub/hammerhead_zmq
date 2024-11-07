This example demonstrates how to modify the camera parameters in Hammerhead using ZMQ. Currently, we expose controls for the exposure and gain using a Request-Reply pattern. The topic names and ports used for this communication are defined in the `topic_ports.hpp` header.

To build this example, follow the traditional CMake process:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release

This will build 2 binaries: `set_exposure` and `set_gain`. To run either of these examples, you need to provide the IP address of the ZMQ source (almost certainly the Orin's IP address):

    # Linux
    ./set_exposure src_ip

    # Windows
    ./Release/set_exposure src_ip

Note that if you specify an incorrect IP address or run this example when Hammerhead is not running, then ZMQ will attempt to subscribe, and nothing will happen. It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.
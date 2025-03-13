This example demonstrates how to record images published by Hammerhead with ZMQ.
To run this example, you need to provide the IP address of the ZMQ source (the device running Hammerhead),
the topic name or port number, and the folder where you want the data to be saved:

    python image_recorder.py src_ip image_topic_or_port output_dir

The file `zmq_msgs/topic_ports.py` defines the `topic->port` mappings.
For example, in that file, you will find the entry

    "nodar/right/image_raw", 9801

which indicates that the raw right images will be sent on port 9801.
If you run this example on the device running Hammerhead,
then the following command would save the raw right images in the `raw_right_images` folder::

    python image_recorder.py 127.0.0.1 9801 raw_right_images

Alternatively, you can specify the topic name:

    python image_recorder.py 127.0.0.1 nodar/right/image_raw output_dir

Note that the parameter `image_topic` should be one of the `IMAGE_TOPICS` in `topic_ports.py`.
Furthermore, if you specify an incorrect IP address or run this example when Hammerhead is not running,
then ZMQ will attempt to subscribe, and nothing will happen. It will appear like the binary is just waiting.

To kill this example, just press CTRL+C.

Some currently valid image topic names are:

    nodar/color_blended_depth/image_raw
    nodar/disparity
    nodar/left/image_raw
    nodar/left/image_rect
    nodar/right/image_raw
    nodar/right/image_rect

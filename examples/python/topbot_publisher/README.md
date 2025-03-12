This example shows how to publish the left and right stereo images stacked vertically (also called topbot images) 
from the disk to Hammerhead.

To run this example, you need to provide the path to the topbot data directory and the port number to publish the images to:

    python topbot_publisher <topbot_data_directory> <port_number>

Note that the images inside the `topbot_data_directory` should be sequentially numbered. 
The images should be in the `.tiff` format.

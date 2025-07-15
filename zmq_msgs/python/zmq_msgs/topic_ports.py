class Topic:
    def __init__(self, name, port):
        self.name = name
        self.port = port


LEFT_RAW_TOPIC = Topic("nodar/left/image_raw", 9800)
RIGHT_RAW_TOPIC = Topic("nodar/right/image_raw", 9801)
LEFT_RECT_TOPIC = Topic("nodar/left/image_rect", 9802)
RIGHT_RECT_TOPIC = Topic("nodar/right/image_rect", 9803)
DISPARITY_TOPIC = Topic("nodar/disparity", 9804)
COLOR_BLENDED_DEPTH_TOPIC = Topic("nodar/color_blended_depth/image_raw", 9805)
TOPBOT_RAW_TOPIC = Topic("nodar/topbot_raw", 9813)

# Define the image topics as a list of Topic objects
IMAGE_TOPICS = [
    LEFT_RAW_TOPIC,
    RIGHT_RAW_TOPIC,
    LEFT_RECT_TOPIC,
    RIGHT_RECT_TOPIC,
    DISPARITY_TOPIC,
    COLOR_BLENDED_DEPTH_TOPIC,
    TOPBOT_RAW_TOPIC,
]

# Define other topics as individual Topic objects
SOUP_TOPIC = Topic("nodar/point_cloud_soup", 9806)
CAMERA_EXPOSURE_TOPIC = Topic("nodar/set_exposure", 9807)
CAMERA_GAIN_TOPIC = Topic("nodar/set_gain", 9808)
POINT_CLOUD_TOPIC = Topic("nodar/point_cloud", 9809)
POINT_CLOUD_RGB_TOPIC = Topic("nodar/point_cloud_rgb", 9810)
RECORDING_TOPIC = Topic("nodar/recording", 9811)
OBSTACLE_TOPIC = Topic("nodar/obstacle", 9812)
WAIT_TOPIC = Topic("nodar/wait", 9814)


# Function to retrieve reserved ports dynamically
def get_reserved_ports():
    reserved_ports = set()
    for topic in IMAGE_TOPICS:
        reserved_ports.add(topic.port)
    reserved_ports.add(SOUP_TOPIC.port)
    reserved_ports.add(CAMERA_EXPOSURE_TOPIC.port)
    reserved_ports.add(CAMERA_GAIN_TOPIC.port)
    reserved_ports.add(POINT_CLOUD_TOPIC.port)
    reserved_ports.add(POINT_CLOUD_RGB_TOPIC.port)
    reserved_ports.add(RECORDING_TOPIC.port)
    reserved_ports.add(OBSTACLE_TOPIC.port)
    reserved_ports.add(WAIT_TOPIC.port)
    return reserved_ports

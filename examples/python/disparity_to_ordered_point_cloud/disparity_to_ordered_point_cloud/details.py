import yaml
import numpy as np


class Details:
    def __init__(self, path_to_yaml: str):
        with open(path_to_yaml, "r") as f:
            data = yaml.safe_load(f)

        self.left_time = int(data["left_time"])
        self.right_time = int(data["right_time"])
        self.focal_length = float(data["focal_length"])
        self.baseline = float(data["baseline"])
        self.meters_above_ground = float(data["meters_above_ground"])

        self.disparity_to_depth4x4 = np.array(data["projection"], dtype=np.float32).reshape((4, 4))
        self.rotation_disparity_to_raw_cam = np.array(
            data["rotation_disparity_to_raw_cam"], dtype=np.float32
        ).reshape((3, 3))
        self.rotation_world_to_raw_cam = np.array(
            data["rotation_world_to_raw_cam"], dtype=np.float32
        ).reshape((3, 3))

    def __str__(self):
        return (
            "Details:\n"
            f"\tleft_time               : {self.left_time}\n"
            f"\tright_time              : {self.right_time}\n"
            f"\tfocal_length            : {self.focal_length:.6f}\n"
            f"\tbaseline                : {self.baseline:.6f}\n"
            f"\tmeters_above_ground     : {self.meters_above_ground:.6f}\n"
            f"\tdisparity_to_depth4x4   :\n{self.disparity_to_depth4x4}\n"
            f"\trotation_disp_to_raw_cam:\n{self.rotation_disparity_to_raw_cam}\n"
            f"\trotation_world_to_raw_cam:\n{self.rotation_world_to_raw_cam}\n"
        )

import numpy as np
import yaml


def get_typed_value(data, key, type_func, default):
    """Safely retrieves and casts a value from a dictionary.

    - If the key is missing, uses the default.
    - If the value is already of the correct type, returns it.
    - If the value can be coerced to the correct type, returns it.
    - Otherwise, returns the default and logs a warning.

    Always returns a value. Never throws.
    """
    value = data.get(key, default)
    if isinstance(value, type_func):
        return value
    try:
        coerced = type_func(value)
        if isinstance(value, str) and type_func is not str:
            print(f"Coerced key '{key}' from {type(value).__name__} to {type_func.__name__}")
        return coerced
    except (ValueError, TypeError):
        print(
            f"Could not coerce key '{key}' value {repr(value)} to {type_func.__name__}. "
            f"Using default: {default}"
        )
        return default


def get_array(data, key, shape, dtype=np.float32):
    try:
        arr = np.array(data[key], dtype=dtype).reshape(shape)
        return arr
    except (KeyError, ValueError, TypeError) as e:
        print(f"[WARN] Failed to load array '{key}' with shape {shape}: {e}")
        return np.zeros(shape, dtype=dtype)


class Details:
    def __init__(self, path_to_yaml: str):
        with open(path_to_yaml, "r") as f:
            data = yaml.safe_load(f)

        # Load scalars
        self.left_time = get_typed_value(data, "left_time", int, 0)
        self.right_time = get_typed_value(data, "right_time", int, 0)
        self.focal_length = get_typed_value(data, "focal_length", float, 0.0)
        self.baseline = get_typed_value(data, "baseline", float, 0.0)
        self.meters_above_ground = get_typed_value(data, "meters_above_ground", float, 0.0)

        # Load arrays
        self.disparity_to_depth4x4 = get_array(data, "projection", (4, 4))
        self.rotation_disparity_to_raw_cam = get_array(
            data, "rotation_disparity_to_raw_cam", (3, 3)
        )
        self.rotation_world_to_raw_cam = get_array(data, "rotation_world_to_raw_cam", (3, 3))

    def __str__(self):
        return (
            "Details:\n"
            f"\tleft_time                   : {self.left_time}\n"
            f"\tright_time                  : {self.right_time}\n"
            f"\tfocal_length                : {self.focal_length:.6f}\n"
            f"\tbaseline                    : {self.baseline:.6f}\n"
            f"\tmeters_above_ground         : {self.meters_above_ground:.6f}\n"
            f"\tdisparity_to_depth4x4       :\n{self.disparity_to_depth4x4}\n"
            f"\trotation_disp_to_raw_cam    :\n{self.rotation_disparity_to_raw_cam}\n"
            f"\trotation_world_to_raw_cam   :\n{self.rotation_world_to_raw_cam}\n"
        )

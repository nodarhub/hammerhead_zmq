import numpy as np


class Details:
    def __init__(self, filename):
        self.disparity_to_depth4x4 = np.zeros((4, 4), dtype=np.float32)
        with open(filename, 'r') as details_file:
            header = details_file.readline()
            detail_data = details_file.readline()
            tokens = detail_data.split(',')
            self.left_time = float(tokens[0])
            self.right_time = float(tokens[1])
            self.focal_length = float(tokens[2])
            self.baseline = float(tokens[3])
            for i in range(16):
                self.disparity_to_depth4x4[i // 4, i % 4] = float(tokens[4 + i])

    def __str__(self):
        return ("Details:\n" +
                f"\tleft_time    : {self.left_time}\n" +
                f"\tright_time   : {self.right_time}\n" +
                f"\tfocal_length : {self.focal_length}\n" +
                f"\tbaseline     : {self.baseline}\n" +
                f"disparity_to_depth4x4 : \n{self.disparity_to_depth4x4}\n"
                )

This example shows how to convert `.tiff` disparity images to ordered point clouds.

To run this example, you need to provide the path of `disparity` and `details` folders of the data directory saved by
Hammerhead.
The `details` data have to be in `yaml` format.
You would also need to specify the output directory where you want to save the organized point clouds.
By default, the ordered point clouds would be saved as 3 channel `.tiffs` with each channel representing the `x`, `y`
and `z` dimensions.
If you specify `--split` option, the data would be saved as 3 separate text files for each dimension.

    cd examples/python/disparity_to_ordered_point_cloud/disparity_to_ordered_point_cloud
    python3 disparity_to_ordered_point_cloud.py disparity_dir details_dir output_dir [--split]

If you already have the output directory, then the files inside it will be overwritten.
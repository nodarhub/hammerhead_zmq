from setuptools import find_packages
from setuptools import setup

package_name = "disparity_to_ordered_point_cloud"

setup(
    name=package_name,
    version="0.0.0",
    packages=find_packages(),
    install_requires=["setuptools"],
    zip_safe=False,
    maintainer="nodar",
    maintainer_email="support@nodarsensor.com",
    description="This package converts the disparity data into organized point clouds.",
    license="NODAR Limited Copyright License",
    license_files=["LICENSE"],
    project_urls={
        "License": "https://github.com/nodarhub/hammerhead_zmq/blob/main/LICENSE",
    },
    entry_points={
        "console_scripts": [
            f"{package_name} = {package_name}.{package_name}:main",
            f"visualize_point_cloud_tiff = {package_name}.visualize_point_cloud_tiff:main",
        ],
    },
)

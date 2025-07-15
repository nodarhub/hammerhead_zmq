from setuptools import find_packages
from setuptools import setup

package_name = "point_cloud_recorder"

setup(
    name=package_name,
    version="0.0.0",
    packages=find_packages(),
    install_requires=["setuptools"],
    zip_safe=False,
    maintainer="nodar",
    maintainer_email="support@nodarsensor.com",
    description="This example demonstrates how to record point clouds published by Hammerhead with ZMQ.",
    license="NODAR Limited Copyright License",
    license_files=["LICENSE"],
    project_urls={
        "License": "https://github.com/nodarhub/hammerhead_zmq/blob/main/LICENSE",
    },
    entry_points={
        "console_scripts": [
            f"{package_name} = {package_name}.{package_name}:main",
        ],
    },
)

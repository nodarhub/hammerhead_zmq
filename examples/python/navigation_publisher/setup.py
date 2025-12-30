from setuptools import find_packages
from setuptools import setup

package_name = "navigation_publisher"

setup(
    name=package_name,
    version="0.0.0",
    packages=find_packages(),
    install_requires=["setuptools"],
    zip_safe=False,
    maintainer="nodar",
    maintainer_email="support@nodarsensor.com",
    description="This example shows how to publish navigation data to Hammerhead.",
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
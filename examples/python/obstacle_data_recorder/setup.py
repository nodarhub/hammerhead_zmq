from setuptools import setup, find_packages

package_name = 'obstacle_data_recorder'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    install_requires=['setuptools'],
    zip_safe=False,
    maintainer='nodar',
    maintainer_email='support@nodarsensor.com',
    description='This example demonstrates how to record obstacle data published by Hammerhead with ZMQ.',
    license='NODAR Limited Copyright License',
    license_files=['LICENSE'],
    project_urls={
        'License': 'https://github.com/nodarhub/hammerhead_zmq/blob/main/LICENSE',
    },
    entry_points={
        'console_scripts': [
            f'{package_name} = {package_name}.{package_name}:main',
        ],
    },
)

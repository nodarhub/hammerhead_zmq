from setuptools import setup, find_packages

package_name = 'hammerhead_scheduler'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    install_requires=['setuptools'],
    zip_safe=False,
    maintainer='nodar',
    maintainer_email='support@nodarsensor.com',
    description='This example demonstrates how to schedule hammerhead using ZMQ.',
    license='TODO',
    entry_points={
        'console_scripts': [
            f'{package_name} = {package_name}.{package_name}:main',
        ],
    },
)

from setuptools import setup, find_packages

package_name = 'topbot_publisher'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    install_requires=['setuptools'],
    zip_safe=False,
    maintainer='nodar',
    maintainer_email='support@nodarsensor.com',
    description='This example shows how to publish topbot images from the disk to Hammerhead.',
    license='TODO',
    entry_points={
        'console_scripts': [
            f'{package_name} = {package_name}.{package_name}:main',
        ],
    },
)

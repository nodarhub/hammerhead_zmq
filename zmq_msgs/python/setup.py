from setuptools import setup, find_packages

package_name = 'zmq_msgs'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    install_requires=['setuptools'],
    zip_safe=False,
    maintainer='nodar',
    maintainer_email='support@nodarsensor.com',
    description='This package contains all of the ZMQ message types.',
    license='TODO',
)

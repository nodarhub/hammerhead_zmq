from setuptools import setup, find_packages

package_name = 'depth_to_disparity'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    install_requires=['setuptools'],
    zip_safe=False,
    maintainer='nodar',
    maintainer_email='support@nodarsensor.com',
    description='This package converts the depth data recorded by hammerhead into disparity data.',
    license='TODO',
    entry_points={
        'console_scripts': [
            f'{package_name} = {package_name}.{package_name}:main',
        ],
    },
)

#!/usr/bin/python3

import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="hawck_ui",
    version="0.4.0",
    author="Jonas Møller",
    author_email="jonasmo441@gmail.com",
    description="User interface for the Hawck keyboard macro system.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/snyball/Hawck",
    packages=setuptools.find_packages(),
    include_package_data=True,
    package_data={
        "hawck_ui": [
            "resources/glade-xml/*.ui"
        ],
    },
    entry_points={
        "main_entry": [
            "start = hawck_ui.main:main"
        ],
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: BSD",
        "Operating System :: Linux",
    ],
)

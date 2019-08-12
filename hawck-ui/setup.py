#!/usr/bin/python3

import os, setuptools, json

with open("README.md", "r") as fh:
    long_description = fh.read()

MESON_BUILD_ROOT = os.environ.get('MESON_BUILD_ROOT') or '../build'

with open(os.path.join(MESON_BUILD_ROOT, "hawck-ui/setup_config.json")) as rf:
    config = json.load(rf)

setuptools.setup(
    name="hawck_ui",
    version=config.get("version", ""),
    author="Jonas Møller",
    author_email="sanoj@nimda.no",
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

# Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/usr/bin/env python3
"""Setup script for edgeapp_cli"""

from setuptools import setup, find_packages

# Read requirements from requirements.txt
try:
    with open('requirements.txt', 'r', encoding='utf-8') as f:
        requirements = [line.strip() for line in f if line.strip() and not line.startswith('#')]
except FileNotFoundError:
    requirements = [
        'paho-mqtt>=1.6.0',
        'colorama>=0.4.0',
        'termcolor>=1.1.0'
    ]

setup(
    name="edgeapp-cli",
    version="1.0.0",
    description="AI Model & Edge App Deployment CLI for AITRIOS",
    long_description="Command line interface for deploying AI models and Edge Apps to AITRIOS devices",
    author="Sony Semiconductor Solutions",
    packages=find_packages(),
    py_modules=['edgeapp_cli', 'mqtt', 'colored_logger', 'webserver'],
    install_requires=requirements,
    entry_points={
        'console_scripts': [
            'edgeapp_cli=edgeapp_cli:main',
        ],
    },
    python_requires='>=3.7',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
    keywords='aitrios edge-app ai-model deployment cli',
)

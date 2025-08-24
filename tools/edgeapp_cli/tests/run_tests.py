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
"""
Test runner script for Deploy CLI
Supports both unittest and pytest execution
"""

import os
import sys
import subprocess
from pathlib import Path

def run_unittest():
    """Run tests using unittest"""
    print("Running tests with unittest...")
    print("=" * 50)

    # Run unit tests from parent directory
    result = subprocess.run([
        sys.executable, "-m", "unittest", "tests.test_deploy_cli", "-v"
    ], cwd=Path(__file__).parent.parent)

    return result.returncode == 0

def run_pytest():
    """Run tests using pytest (if available)"""
    print("Running tests with pytest...")
    print("=" * 50)

    try:
        # Try to run pytest from parent directory
        result = subprocess.run([
            sys.executable, "-m", "pytest", "tests/test_deploy_cli.py", "-v", "--tb=short"
        ], cwd=Path(__file__).parent.parent)

        return result.returncode == 0
    except FileNotFoundError:
        print("pytest not found, falling back to unittest")
        return False

def run_coverage():
    """Run tests with coverage (if available)"""
    print("Running tests with coverage...")
    print("=" * 50)

    try:
        # Run coverage from parent directory
        subprocess.run([
            sys.executable, "-m", "coverage", "run", "-m", "unittest", "tests.test_deploy_cli"
        ], cwd=Path(__file__).parent.parent, check=True)

        subprocess.run([
            sys.executable, "-m", "coverage", "report", "-m"
        ], cwd=Path(__file__).parent.parent)

        print("\nGenerating HTML coverage report...")
        subprocess.run([
            sys.executable, "-m", "coverage", "html"
        ], cwd=Path(__file__).parent.parent)

        print("Coverage HTML report generated in htmlcov/")
        return True
    except (FileNotFoundError, subprocess.CalledProcessError):
        print("Coverage not available")
        return False

def main():
    """Main test runner"""
    if len(sys.argv) > 1:
        test_type = sys.argv[1].lower()
    else:
        test_type = "auto"

    print("Deploy CLI Test Runner")
    print("=" * 50)
    print(f"Python: {sys.executable}")
    print(f"Working directory: {Path(__file__).parent.parent}")
    print(f"Test type: {test_type}")
    print()

    if test_type == "pytest":
        success = run_pytest()
        if not success:
            print("pytest failed or not available, trying unittest...")
            success = run_unittest()
    elif test_type == "unittest":
        success = run_unittest()
    elif test_type == "coverage":
        success = run_coverage()
        if not success:
            print("Coverage failed, running basic tests...")
            success = run_unittest()
    else:  # auto
        # Try pytest first, fallback to unittest
        success = run_pytest()
        if not success:
            success = run_unittest()

    if success:
        print("\n" + "=" * 50)
        print("✅ All tests passed!")
        return 0
    else:
        print("\n" + "=" * 50)
        print("❌ Some tests failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())

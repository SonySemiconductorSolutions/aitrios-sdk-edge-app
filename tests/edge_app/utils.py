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

from contextlib import contextmanager
import json
import signal
import subprocess
import sys
import time
import socket
from typing import Iterator, Tuple
from constants import VALGRIND_LOG
from constants import INTEGRATION_TEST_LOG
import jsondiff

RETRY_COUNT = 30
RETRY_DELAY = 3  # seconds
HOST = "localhost"
PORT = 8080
import time
import os
import re
def kill_processes_by_keyword(keyword: str):
    """
    Find and kill processes whose command line contains the given keyword.
    Excludes the current Python process and the grep itself.
    """
    try:
        # Get process list with 'ps aux'
        result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)
        lines = result.stdout.strip().split('\n')
        for line in lines:
            if keyword in line and 'grep' not in line and 'python' not in line:
                parts = line.split()
                pid = int(parts[1])
                print(f"Killing PID {pid}: {line}")
                os.kill(pid, signal.SIGKILL)  # or signal.SIGTERM for graceful shutdown
    except Exception as e:
        print(f"[ERROR] Failed to kill processes: {e}")
def wait_for_log_match(log_file: str, pattern: str, timeout: float = 10.0):
    """
    Wait until a line matching the given regex pattern appears in the log file.
    """
    regex = re.compile(pattern)
    start_time = time.time()
    pos = 0
    while not os.path.exists(log_file):
        if time.time() - start_time > timeout:
            raise TimeoutError(f"{log_file} not found within {timeout} seconds.")
        time.sleep(0.1)
    with open(log_file, 'r') as f:
        while time.time() - start_time < timeout:
            f.seek(pos)
            line = f.readline()
            if line:
                pos = f.tell()
                if regex.search(line):
                    print(f"[OK] Matched log: {line.strip()}")
                    return line.strip()
            else:
                time.sleep(0.1)
    raise TimeoutError(f"Pattern '{pattern}' not found in {log_file} within {timeout} seconds.")
def run_valgrind(app_path: str, valgrind_log: str) -> subprocess.Popen:
    """Run Valgrind and capture its output."""
    kill_processes_by_keyword("edge_app")
    edge_app_process = subprocess.Popen(
        [
            "valgrind",
            "--error-exitcode=1",
            "--leak-check=full",
            "--track-origins=yes",
            "--show-leak-kinds=all",
            f"--log-file={valgrind_log}",
            app_path
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    wait_for_log_match(INTEGRATION_TEST_LOG, "Running State Machine")
    return edge_app_process
def run_python_edge_app(app_path: str) -> subprocess.Popen:
    """Run Python and capture its output."""
    return subprocess.Popen(
        [
            sys.executable,
            app_path,
            "--integration-test"
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

@contextmanager
def manage_edge_app(app_path: str, file_path: str) -> Iterator[Tuple[dict, subprocess.Popen]]:
    """
    Context manager to handle the lifecycle of an edge application running with valgrind.

    This function starts a mocked edge app with valgrind, reads the initial configuration from a file,
    sends the default configuration to the edge application, and ensures the edge app is gracefully termianted

    :return: tuple containing the configuration data as a dictionary and the Valgrind process object.
    """
    process = run_valgrind(app_path, VALGRIND_LOG)
    print("Valgrind process started.")
    with open(file_path, "r") as f:
        data = json.load(f)["edge_app"]

    # send default configuration
    send_data(json.dumps(data))
    yield data, process
    # trigger exit
    send_data("\n")
    process.wait()

@contextmanager
def manage_python_edge_app(app_path: str, file_path: str) -> Iterator[Tuple[dict, subprocess.Popen]]:
    """
    Context manager to handle the lifecycle of an Python edge application.

    This function starts a mocked Python edge app, reads the initial configuration from a file,
    sends the default configuration to the edge application, and ensures the edge app is gracefully termianted

    :return: tuple containing the configuration data as a dictionary and the Python process object.
    """
    process = run_python_edge_app(app_path)
    print("Python process started.")
    with open(file_path, "r") as f:
        data = json.load(f)["edge_app"]

    # send default configuration
    send_data(json.dumps(data))
    yield data, process

    # trigger exit
    send_data("\n")
    process.wait()

def send_data(data: str | dict, retry_count: int = RETRY_COUNT, host: str = HOST, port: int = PORT, retry_delay: int = RETRY_DELAY) -> None:
    """Send data to the specified host and port with retries."""
    if isinstance(data, dict):
        data = json.dumps(data)
    for attempt in range(retry_count):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((host, port))
                s.sendall(data.encode())
            if data != "\n" :
                print("Data sent.")
            else:
                print("End.")
            return
        except ConnectionRefusedError:
            print(f"Connection refused. Retrying in {retry_delay} seconds... ({attempt + 1}/{retry_count})")
            time.sleep(retry_delay)
    raise ConnectionError(f"Failed to connect to {host}:{port} after {retry_count} attempts.")

def print_valgrind_summary(log_file: str) -> None:
    """Print the Valgrind summary from the log file."""
    with open(log_file, "r") as f:
        found = False
        for line in f:
            if "HEAP SUMMARY" in line:
                found = True
            if found:
                print(line.strip())

def print_dtdl_state_log(dtdl_log: str) -> None:
    with open(dtdl_log, "r") as f:
        prev = next(f)
        if prev:
            prev = json.loads(prev)
            print(f"DTDL 0:")
            print(f"{json.dumps(prev)}\n")
        for i, line in enumerate(f):
            print(f"DTDL {i+1}:")
            act = json.loads(line)
            print(f"{jsondiff.diff(prev, act)}\n")
            prev = act

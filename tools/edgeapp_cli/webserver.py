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

import argparse
import os
import logging
from http.server import HTTPServer
from http.server import SimpleHTTPRequestHandler
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any
from typing import Callable
from typing import Optional
from typing import Tuple
from datetime import datetime

# Create a logger for the webserver
webserver_logger = logging.getLogger("webserver_logger")
webserver_logger.setLevel(logging.INFO)

# Ensure the results directory exists
log_dir = Path("server_logs")
log_dir.mkdir(parents=True, exist_ok=True)  # Create the directory if it doesn't exist

# Generate a unique log file name using the current timestamp
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
log_file_path = log_dir / f"webserver_{timestamp}.log"

# Setup logging to file and console for the webserver only
file_handler = logging.FileHandler(log_file_path)
console_handler = logging.StreamHandler()

# Define a formatter
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

file_handler.setFormatter(formatter)
console_handler.setFormatter(formatter)

webserver_logger.addHandler(file_handler)
webserver_logger.addHandler(console_handler)

# Log a message after logging is configured
absolute_log_file_path = log_file_path.resolve()  # Get the absolute path
webserver_logger.info(f"Saving webserver logs to: {absolute_log_file_path}")


def get_range(range_header: str, file_size: int) -> Tuple[int, int]:
    """Parse HTTP Range header and return start and end positions"""
    unit, byte_range = range_header.split("=")
    assert unit == "bytes"

    start, end = (int(x) if x else None for x in byte_range.split("-"))
    start = start or 0
    end = min(end or file_size - 1, file_size - 1)
    return start, end


class CustomHTTPRequestHandler(SimpleHTTPRequestHandler):
    # Class variable to control logging
    suppress_logs = False

    def __init__(
        self,
        *args: Any,
        on_incoming: Optional[Callable] = None,
        directory: Path,
        **kwargs: Any,
    ):
        self.on_incoming = on_incoming
        self.dir = directory
        # Set the directory for SimpleHTTPRequestHandler
        super().__init__(*args, directory=str(directory), **kwargs)

    def log_message(self, format, *args):
        """Override log_message to control logging"""
        if not self.suppress_logs:
            # Only log to our custom logger, not to stderr
            webserver_logger.info(f"{self.address_string()} - {format % args}")

    @classmethod
    def set_log_suppression(cls, suppress: bool):
        """Class method to control log suppression"""
        cls.suppress_logs = suppress

    def do_GET(self) -> None:
        """Handle GET requests with HTTP Range support for AI model downloads"""
        try:
            # Get the requested file path
            file_path = Path(self.dir) / self.path.lstrip("/")

            # Check if file exists
            if not file_path.exists() or not file_path.is_file():
                self.send_error(404, "File Not Found")
                return

            file_size = file_path.stat().st_size
            header_range = self.headers.get("Range")
            webserver_logger.debug(f"GET request for {file_path}, Range header: {header_range}")

            with file_path.open("rb") as f:
                if header_range:
                    # Handle HTTP Range requests (partial content)
                    try:
                        start, end = get_range(header_range, file_size)
                        content_range = f"bytes {start}-{end}/{file_size}"

                        webserver_logger.debug(f"Content range: {content_range}")
                        if start > end or start >= file_size:
                            self.send_error(416, "Range Not Satisfiable")
                            return

                        f.seek(start)
                        data = f.read(end - start + 1)

                        # Send partial content response (206)
                        self.send_response(206)
                        self.send_header("Content-Range", content_range)
                        self.send_header("Accept-Ranges", "bytes")
                        webserver_logger.info(f"Serving partial content {start}-{end} of {file_size} bytes for {file_path}")
                    except ValueError:
                        self.send_error(416, "Range Not Satisfiable")
                        return
                else:
                    # Handle normal GET requests (full content)
                    data = f.read()
                    self.send_response(200)
                    webserver_logger.info(f"Serving full content {file_size} bytes for {file_path}")

            self.send_header("Content-Length", str(len(data)))
            self.send_header("Content-Type", "application/octet-stream")
            self.end_headers()
            self.wfile.write(data)

        except Exception as e:
            webserver_logger.error(f"Error handling GET request: {e}")
            self.send_error(500, "Internal Server Error")

    def do_POST(self) -> None:
        try:
            self.connection.settimeout(15)
            content_length = int(self.headers.get("Content-Length"))
            webserver_logger.info(f"Receiving POST request, size: {content_length} bytes")
            data = self.rfile.read(content_length)
            dest_path = Path(self.dir) / self.path.lstrip("/")
            os.makedirs(dest_path.parent, exist_ok=True)
            dest_path.write_bytes(data)
            webserver_logger.info(f"Saved file to: {dest_path}")
            saved_path = dest_path
        except OSError as exc:
            if exc.errno == 36:  # OSError: [Errno 36] File name too long
                # If path is too long, os.makedirs for saving data to file is failed.
                # As fallback, saving data to file in self.dir.
                webserver_logger.info(f"File name is too long {dest_path}")
                try:
                    fallback_dest_path = Path(self.dir) / "_fallback_" / dest_path.name
                    webserver_logger.info(f"fallback path is {fallback_dest_path}")
                    os.makedirs(fallback_dest_path.parent, exist_ok=True)
                    fallback_dest_path.write_bytes(data)
                    webserver_logger.info(f"Saved file to: {fallback_dest_path}")
                    fallback_dest_path_info = Path(self.dir) / "_fallback_" / f"{dest_path.name}_info"
                    fallback_dest_path_info.write_text(dest_path.as_posix())
                    saved_path = fallback_dest_path
                except Exception as e:
                    raise AssertionError(f"Error while receiving data: {e}")
            else:
                raise AssertionError(f"Error while receiving data: {e}")
        except Exception as e:
            webserver_logger.error(f"Error while receiving data: {e}")
            raise AssertionError(f"Error while receiving data: {e}")
        finally:
            self.send_response(200)
            self.end_headers()

        # Notify of new file when the callback is set
        if data and self.on_incoming:
            try:
                self.on_incoming(saved_path)
            except Exception as e:
                webserver_logger.error(f"Error while invoking callback: {e}")
                raise AssertionError(f"Error while invoking callback: {e}")

        # Check whether the saved file is a subframe
        if self.is_subframe(dest_path):
            self.combine_subframes(dest_path)

    do_PUT = do_POST

    def is_subframe(self, file_path: Path) -> bool:
        """Check if a file is a subframe based on its name"""
        return "_of_" in file_path.name

    def combine_subframes(self, src_file_path: Path) -> None:
        """Combine subframes into a single file for each timestamp"""
        dest_path = src_file_path.parent

        # Extract the timestamp, current_num, and division_num from the filename
        parts = src_file_path.stem.split('_')
        if len(parts) != 4:
            print(f"Error: the provided file name has only {len(parts)} parts")
            return

        timestamp, current_num, _, division_num = parts
        try:
            current_num = int(current_num)
            division_num = int(division_num)
        except ValueError:
            print("Error: current_num or division_num is not an integer.")
            return

        # Combine subframes when the last subframe is received
        # (The current_num of the input tensor should start from 2)
        if current_num >= 2 and  division_num > 1 and current_num == division_num:
            # Find subframe files with the same timestamp to combine
            subframe_files = sorted(dest_path.glob(f"{timestamp}_*_of_*.*"))
            total_subframes = division_num - 1
            if len(subframe_files) != total_subframes:
                print(
                    f"Error: {len(subframe_files)} subframes found, "
                    f"expected {total_subframes} subframes"
                )

            # Output file path
            combined_file_path = dest_path / f"{timestamp}{src_file_path.suffix}"

            # Combine the subframes and write them to the output file
            with open(combined_file_path, "wb") as combined_file:
                for file_path in subframe_files:
                    with open(file_path, "rb") as subframe_file:
                        combined_file.write(subframe_file.read())

            # Detele subframe files
            for file_path in subframe_files:
                file_path.unlink()

def create_handler(on_incoming: Callable, directory: Path) -> Callable:
    def handler(*args, **kwargs):
        return CustomHTTPRequestHandler(
            *args, on_incoming=on_incoming, directory=directory, **kwargs
        )
    return handler

def start_server(upload_dir_path: Path, port: int = 8000) -> None:

    def on_incoming_callback(dest_path: Path):
        webserver_logger.info(f"Received file at: {dest_path}")
        assert dest_path.exists()

    with HTTPServer(
        ("", port), create_handler(on_incoming_callback, upload_dir_path)
    ) as httpd:
        host, server_port = httpd.socket.getsockname()
        webserver_logger.info(f"Server running at http://{host}:{server_port}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            webserver_logger.info("Server stopped")
        finally:
            webserver_logger.info("Closing the server")
            httpd.server_close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a web server")
    parser.add_argument(
        "--port", type=int, default=8000, help="Port number to run the server on."
    )
    parser.add_argument(
        "--upload-dir", type=Path, help="The path where files will be stored."
    )
    args = parser.parse_args()

    if args.upload_dir:
        upload_dir_path: Path = args.upload_dir
        assert upload_dir_path.exists(), "--upload-dir: directory does not exist."
        start_server(upload_dir_path, args.port)
    else:
        with TemporaryDirectory(prefix="EdgeApp_") as tempdir:
            start_server(Path(tempdir), args.port)

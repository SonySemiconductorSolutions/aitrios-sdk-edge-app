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
Colored Logger Module for Deploy CLI
Provides colored logging output based on log levels
"""

import logging
import sys


class Colors:
    """ANSI color codes for terminal output"""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    RESET = '\033[0m'  # Reset to default color
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


class ColoredFormatter(logging.Formatter):
    """Custom formatter that adds colors based on log level"""

    # Color mapping for different log levels
    LEVEL_COLORS = {
        logging.DEBUG: Colors.CYAN,
        logging.INFO: Colors.RESET,  # No color for info
        logging.WARNING: Colors.YELLOW,
        logging.ERROR: Colors.RED,
        logging.CRITICAL: Colors.RED + Colors.BOLD,
    }

    # Symbols for different log levels
    LEVEL_SYMBOLS = {
        logging.DEBUG: "🔍",
        logging.INFO: " ",
        logging.WARNING: "⚠️",
        logging.ERROR: "❌",
        logging.CRITICAL: "🚨",
    }

    def __init__(self, fmt=None, datefmt=None, use_colors=True, use_symbols=True):
        super().__init__(fmt, datefmt)
        self.use_colors = use_colors
        self.use_symbols = use_symbols

    def format(self, record):
        # Get the original formatted message
        original_format = super().format(record)

        if not self.use_colors:
            return original_format

        # Get color for this log level
        color = self.LEVEL_COLORS.get(record.levelno, Colors.RESET)

        # Get symbol for this log level
        symbol = ""
        if self.use_symbols:
            symbol = self.LEVEL_SYMBOLS.get(record.levelno, "")
            if symbol:
                symbol = f"{symbol} "

        # Apply color formatting
        if color != Colors.RESET:
            colored_message = f"{color}{symbol}{record.getMessage()}{Colors.RESET}"
        else:
            colored_message = f"{symbol}{record.getMessage()}"

        return colored_message


class ColoredLogger:
    """Enhanced logger with colored output and convenience methods"""

    def __init__(self, name, level=logging.INFO, use_colors=True, use_symbols=True):
        self.logger = logging.getLogger(name)
        self.logger.setLevel(level)

        # Remove existing handlers to avoid duplication
        for handler in self.logger.handlers[:]:
            self.logger.removeHandler(handler)

        # Create console handler
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(level)

        # Create colored formatter
        formatter = ColoredFormatter(
            use_colors=use_colors,
            use_symbols=use_symbols
        )
        console_handler.setFormatter(formatter)

        self.logger.addHandler(console_handler)

        # Prevent propagation to avoid duplicate messages
        self.logger.propagate = False

    def debug(self, message):
        """Log debug message in cyan"""
        self.logger.debug(message)

    def info(self, message):
        """Log info message with no color"""
        self.logger.info(message)

    def warning(self, message):
        """Log warning message in yellow"""
        self.logger.warning(message)

    def error(self, message):
        """Log error message in red"""
        self.logger.error(message)

    def critical(self, message):
        """Log critical message in bold red"""
        self.logger.critical(message)

    def success(self, message):
        """Log success message in green (custom method)"""
        # Use info level but with green color
        original_message = f"{Colors.GREEN}✓ {message}{Colors.RESET}"
        print(original_message)

    def tip(self, message):
        """Log tip message in cyan (custom method)"""
        # Use info level but with cyan color
        original_message = f"{Colors.CYAN}💡 TIP: {message}{Colors.RESET}"
        print(original_message)


def get_colored_logger(name, level=logging.INFO, use_colors=True, use_symbols=True):
    """Factory function to create a colored logger instance"""
    return ColoredLogger(name, level, use_colors, use_symbols)


# Create a default logger instance for the module
default_logger = get_colored_logger("edgeapp_cli")

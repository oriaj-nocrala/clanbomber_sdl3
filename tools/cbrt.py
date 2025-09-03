#!/usr/bin/env python3
"""
ClanBomber Refactor Tool (CBRT) - Main executable
Advanced refactoring and code analysis tool for ClanBomber
"""

import sys
from pathlib import Path

# Add the cbrt package to the Python path
sys.path.insert(0, str(Path(__file__).parent))

from cbrt.cli import main

if __name__ == '__main__':
    sys.exit(main())
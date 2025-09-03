#!/usr/bin/env python3
"""
ClanBomber Refactor Tool Dashboard Launcher
Quick launcher script for the web dashboard
"""

import subprocess
import sys
import os
from pathlib import Path

def check_dependencies():
    """Check if required dependencies are installed"""
    try:
        import flask
        return True
    except ImportError:
        return False

def install_dependencies():
    """Install required dependencies"""
    requirements_file = Path(__file__).parent / "requirements.txt"
    
    if requirements_file.exists():
        print("Installing dashboard dependencies...")
        subprocess.check_call([
            sys.executable, "-m", "pip", "install", "-r", str(requirements_file)
        ])
        return True
    else:
        print("Requirements file not found. Installing Flask manually...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "flask"])
        return True

def main():
    print("🚀 Starting ClanBomber Refactor Tool Dashboard...")
    
    # Check if dependencies are installed
    if not check_dependencies():
        print("Missing dependencies. Installing...")
        try:
            install_dependencies()
        except subprocess.CalledProcessError:
            print("❌ Failed to install dependencies. Please run:")
            print("   pip install flask")
            return 1
    
    # Launch the Flask app
    app_file = Path(__file__).parent / "app.py"
    
    if not app_file.exists():
        print("❌ Dashboard app not found!")
        return 1
    
    print("🌐 Dashboard starting at http://localhost:5000")
    print("📊 Analyzing ClanBomber codebase...")
    print("💡 Press Ctrl+C to stop the dashboard")
    print()
    
    # Run the Flask app
    os.environ['FLASK_APP'] = str(app_file)
    os.environ['FLASK_ENV'] = 'development'
    
    try:
        subprocess.run([sys.executable, str(app_file)])
    except KeyboardInterrupt:
        print("\n👋 Dashboard stopped.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
# ClanBomber Refactor Tool (CBRT)

A modular, professional-grade refactoring and code analysis tool specifically designed for the ClanBomber codebase.

## 🚀 Features

- **Advanced Function Analysis**: Detects unused functions with verification system
- **Magic Number Detection**: Identifies hardcoded values and suggests constants
- **Commented Code Analysis**: Finds and scores commented-out code blocks
- **Modular Architecture**: Clean, extensible design with plugin support
- **Multiple Output Formats**: Text, HTML, Markdown, and JSON reports
- **Safe Cleanup Operations**: Conservative approach with confidence scoring
- **CLI Interface**: Professional command-line interface with comprehensive help

## 📦 Installation

The tool is self-contained and requires only Python 3.7+:

```bash
# Make the main script executable
chmod +x cbrt.py

# Run from anywhere (optional: add to PATH)
ln -s $(pwd)/cbrt.py /usr/local/bin/cbrt
```

## 🔧 Usage

### Basic Analysis

```bash
# Analyze entire codebase
python3 cbrt.py analyze src/

# Analyze with custom output
python3 cbrt.py analyze src/ --output my_analysis.json

# Analyze specific aspects only
python3 cbrt.py analyze src/ --functions
python3 cbrt.py analyze src/ --magic-numbers
python3 cbrt.py analyze src/ --commented-code
```

### Generate Reports

```bash
# Text report (default)
python3 cbrt.py report analysis.json

# HTML report
python3 cbrt.py report analysis.json --format html --output report.html

# Markdown report
python3 cbrt.py report analysis.json --format markdown --output report.md

# Filter by severity
python3 cbrt.py report analysis.json --severity high
```

### List Findings

```bash
# List all findings
python3 cbrt.py list analysis.json

# Filter by type
python3 cbrt.py list analysis.json --type functions
python3 cbrt.py list analysis.json --type magic_numbers

# Show only high-confidence findings
python3 cbrt.py list analysis.json --high-confidence
```

### Safe Cleanup Operations

```bash
# Preview cleanup operations (dry-run)
python3 cbrt.py clean src/ --dry-run

# Apply safe cleanup operations
python3 cbrt.py clean src/ --apply --backup

# Use existing analysis for cleanup
python3 cbrt.py clean src/ --analysis my_analysis.json --apply
```

## 🏗️ Architecture

```
cbrt/
├── __init__.py              # Package initialization
├── cli.py                   # Command-line interface
├── core/                    # Core analysis engine
│   ├── __init__.py
│   ├── engine.py           # Main analysis engine
│   └── models.py           # Data models and types
├── analyzers/              # Specialized analyzers (future)
├── commands/               # CLI command handlers
├── utils/                  # Utility functions
├── config/                 # Configuration management
└── tests/                  # Test suite
```

## 📊 Example Output

```
🎮 CLANBOMBER REFACTOR TOOL - ANALYSIS SUMMARY
============================================================

📊 PROJECT STATISTICS:
  Source files: 99
  Functions: 670
  Function calls: 6857
  Magic numbers: 375
  Commented blocks: 41
  Total findings: 30

🚨 FINDINGS BY SEVERITY:
  🔶 Medium: 22
  💡 Low: 8

🔍 TOP FINDINGS:
  🔶 Magic number: 8 (used 12 times)
      📍 MapEntry.cpp:26
  🔶 Magic number: 255 (used 48 times)
      📍 SettingsScreen.cpp:54
  💡 Commented code block (4 lines)
      📍 GPUAcceleratedRenderer.cpp:773
```

## ⚙️ Configuration

Create a `cbrt_config.json` file for project-specific settings:

```json
{
  "name": "ClanBomber",
  "exclude_dirs": ["_deps", "build", "CMakeFiles"],
  "magic_number_suggestions": {
    "255": "COLOR_MAX",
    "4": "PLAYER_COUNT",
    "8": "TILE_SIZE"
  },
  "confidence_threshold": 0.8,
  "always_used_patterns": [
    "main", "SDL_main", ".*_callback$", "^on_.*"
  ]
}
```

## 🔍 Analysis Types

### Function Analysis
- **Unused Function Detection**: Identifies functions that are never called
- **Verification System**: Uses multiple heuristics to avoid false positives
- **Callback Recognition**: Understands framework patterns (SDL, OpenGL)
- **Confidence Scoring**: Provides reliability metrics (0.0-1.0)

### Magic Number Analysis  
- **Context-Aware Detection**: Skips obvious non-magic numbers (0, 1, 2)
- **ClanBomber-Specific Suggestions**: Knows common game development constants
- **Usage Frequency**: Prioritizes numbers that appear multiple times

### Commented Code Analysis
- **Pattern Recognition**: Scores likelihood of being actual code
- **Block Detection**: Groups consecutive commented lines
- **Confidence Rating**: Helps prioritize cleanup efforts

## 🛡️ Safety Features

- **Conservative Approach**: Never suggests removing potentially used code
- **Verification System**: Multiple validation layers before marking code as unused
- **Backup Support**: Optional backups before applying changes
- **Dry-run Mode**: Preview all operations before execution

## 🔄 Integration with ClanBomber Development

### Recommended Workflow

1. **Regular Analysis**: Run weekly analysis during development
2. **Pre-commit Checks**: Use in CI/CD pipeline to catch new magic numbers
3. **Refactoring Sessions**: Use findings to guide cleanup efforts
4. **Code Reviews**: Include CBRT reports in review process

### Example CI Integration

```yaml
# .github/workflows/code-analysis.yml
name: Code Analysis
on: [push, pull_request]

jobs:
  analyze:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Run CBRT Analysis
        run: |
          cd tools
          python3 cbrt.py analyze ../src --output analysis.json
          python3 cbrt.py report analysis.json --format markdown > analysis_report.md
      - name: Upload Analysis Report
        uses: actions/upload-artifact@v2
        with:
          name: analysis-report
          path: tools/analysis_report.md
```

## 🚀 Future Enhancements

- **Plugin System**: Support for custom analyzers
- **IDE Integration**: VS Code and CLion plugins
- **Interactive Mode**: TUI interface for guided cleanup
- **Metrics Dashboard**: Web-based visualization
- **Git Integration**: Analysis of changes over time
- **Auto-fix Suggestions**: Automated code transformations

## 📝 Contributing

1. **Add New Analyzers**: Extend `cbrt/analyzers/`
2. **Improve Detection**: Enhance pattern recognition
3. **Add Output Formats**: Support more report formats
4. **Extend CLI**: Add new commands and options

## 📄 License

Part of the ClanBomber project. See main project license.

---

**🎮 Built specifically for ClanBomber development workflow**
"""
ClanBomber Refactor Tool - Web Dashboard
Flask server for live code analysis visualization
"""

from flask import Flask, render_template, jsonify, request
import json
import os
import sys
from pathlib import Path
from datetime import datetime

# Add parent directory to path for imports
sys.path.append(str(Path(__file__).parent.parent))

from core.engine import AnalysisEngine
from core.models import AnalysisResult, FunctionStatus, AnalysisType, Severity

app = Flask(__name__, 
           template_folder='../web/templates',
           static_folder='../web/static')

# Global analysis result cache
_analysis_cache = None
_last_analysis_time = None

def get_project_root():
    """Get the ClanBomber project root directory"""
    return Path(__file__).parent.parent.parent.parent

def run_analysis():
    """Run fresh analysis and cache results"""
    global _analysis_cache, _last_analysis_time
    
    engine = AnalysisEngine()
    project_root = get_project_root()
    src_dir = project_root / "src"
    
    if not src_dir.exists():
        return None
    
    print(f"Running analysis on {src_dir}...")
    # Run complete analysis
    result = engine.run_analysis(str(src_dir))
    
    _analysis_cache = result
    _last_analysis_time = datetime.now()
    
    return result

def get_cached_analysis():
    """Get cached analysis or run new one if needed"""
    global _analysis_cache, _last_analysis_time
    
    # Run analysis if no cache or cache is old (5 minutes)
    if (_analysis_cache is None or 
        _last_analysis_time is None or 
        (datetime.now() - _last_analysis_time).seconds > 300):
        return run_analysis()
    
    return _analysis_cache

@app.route('/')
def dashboard():
    """Main dashboard page"""
    return render_template('dashboard.html')

@app.route('/api/analysis/overview')
def analysis_overview():
    """Get analysis overview statistics"""
    result = get_cached_analysis()
    if not result:
        return jsonify({'error': 'Failed to run analysis'}), 500
    
    overview = {
        'project_name': result.project_name,
        'last_updated': _last_analysis_time.isoformat() if _last_analysis_time else None,
        'statistics': {
            'total_files': len(result.source_files),
            'total_functions': len(result.functions),
            'unused_functions': len(result.get_unused_functions()),
            'magic_numbers': len(result.magic_numbers),
            'commented_blocks': len(result.commented_blocks),
            'total_findings': len(result.findings)
        }
    }
    
    return jsonify(overview)

@app.route('/api/analysis/functions')
def functions_data():
    """Get functions analysis data"""
    result = get_cached_analysis()
    if not result:
        return jsonify({'error': 'Failed to run analysis'}), 500
    
    functions_data = []
    for name, func in result.functions.items():
        functions_data.append({
            'name': name,
            'file': func.location.filename,
            'line': func.location.line,
            'class_name': func.class_name,
            'status': func.status.value,
            'confidence': func.confidence,
            'calls_to_count': len(func.calls_to),
            'called_from_count': len(func.called_from),
            'is_static': func.is_static,
            'is_virtual': func.is_virtual
        })
    
    return jsonify(functions_data)

@app.route('/api/analysis/findings')
def findings_data():
    """Get findings data grouped by type and severity"""
    result = get_cached_analysis()
    if not result:
        return jsonify({'error': 'Failed to run analysis'}), 500
    
    findings_by_type = {}
    findings_by_severity = {}
    
    for finding in result.findings:
        # Group by type
        type_key = finding.type.value
        if type_key not in findings_by_type:
            findings_by_type[type_key] = []
        findings_by_type[type_key].append({
            'id': finding.id,
            'title': finding.title,
            'description': finding.description,
            'severity': finding.severity.value,
            'file': finding.location.filename,
            'line': finding.location.line,
            'suggestions': finding.suggestions
        })
        
        # Group by severity
        sev_key = finding.severity.value
        if sev_key not in findings_by_severity:
            findings_by_severity[sev_key] = 0
        findings_by_severity[sev_key] += 1
    
    return jsonify({
        'by_type': findings_by_type,
        'by_severity': findings_by_severity
    })

@app.route('/api/analysis/magic-numbers')
def magic_numbers_data():
    """Get magic numbers analysis"""
    result = get_cached_analysis()
    if not result:
        return jsonify({'error': 'Failed to run analysis'}), 500
    
    magic_data = []
    for magic in result.magic_numbers:
        magic_data.append({
            'value': magic.value,
            'file': magic.location.filename,
            'line': magic.location.line,
            'context': magic.context[:100] + '...' if len(magic.context) > 100 else magic.context,
            'suggested_constant': magic.suggested_constant,
            'category': magic.category,
            'usage_count': magic.usage_count
        })
    
    return jsonify(magic_data)

@app.route('/api/analysis/refresh')
def refresh_analysis():
    """Force refresh of analysis data"""
    result = run_analysis()
    if not result:
        return jsonify({'error': 'Failed to run analysis'}), 500
    
    return jsonify({
        'status': 'success',
        'timestamp': _last_analysis_time.isoformat(),
        'functions_analyzed': len(result.functions)
    })

@app.route('/api/analysis/file-stats')
def file_stats():
    """Get statistics per source file"""
    result = get_cached_analysis()
    if not result:
        return jsonify({'error': 'Failed to run analysis'}), 500
    
    file_stats = {}
    
    # Initialize file stats
    for file_path in result.source_files:
        filename = Path(file_path).name
        file_stats[filename] = {
            'functions': 0,
            'unused_functions': 0,
            'magic_numbers': 0,
            'commented_blocks': 0,
            'findings': 0
        }
    
    # Count functions per file
    for func in result.functions.values():
        filename = func.location.filename
        if filename in file_stats:
            file_stats[filename]['functions'] += 1
            if func.status == FunctionStatus.UNUSED:
                file_stats[filename]['unused_functions'] += 1
    
    # Count magic numbers per file
    for magic in result.magic_numbers:
        filename = magic.location.filename
        if filename in file_stats:
            file_stats[filename]['magic_numbers'] += 1
    
    # Count commented blocks per file
    for block in result.commented_blocks:
        filename = block.location.filename
        if filename in file_stats:
            file_stats[filename]['commented_blocks'] += 1
    
    # Count findings per file
    for finding in result.findings:
        filename = finding.location.filename
        if filename in file_stats:
            file_stats[filename]['findings'] += 1
    
    return jsonify(file_stats)

if __name__ == '__main__':
    print("Starting ClanBomber Refactor Tool Dashboard...")
    print("Dashboard will be available at http://localhost:5000")
    app.run(debug=True, host='0.0.0.0', port=5000)
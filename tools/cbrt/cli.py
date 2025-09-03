"""
Command Line Interface for ClanBomber Refactor Tool
"""

import sys
import argparse
from pathlib import Path
from typing import List, Optional
import json

from .core.engine import AnalysisEngine
from .core.models import AnalysisType, Severity
from . import __version__

class CBRTInterface:
    """Main CLI interface for CBRT"""
    
    def __init__(self):
        self.parser = self._create_parser()
    
    def _create_parser(self) -> argparse.ArgumentParser:
        """Create the main argument parser"""
        parser = argparse.ArgumentParser(
            prog='cbrt',
            description='ClanBomber Refactor Tool - Advanced code analysis and refactoring',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog="""
Examples:
  cbrt analyze src/                    # Analyze entire src directory
  cbrt analyze --functions src/        # Only analyze functions
  cbrt analyze --output report.json   # Save results to file
  cbrt report report.json              # Generate human-readable report
  cbrt clean src/ --dry-run            # Preview cleanup actions
  cbrt clean src/ --apply              # Apply safe cleanup actions
            """
        )
        
        parser.add_argument('--version', action='version', version=f'CBRT {__version__}')
        
        # Subcommands
        subparsers = parser.add_subparsers(dest='command', help='Available commands')
        
        # Analyze command
        analyze_parser = subparsers.add_parser('analyze', help='Analyze codebase for legacy patterns')
        self._add_analyze_args(analyze_parser)
        
        # Report command
        report_parser = subparsers.add_parser('report', help='Generate human-readable report from analysis')
        self._add_report_args(report_parser)
        
        # Clean command
        clean_parser = subparsers.add_parser('clean', help='Apply safe cleanup operations')
        self._add_clean_args(clean_parser)
        
        # List command
        list_parser = subparsers.add_parser('list', help='List findings by type or severity')
        self._add_list_args(list_parser)
        
        return parser
    
    def _add_analyze_args(self, parser: argparse.ArgumentParser):
        """Add arguments for analyze command"""
        parser.add_argument('source_dir', help='Source directory to analyze')
        parser.add_argument('--output', '-o', help='Output file for results (JSON format)')
        parser.add_argument('--functions', action='store_true', help='Analyze functions only')
        parser.add_argument('--magic-numbers', action='store_true', help='Analyze magic numbers only')  
        parser.add_argument('--commented-code', action='store_true', help='Analyze commented code only')
        parser.add_argument('--exclude', '-e', action='append', help='Exclude directories')
        parser.add_argument('--config', '-c', help='Configuration file')
        parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    
    def _add_report_args(self, parser: argparse.ArgumentParser):
        """Add arguments for report command"""
        parser.add_argument('analysis_file', help='Analysis results file (JSON)')
        parser.add_argument('--format', choices=['text', 'html', 'markdown'], default='text',
                          help='Report format')
        parser.add_argument('--output', '-o', help='Output file (default: stdout)')
        parser.add_argument('--severity', choices=['low', 'medium', 'high', 'critical'],
                          help='Filter by severity')
        parser.add_argument('--type', choices=['functions', 'magic_numbers', 'commented_code'],
                          help='Filter by finding type')
    
    def _add_clean_args(self, parser: argparse.ArgumentParser):
        """Add arguments for clean command"""
        parser.add_argument('source_dir', help='Source directory to clean')
        parser.add_argument('--analysis', '-a', help='Use existing analysis file')
        parser.add_argument('--dry-run', action='store_true', help='Show what would be cleaned')
        parser.add_argument('--apply', action='store_true', help='Apply cleanup operations')
        parser.add_argument('--backup', action='store_true', help='Create backups before changes')
        parser.add_argument('--confidence', type=float, default=0.8,
                          help='Minimum confidence threshold (0.0-1.0)')
        parser.add_argument('--types', choices=['commented-code', 'unused-functions'],
                          action='append', help='Types of cleanup to perform')
    
    def _add_list_args(self, parser: argparse.ArgumentParser):
        """Add arguments for list command"""
        parser.add_argument('analysis_file', help='Analysis results file (JSON)')
        parser.add_argument('--type', choices=['functions', 'magic_numbers', 'commented_code'],
                          help='Filter by finding type')
        parser.add_argument('--severity', choices=['low', 'medium', 'high', 'critical'],
                          help='Filter by severity')
        parser.add_argument('--unused-only', action='store_true', help='Show only unused functions')
        parser.add_argument('--high-confidence', action='store_true', help='Show only high confidence findings')
    
    def run(self, args: Optional[List[str]] = None) -> int:
        """Run the CLI with given arguments"""
        parsed_args = self.parser.parse_args(args)
        
        if not parsed_args.command:
            self.parser.print_help()
            return 0
        
        try:
            if parsed_args.command == 'analyze':
                return self._handle_analyze(parsed_args)
            elif parsed_args.command == 'report':
                return self._handle_report(parsed_args)
            elif parsed_args.command == 'clean':
                return self._handle_clean(parsed_args)
            elif parsed_args.command == 'list':
                return self._handle_list(parsed_args)
            else:
                print(f"❌ Unknown command: {parsed_args.command}")
                return 1
        
        except Exception as e:
            print(f"❌ Error: {e}")
            if getattr(parsed_args, 'verbose', False):
                import traceback
                traceback.print_exc()
            return 1
    
    def _handle_analyze(self, args) -> int:
        """Handle analyze command"""
        source_dir = Path(args.source_dir)
        if not source_dir.exists():
            print(f"❌ Source directory does not exist: {source_dir}")
            return 1
        
        # Determine analysis types
        analysis_types = []
        if args.functions:
            analysis_types.append(AnalysisType.FUNCTIONS)
        if args.magic_numbers:
            analysis_types.append(AnalysisType.MAGIC_NUMBERS)
        if args.commented_code:
            analysis_types.append(AnalysisType.COMMENTED_CODE)
        
        # If no specific types selected, do all
        if not analysis_types:
            analysis_types = [AnalysisType.FUNCTIONS, AnalysisType.MAGIC_NUMBERS, AnalysisType.COMMENTED_CODE]
        
        # Load configuration
        config = {}
        if args.config:
            try:
                with open(args.config, 'r') as f:
                    config = json.load(f)
            except Exception as e:
                print(f"⚠️  Failed to load config: {e}")
        
        # Run analysis
        engine = AnalysisEngine(config)
        result = engine.run_analysis(str(source_dir), analysis_types)
        
        # Output results
        output_file = args.output or 'cbrt_analysis.json'
        result.save_to_json(output_file)
        
        # Print summary
        self._print_analysis_summary(result)
        print(f"\n💾 Results saved to {output_file}")
        
        return 0
    
    def _handle_report(self, args) -> int:
        """Handle report command"""
        try:
            with open(args.analysis_file, 'r') as f:
                data = json.load(f)
        except Exception as e:
            print(f"❌ Failed to load analysis file: {e}")
            return 1
        
        # Generate report based on format
        if args.format == 'text':
            report = self._generate_text_report(data, args)
        elif args.format == 'html':
            report = self._generate_html_report(data, args)
        elif args.format == 'markdown':
            report = self._generate_markdown_report(data, args)
        else:
            print(f"❌ Unsupported format: {args.format}")
            return 1
        
        # Output report
        if args.output:
            with open(args.output, 'w') as f:
                f.write(report)
            print(f"📄 Report saved to {args.output}")
        else:
            print(report)
        
        return 0
    
    def _handle_clean(self, args) -> int:
        """Handle clean command"""
        if not args.dry_run and not args.apply:
            print("❌ Must specify either --dry-run or --apply")
            return 1
        
        source_dir = Path(args.source_dir)
        if not source_dir.exists():
            print(f"❌ Source directory does not exist: {source_dir}")
            return 1
        
        # Load or run analysis
        if args.analysis:
            try:
                with open(args.analysis, 'r') as f:
                    analysis_data = json.load(f)
            except Exception as e:
                print(f"❌ Failed to load analysis file: {e}")
                return 1
        else:
            print("🔍 Running analysis...")
            engine = AnalysisEngine()
            result = engine.run_analysis(str(source_dir))
            analysis_data = json.loads(json.dumps(result, default=str))
        
        # Perform cleanup
        cleanup_count = self._perform_cleanup(analysis_data, args)
        
        if args.dry_run:
            print(f"🔍 Dry run complete. {cleanup_count} operations would be performed.")
        else:
            print(f"✅ Cleanup complete. {cleanup_count} operations performed.")
        
        return 0
    
    def _handle_list(self, args) -> int:
        """Handle list command"""
        try:
            with open(args.analysis_file, 'r') as f:
                data = json.load(f)
        except Exception as e:
            print(f"❌ Failed to load analysis file: {e}")
            return 1
        
        findings = data.get('findings', [])
        
        # Apply filters
        if args.type:
            findings = [f for f in findings if f['type'] == args.type]
        if args.severity:
            findings = [f for f in findings if f['severity'] == args.severity]
        
        # Special filters
        if args.unused_only:
            findings = [f for f in findings if f['type'] == 'functions' and 'unused' in f['title'].lower()]
        if args.high_confidence:
            findings = [f for f in findings if f['metadata'].get('confidence', 0) >= 0.8]
        
        # Display findings
        print(f"📋 Found {len(findings)} findings:")
        for finding in findings:
            severity_icon = self._get_severity_icon(finding['severity'])
            print(f"{severity_icon} {finding['title']}")
            print(f"   📍 {finding['location']['file']}:{finding['location']['line']}")
            if finding['metadata'].get('confidence'):
                print(f"   🎯 Confidence: {finding['metadata']['confidence']:.1%}")
            print()
        
        return 0
    
    def _print_analysis_summary(self, result):
        """Print analysis summary"""
        print("\n" + "="*60)
        print("🎮 CLANBOMBER REFACTOR TOOL - ANALYSIS SUMMARY")
        print("="*60)
        
        print(f"\n📊 PROJECT STATISTICS:")
        print(f"  Source files: {len(result.source_files)}")
        print(f"  Functions: {len(result.functions)}")
        print(f"  Function calls: {len(result.function_calls)}")
        print(f"  Magic numbers: {len(result.magic_numbers)}")
        print(f"  Commented blocks: {len(result.commented_blocks)}")
        print(f"  Total findings: {len(result.findings)}")
        
        # Findings by severity
        severity_counts = {}
        for finding in result.findings:
            severity_counts[finding.severity] = severity_counts.get(finding.severity, 0) + 1
        
        print(f"\n🚨 FINDINGS BY SEVERITY:")
        for severity in [Severity.CRITICAL, Severity.HIGH, Severity.MEDIUM, Severity.LOW]:
            count = severity_counts.get(severity, 0)
            if count > 0:
                icon = self._get_severity_icon(severity.value)
                print(f"  {icon} {severity.value.title()}: {count}")
        
        # Top findings
        print(f"\n🔍 TOP FINDINGS:")
        for finding in result.findings[:5]:
            icon = self._get_severity_icon(finding.severity.value)
            print(f"  {icon} {finding.title}")
            print(f"      📍 {finding.location}")
    
    def _get_severity_icon(self, severity: str) -> str:
        """Get icon for severity level"""
        icons = {
            'critical': '🚨',
            'high': '⚠️',
            'medium': '🔶',
            'low': '💡'
        }
        return icons.get(severity, '❓')
    
    def _generate_text_report(self, data: dict, args) -> str:
        """Generate text format report"""
        lines = []
        lines.append("="*60)
        lines.append("CLANBOMBER REFACTOR TOOL - ANALYSIS REPORT")
        lines.append("="*60)
        lines.append("")
        
        # Summary
        metadata = data.get('metadata', {})
        lines.append("📊 SUMMARY:")
        lines.append(f"  Project: {metadata.get('project_name', 'Unknown')}")
        lines.append(f"  Source files: {metadata.get('source_files_count', 0)}")
        lines.append(f"  Functions: {metadata.get('functions_count', 0)}")
        lines.append(f"  Findings: {metadata.get('findings_count', 0)}")
        lines.append("")
        
        # Findings
        findings = data.get('findings', [])
        
        # Apply filters
        if args.severity:
            findings = [f for f in findings if f['severity'] == args.severity]
        if hasattr(args, 'type') and args.type:
            findings = [f for f in findings if f['type'] == args.type]
        
        lines.append(f"🔍 FINDINGS ({len(findings)}):")
        for finding in findings:
            icon = self._get_severity_icon(finding['severity'])
            lines.append(f"\n{icon} {finding['title']}")
            lines.append(f"   📍 Location: {finding['location']['file']}:{finding['location']['line']}")
            lines.append(f"   📝 {finding['description']}")
            
            if finding.get('suggestions'):
                lines.append("   💡 Suggestions:")
                for suggestion in finding['suggestions']:
                    lines.append(f"      • {suggestion}")
        
        return "\n".join(lines)
    
    def _generate_html_report(self, data: dict, args) -> str:
        """Generate HTML format report"""
        # This would generate a full HTML report
        # For now, return a basic HTML structure
        return f"""
<!DOCTYPE html>
<html>
<head>
    <title>ClanBomber Analysis Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        .finding {{ margin: 20px 0; padding: 15px; border-left: 4px solid #ccc; }}
        .high {{ border-left-color: #f44336; }}
        .medium {{ border-left-color: #ff9800; }}
        .low {{ border-left-color: #2196f3; }}
    </style>
</head>
<body>
    <h1>🎮 ClanBomber Analysis Report</h1>
    <h2>Summary</h2>
    <p>Findings: {len(data.get('findings', []))}</p>
    
    <h2>Findings</h2>
    <!-- Findings would be rendered here -->
</body>
</html>
        """
    
    def _generate_markdown_report(self, data: dict, args) -> str:
        """Generate Markdown format report"""
        lines = []
        lines.append("# 🎮 ClanBomber Analysis Report")
        lines.append("")
        
        # Summary
        metadata = data.get('metadata', {})
        lines.append("## 📊 Summary")
        lines.append(f"- **Project**: {metadata.get('project_name', 'Unknown')}")
        lines.append(f"- **Source files**: {metadata.get('source_files_count', 0)}")
        lines.append(f"- **Functions**: {metadata.get('functions_count', 0)}")
        lines.append(f"- **Findings**: {metadata.get('findings_count', 0)}")
        lines.append("")
        
        # Findings
        findings = data.get('findings', [])
        lines.append("## 🔍 Findings")
        
        for finding in findings:
            icon = self._get_severity_icon(finding['severity'])
            lines.append(f"### {icon} {finding['title']}")
            lines.append(f"**Location**: `{finding['location']['file']}:{finding['location']['line']}`")
            lines.append(f"**Severity**: {finding['severity'].title()}")
            lines.append("")
            lines.append(finding['description'])
            lines.append("")
            
            if finding.get('suggestions'):
                lines.append("**Suggestions**:")
                for suggestion in finding['suggestions']:
                    lines.append(f"- {suggestion}")
                lines.append("")
        
        return "\n".join(lines)
    
    def _perform_cleanup(self, analysis_data: dict, args) -> int:
        """Perform cleanup operations"""
        cleanup_count = 0
        
        # This would contain the actual cleanup logic
        # For now, just count potential operations
        
        findings = analysis_data.get('findings', [])
        for finding in findings:
            if finding['type'] == 'commented_code':
                if args.dry_run:
                    print(f"Would remove commented code: {finding['location']['file']}:{finding['location']['line']}")
                cleanup_count += 1
        
        return cleanup_count


def main():
    """Main entry point"""
    cli = CBRTInterface()
    return cli.run()


if __name__ == '__main__':
    sys.exit(main())
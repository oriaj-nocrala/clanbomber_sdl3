#!/usr/bin/env python3
"""
ClanBomber Refactoring Analyzer v2.0
Improved version with better function detection and verification system.
"""

import os
import re
import json
from pathlib import Path
from collections import defaultdict, Counter
from dataclasses import dataclass, asdict
from typing import Dict, List, Set, Tuple, Optional

@dataclass
class FunctionInfo:
    name: str
    file: str
    line: int
    signature: str
    class_name: Optional[str] = None
    is_static: bool = False
    is_virtual: bool = False
    is_constructor: bool = False
    is_destructor: bool = False
    is_inline: bool = False
    verification_status: str = "unknown"  # unknown, verified_used, verified_unused, needs_manual_check

@dataclass
class FunctionCall:
    caller_function: Optional[str]
    called_function: str
    file: str
    line: int
    context: str

@dataclass
class VerificationResult:
    function_name: str
    status: str  # used, unused, uncertain
    confidence: float  # 0.0-1.0
    evidence: List[str]
    manual_check_needed: bool = False

class ClanBomberRefactorAnalyzerV2:
    def __init__(self, src_dir: str = "src"):
        self.src_dir = Path(src_dir)
        self.source_files = []
        
        # Analysis results
        self.functions: Dict[str, FunctionInfo] = {}
        self.function_calls: List[FunctionCall] = []
        self.magic_numbers: List = []
        self.commented_blocks: List = []
        
        # Verification system
        self.verification_results: Dict[str, VerificationResult] = {}
        
        # ClanBomber-specific knowledge
        self.clanbomber_knowledge = self._init_clanbomber_knowledge()
    
    def _init_clanbomber_knowledge(self) -> Dict:
        """Initialize ClanBomber-specific patterns and knowledge"""
        return {
            # Libraries and their function patterns
            'library_functions': {
                'cglm': ['glm_', 'GLM_'],
                'sdl': ['SDL_', 'sdl_'],
                'opengl': ['gl', 'GL_'],
                'stb': ['stb_', 'STB_'],
            },
            
            # Framework callback patterns
            'callback_patterns': [
                r'.*_callback$', r'^on_.*', r'.*_handler$',
                r'^handle_.*', r'.*_listener$'
            ],
            
            # Lifecycle functions that might not show direct calls
            'lifecycle_patterns': [
                r'^init.*', r'^initialize.*', r'^setup.*',
                r'^cleanup.*', r'^shutdown.*', r'^destroy.*',
                r'^update.*', r'^render.*', r'^draw.*',
                r'^tick$', r'^act$', r'^show$'
            ],
            
            # Entry points
            'entry_points': ['main', 'SDL_main', 'WinMain'],
            
            # Virtual function indicators
            'virtual_indicators': ['virtual', 'override'],
            
            # Files to exclude from unused function analysis
            'exclude_files': ['test', 'example', 'demo'],
            
            # Magic number suggestions
            'magic_number_suggestions': {
                255: ['COLOR_MAX', 'ALPHA_MAX', 'UINT8_MAX'],
                4: ['PLAYER_COUNT', 'DIRECTION_COUNT', 'RGBA_COMPONENTS'],
                3: ['RGB_COMPONENTS', 'XYZ_COMPONENTS', 'MAX_POWER_LEVEL'],
                8: ['TILE_SIZE', 'ANIMATION_FRAMES', 'BITS_PER_BYTE'],
                20: ['DEFAULT_SPEED', 'TIMER_INTERVAL', 'GRID_SIZE'],
                10: ['DECIMAL_BASE', 'DEFAULT_COUNT', 'GRID_SIZE'],
                100: ['PERCENT_MAX', 'SCORE_BASE', 'DISTANCE_THRESHOLD'],
                150: ['UI_WIDTH', 'MENU_WIDTH'],
                200: ['UI_HEIGHT', 'MENU_HEIGHT'],
            }
        }
    
    def find_source_files(self):
        """Find ClanBomber source files"""
        self.source_files = []
        
        for pattern in ['*.cpp', '*.h', '*.hpp']:
            for file in self.src_dir.rglob(pattern):
                # Skip dependencies, build artifacts, and test files
                if any(skip in str(file) for skip in ['_deps', 'CMakeFiles', 'glad-generated', 'build']):
                    continue
                self.source_files.append(file)
        
        print(f"📁 Found {len(self.source_files)} ClanBomber source files")
    
    def is_library_function(self, func_name: str) -> bool:
        """Check if a function name belongs to a known library"""
        for lib_name, prefixes in self.clanbomber_knowledge['library_functions'].items():
            if any(func_name.startswith(prefix) for prefix in prefixes):
                return True
        return False
    
    def extract_functions_improved(self, file_path: Path):
        """Extract function definitions with improved detection"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
            
            # Skip files that are primarily library wrappers
            if any(lib in str(file_path).lower() for lib in ['glad', 'stb_', 'cglm']):
                return
            
            in_multiline_comment = False
            
            for i, line in enumerate(lines):
                line_stripped = line.strip()
                
                # Handle multi-line comments
                if '/*' in line_stripped:
                    in_multiline_comment = True
                if '*/' in line_stripped:
                    in_multiline_comment = False
                    continue
                if in_multiline_comment:
                    continue
                
                # Skip single line comments and preprocessor directives
                if (line_stripped.startswith('//') or 
                    line_stripped.startswith('#')):
                    continue
                
                # Enhanced patterns for function detection
                patterns = [
                    # Class methods with return type: ReturnType Class::method(...) {
                    r'^(?:(?:static|inline|virtual)\s+)*([a-zA-Z_][a-zA-Z0-9_<>:*&\s]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*::\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:const)?\s*\{',
                    
                    # Regular functions with return type: ReturnType function(...) {  
                    r'^(?:(?:static|inline|virtual)\s+)*([a-zA-Z_][a-zA-Z0-9_<>:*&\s]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:const)?\s*\{',
                    
                    # Simple functions: function(...) { (for constructors, etc)
                    r'^(?:(?:static|inline|virtual)\s+)*([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:const)?\s*\{',
                ]
                
                for pattern_idx, pattern in enumerate(patterns):
                    match = re.match(pattern, line_stripped)
                    if match:
                        if pattern_idx == 0:  # Class::method
                            return_type = match.group(1).strip()
                            class_name = match.group(2)
                            func_name = match.group(3)
                            full_name = f"{class_name}::{func_name}"
                        elif pattern_idx == 1:  # typed function
                            return_type = match.group(1).strip()
                            func_name = match.group(2)
                            full_name = func_name
                            class_name = None
                        else:  # simple function
                            func_name = match.group(1)
                            full_name = func_name
                            class_name = None
                            return_type = "unknown"
                        
                        # Skip obvious false positives
                        if (len(func_name) <= 1 or 
                            func_name in ['if', 'for', 'while', 'switch', 'case', 'return'] or
                            self.is_library_function(func_name)):
                            continue
                        
                        # Determine function characteristics
                        is_static = 'static' in line_stripped
                        is_virtual = 'virtual' in line_stripped
                        is_inline = 'inline' in line_stripped
                        is_constructor = (func_name == class_name) if class_name else False
                        is_destructor = func_name.startswith('~')
                        
                        self.functions[full_name] = FunctionInfo(
                            name=func_name,
                            file=str(file_path),
                            line=i + 1,
                            signature=line_stripped,
                            class_name=class_name,
                            is_static=is_static,
                            is_virtual=is_virtual,
                            is_constructor=is_constructor,
                            is_destructor=is_destructor,
                            is_inline=is_inline
                        )
                        break
        
        except Exception as e:
            print(f"❌ Error extracting functions from {file_path}: {e}")
    
    def extract_function_calls_improved(self, file_path: Path):
        """Extract function calls with better context tracking"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
            
            current_function = None
            in_multiline_comment = False
            
            for i, line in enumerate(lines):
                line_stripped = line.strip()
                
                # Handle comments
                if '/*' in line_stripped:
                    in_multiline_comment = True
                if '*/' in line_stripped:
                    in_multiline_comment = False
                    continue
                if in_multiline_comment or line_stripped.startswith('//'):
                    continue
                
                # Track current function context
                if re.match(r'^[a-zA-Z_][a-zA-Z0-9_<>:*&\s]*\s*\([^)]*\)\s*\{', line_stripped):
                    # Extract function name from signature
                    func_match = re.search(r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\(', line_stripped)
                    if func_match:
                        current_function = func_match.group(1)
                
                # Find function calls with better patterns
                call_patterns = [
                    # Class::method(...)
                    r'([a-zA-Z_][a-zA-Z0-9_]*)\s*::\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(',
                    # function(...)
                    r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\(',
                ]
                
                for pattern in call_patterns:
                    for match in re.finditer(pattern, line_stripped):
                        if '::' in pattern:
                            class_name = match.group(1)
                            method_name = match.group(2)
                            called_func = f"{class_name}::{method_name}"
                            simple_name = method_name
                        else:
                            called_func = match.group(1)
                            simple_name = called_func
                        
                        # Skip obvious false positives
                        if (called_func in ['if', 'for', 'while', 'switch', 'return', 'sizeof', 'typeof'] or
                            len(called_func) <= 2):
                            continue
                        
                        # Create call record
                        call_record = FunctionCall(
                            caller_function=current_function,
                            called_function=called_func,
                            file=str(file_path),
                            line=i + 1,
                            context=line_stripped[:100]
                        )
                        self.function_calls.append(call_record)
                        
                        # Also record simple name if it's a method call
                        if '::' in called_func:
                            simple_call = FunctionCall(
                                caller_function=current_function,
                                called_function=simple_name,
                                file=str(file_path),
                                line=i + 1,
                                context=line_stripped[:100]
                            )
                            self.function_calls.append(simple_call)
        
        except Exception as e:
            print(f"❌ Error extracting calls from {file_path}: {e}")
    
    def verify_function_usage(self, func_name: str, func_info: FunctionInfo) -> VerificationResult:
        """Verify if a function is actually used with high confidence"""
        evidence = []
        confidence = 0.0
        needs_manual = False
        
        # Check if it's an obvious always-used function
        if func_name in self.clanbomber_knowledge['entry_points']:
            return VerificationResult(func_name, "used", 1.0, ["Entry point function"], False)
        
        if func_info.is_constructor or func_info.is_destructor:
            return VerificationResult(func_name, "used", 0.9, ["Constructor/destructor"], False)
        
        if func_info.is_virtual:
            return VerificationResult(func_name, "used", 0.8, ["Virtual function - called via polymorphism"], False)
        
        # Check callback patterns
        for pattern in self.clanbomber_knowledge['callback_patterns']:
            if re.match(pattern, func_name):
                return VerificationResult(func_name, "used", 0.7, ["Callback pattern"], False)
        
        # Check lifecycle patterns  
        for pattern in self.clanbomber_knowledge['lifecycle_patterns']:
            if re.match(pattern, func_name):
                confidence += 0.3
                evidence.append("Lifecycle function pattern")
        
        # Find direct calls
        direct_calls = [call for call in self.function_calls if call.called_function == func_name]
        full_name_calls = [call for call in self.function_calls if call.called_function == f"{func_info.class_name}::{func_name}"] if func_info.class_name else []
        
        all_calls = direct_calls + full_name_calls
        
        if all_calls:
            # Check if calls are from different files (stronger evidence)
            call_files = set(call.file for call in all_calls)
            definition_file = func_info.file
            
            external_calls = [f for f in call_files if f != definition_file]
            if external_calls:
                confidence += 0.8
                evidence.append(f"Called from {len(external_calls)} different files")
            elif len(all_calls) > 2:  # Multiple calls in same file
                confidence += 0.5  
                evidence.append(f"Multiple calls ({len(all_calls)}) in same file")
            else:
                confidence += 0.3
                evidence.append("Called within same file")
                needs_manual = True
        
        # If no evidence of usage found
        if confidence < 0.3:
            # Check if it's a utility function that might be used indirectly
            if any(keyword in func_name.lower() for keyword in ['util', 'helper', 'get', 'set', 'is', 'has']):
                needs_manual = True
                evidence.append("Utility function - needs manual verification")
            else:
                confidence = 0.9  # High confidence it's unused
                evidence.append("No calls found")
        
        # Determine status
        if confidence >= 0.7:
            status = "used"
        elif confidence <= 0.3:
            status = "unused"
        else:
            status = "uncertain" 
            needs_manual = True
        
        return VerificationResult(func_name, status, confidence, evidence, needs_manual)
    
    def run_verification_system(self):
        """Run comprehensive verification on all functions"""
        print("🔍 Running verification system...")
        
        for func_name, func_info in self.functions.items():
            verification = self.verify_function_usage(func_name, func_info)
            self.verification_results[func_name] = verification
            func_info.verification_status = verification.status
    
    def generate_advanced_report(self):
        """Generate advanced report with verification results"""
        # Run verification
        self.run_verification_system()
        
        # Categorize results
        verified_unused = {k: v for k, v in self.verification_results.items() if v.status == "unused" and v.confidence >= 0.8}
        needs_manual = {k: v for k, v in self.verification_results.items() if v.manual_check_needed}
        verified_used = {k: v for k, v in self.verification_results.items() if v.status == "used"}
        
        print("\n" + "="*80)
        print("🎮 CLANBOMBER REFACTORING ANALYZER V2.0 - ADVANCED REPORT")
        print("="*80)
        
        print(f"\n📊 VERIFICATION SUMMARY:")
        print(f"  Total functions analyzed: {len(self.functions)}")
        print(f"  Verified as USED: {len(verified_used)}")
        print(f"  Verified as UNUSED (safe to remove): {len(verified_unused)}")
        print(f"  Need manual verification: {len(needs_manual)}")
        print(f"  Function calls found: {len(self.function_calls)}")
        
        print(f"\n✅ VERIFIED UNUSED FUNCTIONS (safe to remove):")
        for func_name, verification in list(verified_unused.items())[:10]:
            func_info = self.functions[func_name]
            print(f"  🗑️  {func_name}")
            print(f"      📍 {Path(func_info.file).name}:{func_info.line}")
            print(f"      🎯 Confidence: {verification.confidence:.1%}")
            print(f"      📝 Evidence: {', '.join(verification.evidence)}")
            print()
        
        print(f"\n⚠️  NEED MANUAL VERIFICATION:")
        for func_name, verification in list(needs_manual.items())[:8]:
            func_info = self.functions[func_name]
            print(f"  🤔 {func_name}")
            print(f"      📍 {Path(func_info.file).name}:{func_info.line}")
            print(f"      🎯 Confidence: {verification.confidence:.1%}")
            print(f"      📝 Reason: {', '.join(verification.evidence)}")
            print()
        
        # Save comprehensive results
        results = {
            'metadata': {
                'analyzer_version': '2.0',
                'project': 'ClanBomber',
                'verification_enabled': True
            },
            'verification_summary': {
                'total_functions': len(self.functions),
                'verified_used': len(verified_used),
                'verified_unused': len(verified_unused),
                'need_manual_check': len(needs_manual)
            },
            'verified_unused_functions': {k: {
                'function_info': asdict(self.functions[k]),
                'verification': asdict(v)
            } for k, v in verified_unused.items()},
            'manual_verification_needed': {k: {
                'function_info': asdict(self.functions[k]),
                'verification': asdict(v)
            } for k, v in needs_manual.items()},
            'all_functions': {k: asdict(v) for k, v in self.functions.items()},
            'function_calls': [asdict(call) for call in self.function_calls]
        }
        
        output_file = 'clanbomber_refactor_analysis_v2.json'
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2, default=str)
        
        print(f"\n💾 Comprehensive results saved to {output_file}")
        print("\n✨ Advanced analysis complete! Verification system provides high confidence results.")
        
        return results
    
    def run_analysis(self):
        """Run complete analysis with verification"""
        print("🚀 Starting ClanBomber Advanced Refactoring Analysis...")
        
        self.find_source_files()
        
        print("📝 Extracting functions with improved detection...")
        for file_path in self.source_files:
            self.extract_functions_improved(file_path)
        
        print("🔗 Extracting function calls with context tracking...")
        for file_path in self.source_files:
            self.extract_function_calls_improved(file_path)
        
        return self.generate_advanced_report()


def main():
    import sys
    src_dir = sys.argv[1] if len(sys.argv) > 1 else "../src"
    
    analyzer = ClanBomberRefactorAnalyzerV2(src_dir)
    analyzer.run_analysis()


if __name__ == "__main__":
    main()
"""
Core analysis engine for ClanBomber Refactor Tool
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Set, Optional
from collections import defaultdict
import uuid

from .models import (
    AnalysisResult, FunctionInfo, FunctionCall, MagicNumber, 
    CommentedCodeBlock, Finding, SourceLocation, FunctionStatus,
    AnalysisType, Severity
)

class AnalysisEngine:
    """Core engine that orchestrates different analyzers"""
    
    def __init__(self, project_config: Optional[Dict] = None):
        self.config = project_config or {}
        self.source_files: List[Path] = []
        self.result = AnalysisResult(
            project_name=self.config.get('name', 'ClanBomber'),
            source_files=[]
        )
        
    def discover_source_files(self, src_dir: Path, 
                            extensions: List[str] = None,
                            exclude_dirs: List[str] = None) -> List[Path]:
        """Discover source files in the project"""
        if extensions is None:
            extensions = ['.cpp', '.h', '.hpp', '.cc', '.cxx']
        
        if exclude_dirs is None:
            exclude_dirs = ['_deps', 'CMakeFiles', 'build', '.git', 'glad-generated']
        
        files = []
        for ext in extensions:
            for file in src_dir.rglob(f'*{ext}'):
                # Skip excluded directories
                if any(excl in str(file) for excl in exclude_dirs):
                    continue
                files.append(file)
        
        self.source_files = files
        self.result.source_files = [str(f) for f in files]
        return files
    
    def run_analysis(self, src_dir: str, 
                    analysis_types: List[AnalysisType] = None) -> AnalysisResult:
        """Run complete analysis"""
        src_path = Path(src_dir)
        
        if not src_path.exists():
            raise ValueError(f"Source directory does not exist: {src_dir}")
        
        if analysis_types is None:
            analysis_types = [
                AnalysisType.FUNCTIONS,
                AnalysisType.MAGIC_NUMBERS,
                AnalysisType.COMMENTED_CODE
            ]
        
        print(f"🔍 Starting analysis of {src_path}")
        
        # Discover source files
        self.discover_source_files(src_path)
        print(f"📁 Found {len(self.source_files)} source files")
        
        # Run different types of analysis
        if AnalysisType.FUNCTIONS in analysis_types:
            self._analyze_functions()
        
        if AnalysisType.MAGIC_NUMBERS in analysis_types:
            self._analyze_magic_numbers()
        
        if AnalysisType.COMMENTED_CODE in analysis_types:
            self._analyze_commented_code()
        
        # Generate findings
        self._generate_findings()
        
        print(f"✅ Analysis complete: {len(self.result.findings)} findings")
        return self.result
    
    def _analyze_functions(self):
        """Analyze functions and their usage"""
        print("🔧 Analyzing functions...")
        
        # First pass: extract function definitions
        for file_path in self.source_files:
            self._extract_functions_from_file(file_path)
        
        # Second pass: extract function calls
        for file_path in self.source_files:
            self._extract_function_calls_from_file(file_path)
        
        # Third pass: verify function usage
        self._verify_function_usage()
        
        print(f"📊 Found {len(self.result.functions)} functions")
    
    def _extract_functions_from_file(self, file_path: Path):
        """Extract function definitions from a single file"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            # Skip library files
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
                if in_multiline_comment or line_stripped.startswith('//'):
                    continue
                
                # Function definition patterns
                patterns = [
                    # Class::method
                    r'^(?:(?:static|inline|virtual)\s+)*([a-zA-Z_][a-zA-Z0-9_<>:*&\s]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*::\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:const)?\s*\{',
                    # Regular function
                    r'^(?:(?:static|inline|virtual)\s+)*([a-zA-Z_][a-zA-Z0-9_<>:*&\s]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*(?:const)?\s*\{',
                    # Simple function (constructors, etc)
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
                        
                        # Skip invalid names and library functions
                        if (len(func_name) <= 1 or 
                            func_name in ['if', 'for', 'while', 'switch'] or
                            self._is_library_function(func_name)):
                            continue
                        
                        location = SourceLocation(str(file_path), i + 1)
                        
                        func_info = FunctionInfo(
                            name=func_name,
                            location=location,
                            signature=line_stripped,
                            class_name=class_name,
                            is_static='static' in line_stripped,
                            is_virtual='virtual' in line_stripped,
                            is_inline='inline' in line_stripped,
                            is_constructor=(func_name == class_name) if class_name else False,
                            is_destructor=func_name.startswith('~')
                        )
                        
                        self.result.functions[full_name] = func_info
                        break
        
        except Exception as e:
            print(f"❌ Error analyzing {file_path}: {e}")
    
    def _extract_function_calls_from_file(self, file_path: Path):
        """Extract function calls from a single file"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
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
                
                # Track current function
                func_match = re.match(r'^[a-zA-Z_][a-zA-Z0-9_<>:*&\s]*\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(.*\)\s*\{', line_stripped)
                if func_match:
                    current_function = func_match.group(1)
                
                # Find function calls
                call_patterns = [
                    r'([a-zA-Z_][a-zA-Z0-9_]*)\s*::\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(',  # Class::method
                    r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\('  # function
                ]
                
                for pattern in call_patterns:
                    for match in re.finditer(pattern, line_stripped):
                        if '::' in pattern:
                            class_name = match.group(1)
                            method_name = match.group(2)
                            called_func = f"{class_name}::{method_name}"
                        else:
                            called_func = match.group(1)
                        
                        # Skip keywords and invalid names
                        if (called_func in ['if', 'for', 'while', 'switch', 'return', 'sizeof'] or
                            len(called_func) <= 2):
                            continue
                        
                        location = SourceLocation(str(file_path), i + 1)
                        call = FunctionCall(
                            caller_function=current_function,
                            called_function=called_func,
                            location=location,
                            context=line_stripped[:100]
                        )
                        self.result.function_calls.append(call)
        
        except Exception as e:
            print(f"❌ Error extracting calls from {file_path}: {e}")
    
    def _verify_function_usage(self):
        """Verify which functions are actually used"""
        # Build call graph
        called_functions = set()
        for call in self.result.function_calls:
            called_functions.add(call.called_function)
        
        # Verify each function
        for func_name, func_info in self.result.functions.items():
            evidence = []
            confidence = 0.0
            
            # Always-used patterns
            if (func_name in ['main', 'SDL_main'] or
                func_info.is_constructor or func_info.is_destructor or
                func_info.is_virtual):
                func_info.status = FunctionStatus.USED
                func_info.confidence = 1.0
                func_info.evidence = ["Always-used function type"]
                continue
            
            # Check if called
            simple_name = func_info.name
            is_called = (func_name in called_functions or simple_name in called_functions)
            
            if is_called:
                # Find calls from different files
                calls = [c for c in self.result.function_calls 
                        if c.called_function in [func_name, simple_name]]
                call_files = set(c.location.file for c in calls)
                
                if func_info.location.file not in call_files:
                    confidence = 0.9
                    evidence.append(f"Called from {len(call_files)} external files")
                elif len(calls) > 2:
                    confidence = 0.7
                    evidence.append(f"Multiple calls ({len(calls)}) in same file")
                else:
                    confidence = 0.5
                    evidence.append("Called within same file")
                    
                func_info.status = FunctionStatus.USED
            else:
                # Check if it's a callback or lifecycle function
                callback_patterns = [r'.*_callback$', r'^on_.*', r'.*_handler$', r'^update$', r'^render$']
                if any(re.match(pattern, func_name) for pattern in callback_patterns):
                    confidence = 0.6
                    evidence.append("Callback/lifecycle pattern")
                    func_info.status = FunctionStatus.UNCERTAIN
                else:
                    confidence = 0.8
                    evidence.append("No calls found")
                    func_info.status = FunctionStatus.UNUSED
            
            func_info.confidence = confidence
            func_info.evidence = evidence
    
    def _analyze_magic_numbers(self):
        """Analyze magic numbers in the codebase"""
        print("🔢 Analyzing magic numbers...")
        
        # ClanBomber-specific suggestions
        suggestions = {
            255: 'COLOR_MAX',
            4: 'PLAYER_COUNT',
            3: 'RGB_COMPONENTS',
            8: 'TILE_SIZE',
            20: 'DEFAULT_SPEED',
            10: 'GRID_SIZE',
            100: 'SCORE_INCREMENT'
        }
        
        for file_path in self.source_files:
            if file_path.suffix != '.cpp':  # Only check .cpp files
                continue
                
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                
                for i, line in enumerate(lines):
                    # Skip comments
                    if re.search(r'^\s*//|^\s*/\*', line):
                        continue
                    
                    # Find integer literals
                    for match in re.finditer(r'\b(\d+)\b', line):
                        value = int(match.group(1))
                        
                        # Skip obvious non-magic numbers
                        if value in [0, 1, 2, -1]:
                            continue
                        
                        # Skip array indices and version numbers
                        context = line[max(0, match.start()-10):match.end()+10]
                        if re.search(r'[\[\]++--]|version|Ver|v\d', context, re.I):
                            continue
                        
                        location = SourceLocation(str(file_path), i + 1, match.start())
                        magic_num = MagicNumber(
                            value=value,
                            location=location,
                            context=line.strip(),
                            suggested_constant=suggestions.get(value)
                        )
                        self.result.magic_numbers.append(magic_num)
            
            except Exception as e:
                print(f"❌ Error analyzing magic numbers in {file_path}: {e}")
    
    def _analyze_commented_code(self):
        """Analyze commented-out code blocks"""
        print("💬 Analyzing commented code...")
        
        for file_path in self.source_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                
                current_block = []
                in_block = False
                
                for i, line in enumerate(lines):
                    stripped = line.strip()
                    
                    if stripped.startswith('//') and not stripped.startswith('///'):
                        comment_content = stripped[2:].strip()
                        
                        # Score likelihood of being code
                        code_score = 0
                        patterns = [
                            (r'\{|\}', 3),
                            (r';$', 2),
                            (r'=', 1),
                            (r'\([^)]*\)', 2),
                            (r'\b(if|for|while|int|float|void)\b', 2)
                        ]
                        
                        for pattern, score in patterns:
                            if re.search(pattern, comment_content):
                                code_score += score
                        
                        if code_score >= 2:
                            if not in_block:
                                current_block = []
                                in_block = True
                            current_block.append(line.strip())
                    else:
                        if in_block and len(current_block) >= 2:
                            start_line = i - len(current_block) + 1
                            location = SourceLocation(str(file_path), start_line)
                            
                            block = CommentedCodeBlock(
                                location=location,
                                end_line=i,
                                lines=current_block,
                                confidence=min(1.0, len(current_block) / 10.0)
                            )
                            self.result.commented_blocks.append(block)
                        
                        current_block = []
                        in_block = False
            
            except Exception as e:
                print(f"❌ Error analyzing commented code in {file_path}: {e}")
    
    def _generate_findings(self):
        """Generate findings based on analysis results"""
        # Generate findings for unused functions
        for func_name, func_info in self.result.functions.items():
            if func_info.status == FunctionStatus.UNUSED and func_info.confidence >= 0.7:
                finding = Finding(
                    id=str(uuid.uuid4())[:8],
                    type=AnalysisType.FUNCTIONS,
                    severity=Severity.MEDIUM,
                    title=f"Unused function: {func_name}",
                    description=f"Function '{func_name}' appears to be unused (confidence: {func_info.confidence:.1%})",
                    location=func_info.location,
                    suggestions=[
                        "Remove the function if confirmed unused",
                        "Add unit tests if the function should be used",
                        "Mark as static if only used internally"
                    ],
                    metadata={
                        'function_name': func_name,
                        'confidence': func_info.confidence,
                        'evidence': func_info.evidence
                    }
                )
                self.result.findings.append(finding)
        
        # Generate findings for magic numbers
        from collections import Counter
        number_counts = Counter(m.value for m in self.result.magic_numbers)
        
        for value, count in number_counts.items():
            if count >= 3:  # Appears 3+ times
                examples = [m for m in self.result.magic_numbers if m.value == value][:3]
                
                finding = Finding(
                    id=str(uuid.uuid4())[:8],
                    type=AnalysisType.MAGIC_NUMBERS,
                    severity=Severity.MEDIUM if count >= 5 else Severity.LOW,
                    title=f"Magic number: {value} (used {count} times)",
                    description=f"Number {value} appears {count} times and should be replaced with a named constant",
                    location=examples[0].location,
                    suggestions=[
                        f"Create constant: const int CONSTANT_NAME = {value};",
                        "Add to GameConstants.h",
                        "Replace all occurrences with the named constant"
                    ],
                    metadata={
                        'value': value,
                        'count': count,
                        'suggested_constant': examples[0].suggested_constant,
                        'locations': [str(e.location) for e in examples]
                    }
                )
                self.result.findings.append(finding)
        
        # Generate findings for commented code
        for block in self.result.commented_blocks:
            if block.confidence >= 0.6:
                finding = Finding(
                    id=str(uuid.uuid4())[:8],
                    type=AnalysisType.COMMENTED_CODE,
                    severity=Severity.LOW,
                    title=f"Commented code block ({block.line_count} lines)",
                    description=f"Block of commented code detected with {block.confidence:.1%} confidence",
                    location=block.location,
                    suggestions=[
                        "Remove if confirmed to be dead code",
                        "Convert to proper documentation if needed",
                        "Uncomment if the code is still needed"
                    ],
                    metadata={
                        'line_count': block.line_count,
                        'confidence': block.confidence,
                        'preview': block.preview
                    }
                )
                self.result.findings.append(finding)
    
    def _is_library_function(self, func_name: str) -> bool:
        """Check if function name belongs to a library"""
        library_prefixes = ['glm_', 'SDL_', 'GL_', 'gl', 'stb_']
        return any(func_name.startswith(prefix) for prefix in library_prefixes)
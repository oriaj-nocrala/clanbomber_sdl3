#!/usr/bin/env python3
"""
ClanBomber Legacy Code Analyzer
Analyzes C++ codebase for legacy systems, unused functions, magic numbers, and dead code.
"""

import os
import sys
import re
import json
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Set, Tuple, Any
import clang.cindex
from clang.cindex import CursorKind, TypeKind
import networkx as nx
import matplotlib.pyplot as plt
import graphviz

class LegacyCodeAnalyzer:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.source_files = []
        self.functions = {}  # function_name -> {file, line, calls, called_by}
        self.classes = {}
        self.magic_numbers = []
        self.includes = defaultdict(set)
        self.call_graph = nx.DiGraph()
        self.enums = {}
        self.macros = {}
        self.global_vars = {}
        
        # Initialize libclang
        clang.cindex.conf.set_library_file('/usr/lib/x86_64-linux-gnu/libclang-19.so.1')
        
    def find_source_files(self):
        """Find all C++ source files"""
        extensions = ['.cpp', '.cc', '.cxx', '.h', '.hpp']
        self.source_files = [
            f for f in self.source_dir.rglob('*')
            if (f.suffix in extensions and 
                not str(f).endswith('.o') and
                '_deps' not in str(f) and
                'glad-generated' not in str(f) and
                'CMakeFiles' not in str(f))
        ]
        print(f"Found {len(self.source_files)} source files (excluding dependencies)")
        
    def analyze_file(self, filepath: Path):
        """Analyze a single C++ file using libclang AST"""
        try:
            # Create translation unit
            index = clang.cindex.Index.create()
            args = ['-std=c++17', '-I./src', '-DWITH_DEBUG']
            tu = index.parse(str(filepath), args=args)
            
            if not tu:
                print(f"Failed to parse {filepath}")
                return
                
            # Walk the AST
            self._walk_ast(tu.cursor, filepath)
            
        except Exception as e:
            print(f"Error analyzing {filepath}: {e}")
    
    def _walk_ast(self, cursor, filepath: Path, depth=0):
        """Walk the AST and collect information"""
        if cursor.location.file and cursor.location.file.name != str(filepath):
            return  # Skip nodes from included files
            
        kind = cursor.kind
        
        # Function declarations/definitions
        if kind in [CursorKind.FUNCTION_DECL, CursorKind.CXX_METHOD]:
            self._analyze_function(cursor, filepath)
        
        # Class declarations
        elif kind == CursorKind.CLASS_DECL:
            self._analyze_class(cursor, filepath)
            
        # Variables and magic numbers
        elif kind == CursorKind.INTEGER_LITERAL:
            self._analyze_magic_number(cursor, filepath)
            
        # Function calls
        elif kind == CursorKind.CALL_EXPR:
            self._analyze_function_call(cursor, filepath)
            
        # Enums
        elif kind == CursorKind.ENUM_DECL:
            self._analyze_enum(cursor, filepath)
            
        # Macros
        elif kind == CursorKind.MACRO_DEFINITION:
            self._analyze_macro(cursor, filepath)
            
        # Global variables
        elif kind == CursorKind.VAR_DECL and cursor.semantic_parent.kind == CursorKind.TRANSLATION_UNIT:
            self._analyze_global_var(cursor, filepath)
        
        # Recurse into children
        for child in cursor.get_children():
            self._walk_ast(child, filepath, depth + 1)
    
    def _analyze_function(self, cursor, filepath: Path):
        """Analyze function declaration/definition"""
        func_name = cursor.spelling
        if not func_name:
            return
            
        location = cursor.location
        signature = self._get_function_signature(cursor)
        
        if func_name not in self.functions:
            self.functions[func_name] = {
                'file': str(filepath),
                'line': location.line,
                'signature': signature,
                'calls': set(),
                'called_by': set(),
                'is_definition': cursor.is_definition(),
                'is_static': cursor.storage_class == clang.cindex.StorageClass.STATIC,
                'parameters': len(list(cursor.get_arguments())),
                'return_type': cursor.result_type.spelling
            }
        
        self.call_graph.add_node(func_name, **self.functions[func_name])
    
    def _analyze_class(self, cursor, filepath: Path):
        """Analyze class declaration"""
        class_name = cursor.spelling
        if not class_name:
            return
            
        methods = []
        members = []
        
        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_METHOD:
                methods.append(child.spelling)
            elif child.kind == CursorKind.FIELD_DECL:
                members.append(child.spelling)
        
        self.classes[class_name] = {
            'file': str(filepath),
            'line': cursor.location.line,
            'methods': methods,
            'members': members,
            'is_abstract': self._is_abstract_class(cursor),
            'base_classes': self._get_base_classes(cursor)
        }
    
    def _analyze_magic_number(self, cursor, filepath: Path):
        """Analyze integer literals (potential magic numbers)"""
        try:
            value = int(cursor.get_tokens().__next__().spelling)
            
            # Skip common non-magic numbers
            if value in [0, 1, 2, -1, 10, 100, 1000]:
                return
                
            # Skip array indices and simple increments
            parent = cursor.semantic_parent
            if parent and parent.kind in [CursorKind.ARRAY_SUBSCRIPT_EXPR, CursorKind.UNARY_OPERATOR]:
                return
                
            self.magic_numbers.append({
                'value': value,
                'file': str(filepath),
                'line': cursor.location.line,
                'column': cursor.location.column,
                'context': self._get_context_around_cursor(cursor)
            })
        except:
            pass
    
    def _analyze_function_call(self, cursor, filepath: Path):
        """Analyze function calls to build call graph"""
        try:
            called_func = cursor.referenced
            if called_func and called_func.kind in [CursorKind.FUNCTION_DECL, CursorKind.CXX_METHOD]:
                called_name = called_func.spelling
                
                # Find the calling function
                calling_func = self._find_containing_function(cursor)
                if calling_func and called_name:
                    if calling_func in self.functions:
                        self.functions[calling_func]['calls'].add(called_name)
                    if called_name in self.functions:
                        self.functions[called_name]['called_by'].add(calling_func)
                    
                    self.call_graph.add_edge(calling_func, called_name)
        except:
            pass
    
    def _analyze_enum(self, cursor, filepath: Path):
        """Analyze enum declarations"""
        enum_name = cursor.spelling or "anonymous"
        values = []
        
        for child in cursor.get_children():
            if child.kind == CursorKind.ENUM_CONSTANT_DECL:
                values.append(child.spelling)
        
        self.enums[enum_name] = {
            'file': str(filepath),
            'line': cursor.location.line,
            'values': values
        }
    
    def _analyze_macro(self, cursor, filepath: Path):
        """Analyze macro definitions"""
        macro_name = cursor.spelling
        if macro_name:
            self.macros[macro_name] = {
                'file': str(filepath),
                'line': cursor.location.line
            }
    
    def _analyze_global_var(self, cursor, filepath: Path):
        """Analyze global variables"""
        var_name = cursor.spelling
        if var_name:
            self.global_vars[var_name] = {
                'file': str(filepath),
                'line': cursor.location.line,
                'type': cursor.type.spelling,
                'is_static': cursor.storage_class == clang.cindex.StorageClass.STATIC
            }
    
    def _get_function_signature(self, cursor) -> str:
        """Get function signature"""
        try:
            params = []
            for arg in cursor.get_arguments():
                params.append(f"{arg.type.spelling} {arg.spelling}")
            return f"{cursor.result_type.spelling} {cursor.spelling}({', '.join(params)})"
        except:
            return cursor.spelling or "unknown"
    
    def _is_abstract_class(self, cursor) -> bool:
        """Check if class is abstract"""
        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_METHOD:
                if child.is_pure_virtual_method():
                    return True
        return False
    
    def _get_base_classes(self, cursor) -> List[str]:
        """Get base class names"""
        bases = []
        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_BASE_SPECIFIER:
                bases.append(child.type.spelling)
        return bases
    
    def _find_containing_function(self, cursor):
        """Find the function that contains this cursor"""
        parent = cursor.semantic_parent
        while parent:
            if parent.kind in [CursorKind.FUNCTION_DECL, CursorKind.CXX_METHOD]:
                return parent.spelling
            parent = parent.semantic_parent
        return None
    
    def _get_context_around_cursor(self, cursor, lines=2) -> str:
        """Get code context around cursor"""
        try:
            with open(cursor.location.file.name, 'r') as f:
                content = f.readlines()
            
            start = max(0, cursor.location.line - lines - 1)
            end = min(len(content), cursor.location.line + lines)
            
            return ''.join(content[start:end]).strip()
        except:
            return "Could not get context"
    
    def find_unused_functions(self) -> List[str]:
        """Find functions that are never called"""
        unused = []
        
        for func_name, func_info in self.functions.items():
            # Skip main, constructors, destructors, and virtual functions
            if func_name in ['main'] or func_name.startswith('~') or '::' in func_name:
                continue
                
            if not func_info['called_by'] and not func_info.get('is_virtual', False):
                unused.append(func_name)
        
        return unused
    
    def find_dead_code(self) -> List[Dict]:
        """Find potentially dead code patterns"""
        dead_code = []
        
        # Find unreachable nodes in call graph
        if 'main' in self.call_graph:
            reachable = set(nx.descendants(self.call_graph, 'main'))
            reachable.add('main')
            
            for node in self.call_graph.nodes():
                if node not in reachable and node in self.functions:
                    dead_code.append({
                        'type': 'unreachable_function',
                        'name': node,
                        'file': self.functions[node]['file'],
                        'line': self.functions[node]['line']
                    })
        
        return dead_code
    
    def analyze_code_complexity(self) -> Dict:
        """Analyze code complexity metrics"""
        complexity = {
            'total_functions': len(self.functions),
            'total_classes': len(self.classes),
            'total_magic_numbers': len(self.magic_numbers),
            'average_function_calls': 0,
            'most_called_functions': [],
            'largest_classes': []
        }
        
        # Calculate average function calls
        if self.functions:
            total_calls = sum(len(f['calls']) for f in self.functions.values())
            complexity['average_function_calls'] = total_calls / len(self.functions)
        
        # Most called functions
        call_counts = {name: len(info['called_by']) for name, info in self.functions.items()}
        complexity['most_called_functions'] = sorted(call_counts.items(), 
                                                   key=lambda x: x[1], reverse=True)[:10]
        
        # Largest classes (by method count)
        class_sizes = {name: len(info['methods']) for name, info in self.classes.items()}
        complexity['largest_classes'] = sorted(class_sizes.items(),
                                             key=lambda x: x[1], reverse=True)[:10]
        
        return complexity
    
    def generate_reports(self):
        """Generate comprehensive analysis reports"""
        print("=== CLANBOMBER LEGACY CODE ANALYSIS ===\n")
        
        # Basic statistics
        print("📊 BASIC STATISTICS:")
        print(f"  Source files: {len(self.source_files)}")
        print(f"  Functions: {len(self.functions)}")
        print(f"  Classes: {len(self.classes)}")
        print(f"  Enums: {len(self.enums)}")
        print(f"  Global variables: {len(self.global_vars)}")
        print(f"  Magic numbers: {len(self.magic_numbers)}")
        
        # Unused functions
        unused_funcs = self.find_unused_functions()
        print(f"\n🚨 UNUSED FUNCTIONS ({len(unused_funcs)}):")
        for func in unused_funcs[:10]:  # Show first 10
            info = self.functions[func]
            print(f"  {func} - {info['file']}:{info['line']}")
        
        # Magic numbers
        print(f"\n🔢 MAGIC NUMBERS ({len(self.magic_numbers)}):")
        magic_counter = Counter(m['value'] for m in self.magic_numbers)
        for value, count in magic_counter.most_common(10):
            locations = [m for m in self.magic_numbers if m['value'] == value]
            print(f"  {value} (appears {count} times)")
            for loc in locations[:3]:  # Show first 3 locations
                print(f"    {loc['file']}:{loc['line']}")
        
        # Dead code
        dead_code = self.find_dead_code()
        print(f"\n☠️  DEAD CODE ({len(dead_code)}):")
        for item in dead_code[:10]:
            print(f"  {item['type']}: {item['name']} - {item['file']}:{item['line']}")
        
        # Complexity analysis
        complexity = self.analyze_code_complexity()
        print(f"\n📈 COMPLEXITY METRICS:")
        print(f"  Average function calls: {complexity['average_function_calls']:.2f}")
        print(f"  Most called functions:")
        for func, count in complexity['most_called_functions'][:5]:
            print(f"    {func}: {count} calls")
        print(f"  Largest classes:")
        for cls, methods in complexity['largest_classes'][:5]:
            print(f"    {cls}: {methods} methods")
    
    def save_results(self, output_file: str = "legacy_analysis.json"):
        """Save analysis results to JSON"""
        results = {
            'statistics': {
                'source_files': len(self.source_files),
                'functions': len(self.functions),
                'classes': len(self.classes),
                'magic_numbers': len(self.magic_numbers)
            },
            'unused_functions': self.find_unused_functions(),
            'magic_numbers': self.magic_numbers,
            'dead_code': self.find_dead_code(),
            'complexity': self.analyze_code_complexity(),
            'functions': {name: {**info, 'calls': list(info['calls']), 
                               'called_by': list(info['called_by'])} 
                        for name, info in self.functions.items()},
            'classes': self.classes,
            'enums': self.enums,
            'global_vars': self.global_vars
        }
        
        with open(output_file, 'w') as f:
            json.dump(results, f, indent=2, default=str)
        
        print(f"\n💾 Results saved to {output_file}")
    
    def create_system_map(self):
        """Create visual system architecture map"""
        try:
            # Create a simplified call graph with only major components
            major_components = {}
            
            # Identify major components by file grouping
            for func_name, func_info in self.functions.items():
                file_base = Path(func_info['file']).stem
                if file_base not in major_components:
                    major_components[file_base] = {
                        'functions': [],
                        'calls_external': set(),
                        'called_by_external': set()
                    }
                major_components[file_base]['functions'].append(func_name)
            
            # Build component graph
            component_graph = nx.DiGraph()
            for comp_name, comp_info in major_components.items():
                component_graph.add_node(comp_name, function_count=len(comp_info['functions']))
            
            # Add edges between components
            for func_name, func_info in self.functions.items():
                source_comp = Path(func_info['file']).stem
                for called_func in func_info['calls']:
                    if called_func in self.functions:
                        target_comp = Path(self.functions[called_func]['file']).stem
                        if source_comp != target_comp:
                            component_graph.add_edge(source_comp, target_comp)
            
            # Create visualization
            plt.figure(figsize=(15, 10))
            pos = nx.spring_layout(component_graph, k=3, iterations=50)
            
            # Draw nodes with size based on function count
            node_sizes = [major_components[node]['functions'].__len__() * 100 
                         for node in component_graph.nodes()]
            nx.draw_networkx_nodes(component_graph, pos, node_size=node_sizes,
                                 node_color='lightblue', alpha=0.7)
            
            # Draw edges
            nx.draw_networkx_edges(component_graph, pos, alpha=0.5, 
                                 edge_color='gray', arrows=True, arrowsize=20)
            
            # Draw labels
            nx.draw_networkx_labels(component_graph, pos, font_size=8, font_weight='bold')
            
            plt.title("ClanBomber System Architecture Map", fontsize=16, fontweight='bold')
            plt.axis('off')
            plt.tight_layout()
            plt.savefig('system_architecture_map.png', dpi=300, bbox_inches='tight')
            print("📊 System architecture map saved as 'system_architecture_map.png'")
            
        except Exception as e:
            print(f"Error creating system map: {e}")
    
    def run_analysis(self):
        """Run complete analysis"""
        print("Starting ClanBomber legacy code analysis...")
        
        self.find_source_files()
        
        print("Analyzing source files...")
        for i, filepath in enumerate(self.source_files):
            if i % 10 == 0:
                print(f"  Progress: {i}/{len(self.source_files)} files")
            self.analyze_file(filepath)
        
        print("Generating reports...")
        self.generate_reports()
        
        print("Creating system map...")
        self.create_system_map()
        
        print("Saving results...")
        self.save_results()
        
        print("\n✅ Analysis complete!")


def main():
    if len(sys.argv) > 1:
        source_dir = sys.argv[1]
    else:
        source_dir = "./src"
    
    analyzer = LegacyCodeAnalyzer(source_dir)
    analyzer.run_analysis()


if __name__ == "__main__":
    main()
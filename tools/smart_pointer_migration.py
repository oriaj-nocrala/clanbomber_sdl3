#!/usr/bin/env python3
"""
Smart Pointer Migration Script for ClanBomber

This script systematically converts common raw pointer patterns to smart pointer patterns.
CRITICAL: Run this on files that are already partially converted to avoid conflicts.

Author: Claude Code Assistant
"""

import os
import re
import sys
from pathlib import Path

class SmartPointerMigrator:
    def __init__(self, src_dir):
        self.src_dir = Path(src_dir)
        self.changes_made = []
        
    def log_change(self, file_path, pattern, description):
        self.changes_made.append(f"{file_path}: {description}")
        print(f"✓ {file_path}: {description}")
    
    def fix_static_cast_with_smart_pointers(self, content, file_path):
        """Fix static_cast patterns that need .get() for smart pointers"""
        changes = 0
        
        # Pattern: static_cast<Type*>(obj) where obj is from smart pointer collection iteration
        # Look for patterns in for loops over smart pointer collections
        
        # Pattern 1: for (auto& obj : collection) with static_cast<Type*>(obj)
        pattern = r'(for\s*\([^)]*auto[^)]*&\s+obj\s*:[^)]*get_object_lists\(\)[^}]*?)\bstatic_cast<(\w+\*?)>\(obj\)'
        def replace_cast_in_smart_ptr_loop(match):
            nonlocal changes
            changes += 1
            return match.group(1) + f'static_cast<{match.group(2)}>(obj.get())'
        
        content = re.sub(pattern, replace_cast_in_smart_ptr_loop, content, flags=re.DOTALL)
        
        # Pattern 2: Lambda parameters expecting raw pointers but getting smart pointers
        # [](const GameObject* obj) should be [](const std::unique_ptr<GameObject>& obj)
        pattern = r'\[\]?\([^)]*const\s+(\w+)\*\s+obj\)'
        def replace_lambda_param(match):
            nonlocal changes
            changes += 1
            return f'[](const std::unique_ptr<{match.group(1)}>& obj)'
        
        content = re.sub(pattern, replace_lambda_param, content)
        
        if changes > 0:
            self.log_change(file_path, "static_cast_fixes", f"Fixed {changes} static_cast and lambda patterns")
        
        return content
    
    def fix_function_signatures_expecting_raw_pointers(self, content, file_path):
        """Fix function signatures that expect raw pointers from smart pointer collections"""
        changes = 0
        
        # Pattern: should_skip_object_update(GameObject* obj) called with smart pointer
        pattern = r'should_skip_object_update\(obj\)'
        if 'should_skip_object_update(obj)' in content:
            content = content.replace('should_skip_object_update(obj)', 'should_skip_object_update(obj.get())')
            changes += 1
        
        # Pattern: push_back(obj) where obj is smart pointer but vector expects raw pointer
        pattern = r'draw_list\.push_back\(obj\)'
        if 'draw_list.push_back(obj)' in content:
            content = content.replace('draw_list.push_back(obj)', 'draw_list.push_back(obj.get())')
            changes += 1
        
        if changes > 0:
            self.log_change(file_path, "function_call_fixes", f"Fixed {changes} function call patterns")
        
        return content
    
    def fix_count_if_and_algorithms(self, content, file_path):
        """Fix std::count_if and algorithm functions with smart pointers"""
        changes = 0
        
        # Pattern: count_if with lambda expecting GameObject* but getting unique_ptr<GameObject>
        pattern = r'(\[]\([^)]*const\s+GameObject\*\s+obj\)[^}]*\{[^}]*\})'
        def replace_count_if_lambda(match):
            nonlocal changes
            changes += 1
            lambda_body = match.group(1)
            # Replace the parameter type and add .get() to access
            lambda_body = lambda_body.replace('const GameObject* obj', 'const std::unique_ptr<GameObject>& obj')
            lambda_body = re.sub(r'\bobj\b(?!\.)', 'obj.get()', lambda_body)
            return lambda_body
        
        new_content = re.sub(pattern, replace_count_if_lambda, content)
        if new_content != content:
            content = new_content
            changes += 1
        
        if changes > 0:
            self.log_change(file_path, "algorithm_fixes", f"Fixed {changes} std::algorithm patterns")
        
        return content
    
    def add_missing_includes(self, content, file_path):
        """Add missing #include <memory> for smart pointers"""
        changes = 0
        
        # Check if memory is already included
        if '#include <memory>' not in content and 'std::unique_ptr' in content:
            # Find the include section
            pattern = r'(#include <[^>]+>\n)(?=#include <[^>]+>|\n)'
            matches = list(re.finditer(pattern, content))
            if matches:
                # Add after the last system include
                last_include = matches[-1]
                insertion_point = last_include.end()
                content = content[:insertion_point] + '#include <memory>\n' + content[insertion_point:]
                changes += 1
        
        if changes > 0:
            self.log_change(file_path, "include_fixes", f"Added missing #include <memory>")
        
        return content
    
    def process_file(self, file_path):
        """Process a single C++ file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                original_content = f.read()
            
            content = original_content
            
            # Apply fixes in order
            content = self.add_missing_includes(content, file_path)
            content = self.fix_static_cast_with_smart_pointers(content, file_path)
            content = self.fix_function_signatures_expecting_raw_pointers(content, file_path)
            content = self.fix_count_if_and_algorithms(content, file_path)
            
            # Only write if changes were made
            if content != original_content:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Error processing {file_path}: {e}")
            return False
    
    def run_on_files(self, file_list):
        """Run migration on specific files"""
        modified_files = 0
        
        for file_path in file_list:
            full_path = self.src_dir / file_path
            if full_path.exists() and full_path.suffix in ['.cpp', '.h']:
                if self.process_file(full_path):
                    modified_files += 1
            else:
                print(f"⚠️  File not found or not C++: {full_path}")
        
        print(f"\n📊 SUMMARY: Modified {modified_files} files")
        if self.changes_made:
            print("\n📝 Changes made:")
            for change in self.changes_made[:10]:  # Show first 10 changes
                print(f"  - {change}")
            if len(self.changes_made) > 10:
                print(f"  ... and {len(self.changes_made) - 10} more")


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 smart_pointer_migration.py <src_directory>")
        sys.exit(1)
    
    src_dir = sys.argv[1]
    if not os.path.exists(src_dir):
        print(f"❌ Source directory does not exist: {src_dir}")
        sys.exit(1)
    
    print("🔧 Smart Pointer Migration Script")
    print("=" * 40)
    
    migrator = SmartPointerMigrator(src_dir)
    
    # Files that need smart pointer fixes based on compilation errors
    target_files = [
        'GameLogic.cpp',
        'GameLogic.h',
        # Add more files as needed from compilation errors
    ]
    
    print(f"🎯 Targeting {len(target_files)} files for smart pointer migration")
    migrator.run_on_files(target_files)
    
    print("\n✅ Migration completed!")
    print("⚠️  IMPORTANT: Run 'make -j4' to check for compilation errors")
    print("⚠️  IMPORTANT: Review changes with 'git diff' before committing")

if __name__ == "__main__":
    main()
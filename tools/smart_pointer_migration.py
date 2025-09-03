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
        if '#include <memory>' not in content and ('std::unique_ptr' in content or 'std::make_unique' in content):
            # Find include section - look for existing includes
            include_pattern = r'#include\s*[<"][^>"]+[>"]'
            includes = list(re.finditer(include_pattern, content))
            
            if includes:
                # Find the best insertion point (after system includes, before local includes)
                insertion_point = 0
                for include in includes:
                    if '<' in include.group() and '>' in include.group():  # System include
                        insertion_point = include.end()
                
                if insertion_point > 0:
                    # Find end of line
                    while insertion_point < len(content) and content[insertion_point] != '\n':
                        insertion_point += 1
                    if insertion_point < len(content):
                        insertion_point += 1  # Include the newline
                    
                    content = content[:insertion_point] + '#include <memory>\n' + content[insertion_point:]
                    changes += 1
        
        if changes > 0:
            self.log_change(file_path, "include_fixes", f"Added missing #include <memory>")
        
        return content
    
    def fix_forward_declarations_and_includes(self, content, file_path):
        """Fix incomplete type issues by adding proper includes"""
        changes = 0
        
        # Critical fix: Files that use std::unique_ptr<T> MUST include the complete definition
        incomplete_type_fixes = [
            ('std::unique_ptr<GameObject>', '#include "GameObject.h"'),
            ('std::unique_ptr<Bomber>', '#include "Bomber.h"'),
            ('std::list<std::unique_ptr<GameObject>>', '#include "GameObject.h"'),
            ('std::list<std::unique_ptr<Bomber>>', '#include "Bomber.h"'),
        ]
        
        for usage_pattern, include_needed in incomplete_type_fixes:
            if usage_pattern in content and include_needed not in content:
                # Find the best insertion point after other includes
                include_pattern = r'(#include\s*[<"][^>"]+[>"][^\n]*\n)'
                matches = list(re.finditer(include_pattern, content))
                
                if matches:
                    # Insert after the last include
                    insertion_point = matches[-1].end()
                    content = content[:insertion_point] + include_needed + '\n' + content[insertion_point:]
                    changes += 1
                    self.log_change(file_path, f"incomplete_type_fix", f"Added {include_needed} for {usage_pattern}")
        
        # Special case: Files that include ClanBomber.h (which has smart pointer collections)
        # need GameObject.h for complete type
        if ('#include "ClanBomber.h"' in content and 
            '#include "GameObject.h"' not in content and
            file_path.suffix == '.cpp'):  # Only for .cpp files to avoid circular includes
            
            # Find ClanBomber.h include and add GameObject.h after it
            pattern = r'(#include "ClanBomber\.h"[^\n]*\n)'
            match = re.search(pattern, content)
            if match:
                insertion_point = match.end()
                content = content[:insertion_point] + '#include "GameObject.h"\n' + content[insertion_point:]
                changes += 1
                self.log_change(file_path, "clanbomber_include_fix", "Added GameObject.h after ClanBomber.h")
        
        return content
    
    def fix_object_creation_patterns(self, content, file_path):
        """Convert raw 'new' object creation to GameContext::create_and_register_object"""
        changes = 0
        
        # Pattern: Type* obj = new Type(...); context->register_object(obj);
        pattern = r'(\w+)\*\s+(\w+)\s*=\s*new\s+\1\s*\([^)]*\)\s*;\s*[^;]*->register_object\(\2\);'
        
        def replace_creation_pattern(match):
            nonlocal changes
            changes += 1
            type_name = match.group(1)
            var_name = match.group(2)
            return f'{type_name}* {var_name} = context->create_and_register_object<{type_name}>(...);  // TODO: Fix constructor args'
        
        new_content = re.sub(pattern, replace_creation_pattern, content, flags=re.MULTILINE | re.DOTALL)
        if new_content != content:
            content = new_content
            
        if changes > 0:
            self.log_change(file_path, "object_creation", f"Converted {changes} object creation patterns to smart pointers")
        
        return content
    
    def fix_push_back_with_smart_pointers(self, content, file_path):
        """Fix push_back calls that need .get() for smart pointers"""
        changes = 0
        
        # Pattern: vector.push_back(smart_ptr_obj) -> vector.push_back(smart_ptr_obj.get())
        patterns_to_fix = [
            (r'\.push_back\((\w+)\)', r'.push_back(\1.get())'),
            (r'\.emplace_back\((\w+)\)', r'.emplace_back(\1.get())'),
        ]
        
        for pattern, replacement in patterns_to_fix:
            # Only apply if the context suggests it's a smart pointer
            matches = re.findall(pattern, content)
            for match in matches:
                var_name = match if isinstance(match, str) else match[0]
                # Look for evidence this is a smart pointer (auto& in for loop, etc.)
                if f'auto& {var_name} :' in content or f'std::unique_ptr' in content:
                    old_content = content
                    content = re.sub(pattern, replacement, content, count=1)
                    if content != old_content:
                        changes += 1
        
        if changes > 0:
            self.log_change(file_path, "container_operations", f"Fixed {changes} container push_back patterns")
        
        return content
    
    def process_file(self, file_path):
        """Process a single C++ file"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                original_content = f.read()
            
            content = original_content
            
            # Apply fixes in order (includes first to avoid compilation errors)
            content = self.add_missing_includes(content, file_path)
            content = self.fix_forward_declarations_and_includes(content, file_path)
            content = self.fix_static_cast_with_smart_pointers(content, file_path)
            content = self.fix_function_signatures_expecting_raw_pointers(content, file_path)
            content = self.fix_push_back_with_smart_pointers(content, file_path)
            content = self.fix_count_if_and_algorithms(content, file_path)
            content = self.fix_object_creation_patterns(content, file_path)
            
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
        # Files with CRITICAL incomplete type compilation errors (PRIORITY)
        'MainMenuScreen.cpp',
        'MainMenuScreen.h',
        'GameplayScreen.cpp',
        'GameSystems.cpp',
        
        # Core files already partially converted
        'GameLogic.cpp',
        'GameLogic.h',
        'TileEntity.cpp',
        'TileEntity.h',
        'Map.cpp',
        'Map.h',
        'BomberCorpse.cpp',
        'BomberCorpse.h',
        'Bomb.cpp',
        'Bomb.h',
        'ThrownBomb.cpp',
        'ThrownBomb.h',
        
        # Files that create objects and need conversion
        'MapTile.cpp',
        'LifecycleManager.cpp',
        'AudioMixer.cpp',
        
        # AI and game systems
        'Controller_AI_Modern.cpp',
        
        # Any other files that show up in compilation errors
    ]
    
    print(f"🎯 Targeting {len(target_files)} files for smart pointer migration")
    migrator.run_on_files(target_files)
    
    print("\n✅ Migration completed!")
    print("⚠️  IMPORTANT: Run 'make -j4' to check for compilation errors")
    print("⚠️  IMPORTANT: Review changes with 'git diff' before committing")

if __name__ == "__main__":
    main()
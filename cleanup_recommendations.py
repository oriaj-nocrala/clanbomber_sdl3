#!/usr/bin/env python3
"""
ClanBomber Legacy System Cleanup Recommendations
Generate specific cleanup recommendations based on analysis results.
"""

import json
from pathlib import Path
import re

class CleanupRecommendationsGenerator:
    def __init__(self):
        self.analysis_data = {}
        self.src_path = Path("src")
        
    def load_analysis(self, analysis_file="legacy_analysis.json"):
        """Load analysis results"""
        with open(analysis_file, 'r') as f:
            self.analysis_data = json.load(f)
    
    def analyze_magic_numbers(self):
        """Analyze magic numbers and suggest constants"""
        magic_numbers = self.analysis_data.get('magic_numbers', [])
        
        # Group by value and context
        number_groups = {}
        for magic in magic_numbers:
            value = magic['value']
            if value not in number_groups:
                number_groups[value] = []
            number_groups[value].append(magic)
        
        recommendations = []
        
        # Common patterns that should be constants
        common_patterns = {
            255: "COLOR_MAX or ALPHA_MAX",
            4: "PLAYER_COUNT or DIRECTION_COUNT", 
            3: "RGB_COMPONENTS or MAX_POWER_LEVEL",
            8: "TILE_SIZE or ANIMATION_FRAMES",
            20: "DEFAULT_SPEED or TIMER_INTERVAL",
            150: "MENU_WIDTH or UI_ELEMENT_WIDTH",
            200: "MENU_HEIGHT or UI_ELEMENT_HEIGHT"
        }
        
        for value, occurrences in number_groups.items():
            if len(occurrences) >= 5:  # Appears 5+ times
                suggestion = common_patterns.get(value, f"CONSTANT_VALUE_{value}")
                recommendations.append({
                    'type': 'magic_number_to_constant',
                    'value': value,
                    'occurrences': len(occurrences),
                    'suggested_constant': suggestion,
                    'locations': [f"{loc['file']}:{loc['line']}" for loc in occurrences[:5]],
                    'priority': 'HIGH' if len(occurrences) >= 10 else 'MEDIUM'
                })
        
        return recommendations
    
    def analyze_unused_functions(self):
        """Analyze unused functions for removal"""
        unused_functions = self.analysis_data.get('unused_functions', [])
        functions = self.analysis_data.get('functions', {})
        
        recommendations = []
        
        # Categorize unused functions
        categories = {
            'timer_functions': ['init', 'tick', 'time_elapsed'],
            'ui_functions': ['handle_events', 'update', 'render', 'get_next_state'],
            'audio_functions': ['shutdown', 'load_sound', 'play_sound', 'audio_callback'],
            'particle_functions': ['update_particles', 'create_particle', 'emit_'],
            'system_functions': ['update_all_', 'render_all_', 'init_all_'],
            'debug_functions': ['log_', 'get_error_', 'clear_error_']
        }
        
        for func_name in unused_functions:
            if func_name in functions:
                func_info = functions[func_name]
                category = 'misc'
                
                # Categorize function
                for cat_name, patterns in categories.items():
                    if any(pattern in func_name for pattern in patterns):
                        category = cat_name
                        break
                
                # Determine priority based on category and file
                priority = 'LOW'
                if 'test' in func_info.get('file', '').lower():
                    priority = 'HIGH'  # Test functions can be removed easily
                elif category in ['debug_functions', 'timer_functions']:
                    priority = 'HIGH'  # Likely safe to remove
                elif category in ['ui_functions', 'system_functions']:
                    priority = 'MEDIUM'  # May be part of interface
                
                recommendations.append({
                    'type': 'remove_unused_function',
                    'function': func_name,
                    'file': func_info.get('file'),
                    'line': func_info.get('line'),
                    'category': category,
                    'priority': priority,
                    'signature': func_info.get('signature', ''),
                    'reason': f"Never called ({category})"
                })
        
        return recommendations
    
    def analyze_dead_code_patterns(self):
        """Analyze dead code patterns"""
        recommendations = []
        
        # Check for commented-out code blocks
        for src_file in self.src_path.glob("*.cpp"):
            try:
                with open(src_file, 'r') as f:
                    content = f.read()
                    lines = content.split('\n')
                
                # Find blocks of commented code
                comment_blocks = []
                current_block = []
                in_comment_block = False
                
                for i, line in enumerate(lines):
                    stripped = line.strip()
                    if stripped.startswith('//') and not stripped.startswith('///'):
                        # Check if it looks like commented code
                        code_indicators = ['{', '}', ';', '=', '()', 'if ', 'for ', 'while ']
                        if any(indicator in stripped for indicator in code_indicators):
                            if not in_comment_block:
                                current_block = []
                                in_comment_block = True
                            current_block.append((i+1, line))
                    else:
                        if in_comment_block and len(current_block) >= 3:
                            comment_blocks.append(current_block)
                        current_block = []
                        in_comment_block = False
                
                # Add final block if exists
                if in_comment_block and len(current_block) >= 3:
                    comment_blocks.append(current_block)
                
                for block in comment_blocks:
                    recommendations.append({
                        'type': 'remove_commented_code',
                        'file': str(src_file),
                        'start_line': block[0][0],
                        'end_line': block[-1][0],
                        'lines_count': len(block),
                        'priority': 'MEDIUM',
                        'preview': block[0][1][:50] + '...'
                    })
            
            except Exception as e:
                print(f"Error analyzing {src_file}: {e}")
        
        return recommendations
    
    def analyze_enum_usage(self):
        """Analyze enum vs magic number usage"""
        enums = self.analysis_data.get('enums', {})
        magic_numbers = self.analysis_data.get('magic_numbers', [])
        
        recommendations = []
        
        # Look for magic numbers that could be enum values
        direction_numbers = [0, 1, 2, 3]  # Common direction values
        state_numbers = [0, 1, 2, 3, 4, 5]  # Common state values
        
        for magic in magic_numbers:
            value = magic['value']
            file_path = magic['file']
            
            # Check if this looks like a direction or state
            if value in direction_numbers:
                recommendations.append({
                    'type': 'magic_to_enum',
                    'value': value,
                    'file': file_path,
                    'line': magic['line'],
                    'suggested_enum': 'Direction',
                    'enum_value': ['DIR_DOWN', 'DIR_LEFT', 'DIR_UP', 'DIR_RIGHT'][value],
                    'priority': 'MEDIUM'
                })
            elif value in state_numbers and 'state' in file_path.lower():
                recommendations.append({
                    'type': 'magic_to_enum',
                    'value': value,
                    'file': file_path,
                    'line': magic['line'],
                    'suggested_enum': 'GameState',
                    'priority': 'MEDIUM'
                })
        
        return recommendations
    
    def find_duplicate_code(self):
        """Find potential duplicate code patterns"""
        recommendations = []
        
        # Simple duplicate detection by looking for similar function names
        functions = self.analysis_data.get('functions', {})
        function_patterns = {}
        
        for func_name, func_info in functions.items():
            # Extract base pattern (remove numbers, common suffixes)
            base_pattern = re.sub(r'_\d+$', '', func_name)  # Remove _1, _2 etc
            base_pattern = re.sub(r'(Old|New|Alt|Backup)$', '', base_pattern)
            
            if base_pattern not in function_patterns:
                function_patterns[base_pattern] = []
            function_patterns[base_pattern].append((func_name, func_info))
        
        for pattern, functions_list in function_patterns.items():
            if len(functions_list) > 1 and len(pattern) > 3:
                recommendations.append({
                    'type': 'potential_duplicate_functions',
                    'pattern': pattern,
                    'functions': [f[0] for f in functions_list],
                    'files': list(set(f[1].get('file', '') for f in functions_list)),
                    'priority': 'MEDIUM',
                    'action': 'Review and consolidate similar functions'
                })
        
        return recommendations
    
    def generate_comprehensive_report(self):
        """Generate comprehensive cleanup report"""
        print("=== CLANBOMBER LEGACY CLEANUP RECOMMENDATIONS ===\n")
        
        # Magic numbers analysis
        magic_recs = self.analyze_magic_numbers()
        print(f"🔢 MAGIC NUMBERS TO CONSTANTS ({len(magic_recs)}):")
        for rec in sorted(magic_recs, key=lambda x: x['occurrences'], reverse=True)[:10]:
            print(f"  [{rec['priority']}] {rec['value']} → {rec['suggested_constant']}")
            print(f"      Appears {rec['occurrences']} times")
            print(f"      Locations: {', '.join(rec['locations'][:3])}")
            print()
        
        # Unused functions analysis
        unused_recs = self.analyze_unused_functions()
        high_priority_unused = [r for r in unused_recs if r['priority'] == 'HIGH']
        print(f"🗑️  UNUSED FUNCTIONS TO REMOVE ({len(high_priority_unused)} high priority):")
        for rec in high_priority_unused[:10]:
            print(f"  [{rec['priority']}] {rec['function']} ({rec['category']})")
            print(f"      {rec['file']}:{rec['line']} - {rec['reason']}")
            print()
        
        # Dead code patterns
        dead_code_recs = self.analyze_dead_code_patterns()
        print(f"☠️  COMMENTED-OUT CODE BLOCKS ({len(dead_code_recs)}):")
        for rec in dead_code_recs[:5]:
            print(f"  [{rec['priority']}] {rec['file']} lines {rec['start_line']}-{rec['end_line']}")
            print(f"      {rec['lines_count']} lines of commented code")
            print(f"      Preview: {rec['preview']}")
            print()
        
        # Enum usage
        enum_recs = self.analyze_enum_usage()
        print(f"🏷️  MAGIC NUMBERS TO ENUMS ({len(enum_recs)}):")
        for rec in enum_recs[:5]:
            print(f"  [{rec['priority']}] {rec['file']}:{rec['line']}")
            print(f"      {rec['value']} → {rec['suggested_enum']}::{rec.get('enum_value', 'VALUE')}")
            print()
        
        # Duplicate code
        duplicate_recs = self.find_duplicate_code()
        print(f"👥 POTENTIAL DUPLICATE FUNCTIONS ({len(duplicate_recs)}):")
        for rec in duplicate_recs[:5]:
            print(f"  [{rec['priority']}] Pattern: {rec['pattern']}")
            print(f"      Functions: {', '.join(rec['functions'])}")
            print(f"      Files: {', '.join(rec['files'])}")
            print()
        
        # Summary
        total_issues = len(magic_recs) + len(unused_recs) + len(dead_code_recs) + len(enum_recs) + len(duplicate_recs)
        high_priority = len([r for r in magic_recs if r['priority'] == 'HIGH']) + len(high_priority_unused)
        
        print(f"📊 CLEANUP SUMMARY:")
        print(f"  Total issues found: {total_issues}")
        print(f"  High priority: {high_priority}")
        print(f"  Estimated cleanup time: {high_priority * 15 + (total_issues - high_priority) * 5} minutes")
        
        return {
            'magic_numbers': magic_recs,
            'unused_functions': unused_recs,
            'dead_code': dead_code_recs,
            'enum_suggestions': enum_recs,
            'duplicate_code': duplicate_recs,
            'summary': {
                'total_issues': total_issues,
                'high_priority': high_priority
            }
        }
    
    def save_cleanup_plan(self, recommendations, output_file="cleanup_plan.json"):
        """Save cleanup recommendations to file"""
        with open(output_file, 'w') as f:
            json.dump(recommendations, f, indent=2, default=str)
        print(f"\n💾 Cleanup plan saved to {output_file}")


def main():
    generator = CleanupRecommendationsGenerator()
    generator.load_analysis()
    
    recommendations = generator.generate_comprehensive_report()
    generator.save_cleanup_plan(recommendations)
    
    print("\n✨ Ready to start cleanup! Use the generated plan as a guide.")


if __name__ == "__main__":
    main()
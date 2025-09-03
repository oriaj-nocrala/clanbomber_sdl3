"""
Data models for the ClanBomber Refactor Tool
"""

from dataclasses import dataclass, field, asdict
from typing import Dict, List, Set, Optional, Any
from enum import Enum
import json

class FunctionStatus(Enum):
    """Status of function usage verification"""
    USED = "used"
    UNUSED = "unused"
    UNCERTAIN = "uncertain"
    NEEDS_MANUAL_CHECK = "needs_manual_check"

class AnalysisType(Enum):
    """Types of analysis that can be performed"""
    FUNCTIONS = "functions"
    MAGIC_NUMBERS = "magic_numbers"
    COMMENTED_CODE = "commented_code"
    DUPLICATES = "duplicates"
    DEPENDENCIES = "dependencies"

class Severity(Enum):
    """Severity levels for findings"""
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"
    CRITICAL = "critical"

@dataclass
class SourceLocation:
    """Represents a location in source code"""
    file: str
    line: int
    column: int = 0
    
    def __str__(self) -> str:
        return f"{self.file}:{self.line}"
    
    @property
    def filename(self) -> str:
        """Get just the filename without path"""
        from pathlib import Path
        return Path(self.file).name

@dataclass
class FunctionInfo:
    """Information about a function"""
    name: str
    location: SourceLocation
    signature: str
    class_name: Optional[str] = None
    is_static: bool = False
    is_virtual: bool = False
    is_constructor: bool = False
    is_destructor: bool = False
    is_inline: bool = False
    status: FunctionStatus = FunctionStatus.UNCERTAIN
    confidence: float = 0.0
    evidence: List[str] = field(default_factory=list)
    calls_to: Set[str] = field(default_factory=set)
    called_from: Set[str] = field(default_factory=set)

@dataclass
class FunctionCall:
    """Represents a function call"""
    caller_function: Optional[str]
    called_function: str
    location: SourceLocation
    context: str

@dataclass
class MagicNumber:
    """Information about a magic number"""
    value: int
    location: SourceLocation
    context: str
    suggested_constant: Optional[str] = None
    category: Optional[str] = None  # ui, game_logic, graphics, etc.
    usage_count: int = 1

@dataclass
class CommentedCodeBlock:
    """Represents a block of commented-out code"""
    location: SourceLocation
    end_line: int
    lines: List[str]
    confidence: float
    suggested_action: str = "remove"
    
    @property
    def line_count(self) -> int:
        return len(self.lines)
    
    @property
    def preview(self) -> str:
        if self.lines:
            preview = self.lines[0][:60]
            return preview + "..." if len(self.lines[0]) > 60 else preview
        return ""

@dataclass
class Finding:
    """Represents an analysis finding"""
    id: str
    type: AnalysisType
    severity: Severity
    title: str
    description: str
    location: SourceLocation
    suggestions: List[str] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization"""
        return {
            'id': self.id,
            'type': self.type.value,
            'severity': self.severity.value,
            'title': self.title,
            'description': self.description,
            'location': {
                'file': self.location.file,
                'line': self.location.line,
                'column': self.location.column
            },
            'suggestions': self.suggestions,
            'metadata': self.metadata
        }

@dataclass
class AnalysisResult:
    """Complete analysis results"""
    project_name: str
    source_files: List[str]
    functions: Dict[str, FunctionInfo] = field(default_factory=dict)
    function_calls: List[FunctionCall] = field(default_factory=list)
    magic_numbers: List[MagicNumber] = field(default_factory=list)
    commented_blocks: List[CommentedCodeBlock] = field(default_factory=list)
    findings: List[Finding] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def get_findings_by_type(self, finding_type: AnalysisType) -> List[Finding]:
        """Get findings filtered by type"""
        return [f for f in self.findings if f.type == finding_type]
    
    def get_findings_by_severity(self, severity: Severity) -> List[Finding]:
        """Get findings filtered by severity"""
        return [f for f in self.findings if f.severity == severity]
    
    def get_unused_functions(self) -> List[FunctionInfo]:
        """Get functions marked as unused"""
        return [f for f in self.functions.values() if f.status == FunctionStatus.UNUSED]
    
    def get_high_confidence_unused(self, threshold: float = 0.8) -> List[FunctionInfo]:
        """Get unused functions with high confidence"""
        return [f for f in self.get_unused_functions() if f.confidence >= threshold]
    
    def save_to_json(self, filepath: str):
        """Save results to JSON file"""
        def serialize_item(obj):
            if isinstance(obj, (set, frozenset)):
                return list(obj)
            elif isinstance(obj, Enum):
                return obj.value
            elif hasattr(obj, '__dataclass_fields__'):  # Check if it's a dataclass
                return asdict(obj)
            elif hasattr(obj, '__dict__'):
                return obj.__dict__
            return str(obj)
        
        data = {
            'metadata': {
                **self.metadata,
                'project_name': self.project_name,
                'source_files_count': len(self.source_files),
                'functions_count': len(self.functions),
                'findings_count': len(self.findings)
            },
            'functions': {k: asdict(v) for k, v in self.functions.items()},
            'function_calls': [asdict(call) for call in self.function_calls],
            'magic_numbers': [asdict(num) for num in self.magic_numbers],
            'commented_blocks': [asdict(block) for block in self.commented_blocks],
            'findings': [f.to_dict() for f in self.findings],
            'source_files': self.source_files
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2, default=serialize_item)
    
    @classmethod
    def load_from_json(cls, filepath: str) -> 'AnalysisResult':
        """Load results from JSON file"""
        with open(filepath, 'r') as f:
            data = json.load(f)
        
        # This would need proper deserialization logic
        # For now, return a basic instance
        result = cls(
            project_name=data['metadata'].get('project_name', 'Unknown'),
            source_files=data.get('source_files', [])
        )
        result.metadata = data.get('metadata', {})
        return result
// ClanBomber Refactor Tool Dashboard JavaScript

class Dashboard {
    constructor() {
        this.severityChart = null;
        this.functionChart = null;
        this.rawPointerSeverityChart = null;
        this.rawPointerPatternsChart = null;
        this.isLoading = false;
        
        this.init();
    }
    
    init() {
        this.setupEventListeners();
        this.loadDashboard();
        
        // Auto-refresh every 5 minutes
        setInterval(() => {
            if (!this.isLoading) {
                this.loadDashboard();
            }
        }, 300000);
    }
    
    setupEventListeners() {
        document.getElementById('refresh-btn').addEventListener('click', () => {
            this.refreshAnalysis();
        });
    }
    
    showLoading(show = true) {
        const overlay = document.getElementById('loading-overlay');
        this.isLoading = show;
        
        if (show) {
            overlay.classList.add('show');
        } else {
            overlay.classList.remove('show');
        }
    }
    
    async loadDashboard() {
        try {
            this.showLoading(true);
            
            // Load overview data
            await this.loadOverview();
            
            // Load charts data
            await this.loadCharts();
            
            // Load tables data
            await this.loadTables();
            
            // Load file statistics
            await this.loadFileStats();
            
        } catch (error) {
            console.error('Error loading dashboard:', error);
            this.showError('Failed to load dashboard data');
        } finally {
            this.showLoading(false);
        }
    }
    
    async loadOverview() {
        const response = await fetch('/api/analysis/overview');
        const data = await response.json();
        
        if (data.error) {
            throw new Error(data.error);
        }
        
        // Update statistics
        const stats = data.statistics;
        document.getElementById('total-files').textContent = stats.total_files;
        document.getElementById('total-functions').textContent = stats.total_functions;
        document.getElementById('unused-functions').textContent = stats.unused_functions;
        document.getElementById('magic-numbers').textContent = stats.magic_numbers;
        document.getElementById('commented-blocks').textContent = stats.commented_blocks;
        document.getElementById('raw-pointers').textContent = stats.raw_pointers;
        document.getElementById('total-findings').textContent = stats.total_findings;
        
        // Update last updated time
        if (data.last_updated) {
            const lastUpdated = new Date(data.last_updated);
            document.getElementById('last-update').textContent = 
                `Last updated: ${lastUpdated.toLocaleString()}`;
        }
    }
    
    async loadCharts() {
        // Load findings data for charts
        const findingsResponse = await fetch('/api/analysis/findings');
        const findingsData = await findingsResponse.json();
        
        // Load functions data for function status chart
        const functionsResponse = await fetch('/api/analysis/functions');
        const functionsData = await functionsResponse.json();
        
        this.updateSeverityChart(findingsData.by_severity);
        this.updateFunctionChart(functionsData);
        
        // Load raw pointers data
        await this.loadRawPointers();
    }
    
    updateSeverityChart(severityData) {
        const ctx = document.getElementById('severityChart').getContext('2d');
        
        if (this.severityChart) {
            this.severityChart.destroy();
        }
        
        const severityColors = {
            'critical': '#dc3545',
            'high': '#fd7e14',
            'medium': '#ffc107',
            'low': '#28a745'
        };
        
        const labels = Object.keys(severityData);
        const values = Object.values(severityData);
        const colors = labels.map(label => severityColors[label] || '#6c757d');
        
        this.severityChart = new Chart(ctx, {
            type: 'doughnut',
            data: {
                labels: labels.map(l => l.charAt(0).toUpperCase() + l.slice(1)),
                datasets: [{
                    data: values,
                    backgroundColor: colors,
                    borderWidth: 2,
                    borderColor: '#fff'
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            padding: 20,
                            usePointStyle: true
                        }
                    }
                }
            }
        });
    }
    
    updateFunctionChart(functionsData) {
        const ctx = document.getElementById('functionStatusChart').getContext('2d');
        
        if (this.functionChart) {
            this.functionChart.destroy();
        }
        
        // Count function statuses
        const statusCounts = {
            'used': 0,
            'unused': 0,
            'uncertain': 0
        };
        
        functionsData.forEach(func => {
            if (statusCounts.hasOwnProperty(func.status)) {
                statusCounts[func.status]++;
            }
        });
        
        const statusColors = {
            'used': '#28a745',
            'unused': '#dc3545',
            'uncertain': '#ffc107'
        };
        
        this.functionChart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: Object.keys(statusCounts).map(s => s.charAt(0).toUpperCase() + s.slice(1)),
                datasets: [{
                    label: 'Functions',
                    data: Object.values(statusCounts),
                    backgroundColor: Object.keys(statusCounts).map(s => statusColors[s]),
                    borderWidth: 2,
                    borderColor: '#fff',
                    borderRadius: 5
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        ticks: {
                            stepSize: 1
                        }
                    }
                }
            }
        });
    }
    
    async loadTables() {
        // Load findings for table
        const findingsResponse = await fetch('/api/analysis/findings');
        const findingsData = await findingsResponse.json();
        
        // Load magic numbers
        const magicResponse = await fetch('/api/analysis/magic-numbers');
        const magicData = await magicResponse.json();
        
        this.updateFindingsTable(findingsData.by_type);
        this.updateMagicNumbersTable(magicData);
    }
    
    updateFindingsTable(findingsByType) {
        const tbody = document.getElementById('findings-tbody');
        tbody.innerHTML = '';
        
        // Flatten findings and take first 20
        const allFindings = [];
        Object.entries(findingsByType).forEach(([type, findings]) => {
            findings.forEach(finding => {
                allFindings.push({...finding, type});
            });
        });
        
        // Sort by severity (critical first)
        const severityOrder = {'critical': 0, 'high': 1, 'medium': 2, 'low': 3};
        allFindings.sort((a, b) => (severityOrder[a.severity] || 99) - (severityOrder[b.severity] || 99));
        
        allFindings.slice(0, 20).forEach(finding => {
            const row = tbody.insertRow();
            row.innerHTML = `
                <td>${finding.type}</td>
                <td><span class="severity-${finding.severity}">${finding.severity.toUpperCase()}</span></td>
                <td>${finding.title}</td>
                <td>${finding.file}</td>
                <td>${finding.line}</td>
            `;
        });
    }
    
    updateMagicNumbersTable(magicNumbers) {
        const tbody = document.getElementById('magic-tbody');
        tbody.innerHTML = '';
        
        // Show first 15 magic numbers
        magicNumbers.slice(0, 15).forEach(magic => {
            const row = tbody.insertRow();
            row.innerHTML = `
                <td><strong>${magic.value}</strong></td>
                <td>${magic.file}</td>
                <td>${magic.line}</td>
                <td>${magic.suggested_constant || '-'}</td>
                <td>${magic.category || '-'}</td>
            `;
        });
    }
    
    async loadFileStats() {
        const response = await fetch('/api/analysis/file-stats');
        const data = await response.json();
        
        const tbody = document.getElementById('file-stats-tbody');
        tbody.innerHTML = '';
        
        // Sort files by total issues (descending)
        const fileEntries = Object.entries(data).sort((a, b) => {
            const issuesA = a[1].unused_functions + a[1].magic_numbers + a[1].commented_blocks + a[1].findings;
            const issuesB = b[1].unused_functions + b[1].magic_numbers + b[1].commented_blocks + b[1].findings;
            return issuesB - issuesA;
        });
        
        fileEntries.forEach(([filename, stats]) => {
            const row = tbody.insertRow();
            row.innerHTML = `
                <td><strong>${filename}</strong></td>
                <td>${stats.functions}</td>
                <td>${stats.unused_functions > 0 ? `<span style="color: #dc3545;">${stats.unused_functions}</span>` : '0'}</td>
                <td>${stats.magic_numbers > 0 ? `<span style="color: #fd7e14;">${stats.magic_numbers}</span>` : '0'}</td>
                <td>${stats.commented_blocks > 0 ? `<span style="color: #ffc107;">${stats.commented_blocks}</span>` : '0'}</td>
                <td>${stats.findings > 0 ? `<span style="color: #dc3545;">${stats.findings}</span>` : '0'}</td>
            `;
        });
    }
    
    async refreshAnalysis() {
        try {
            this.showLoading(true);
            
            const response = await fetch('/api/analysis/refresh');
            const data = await response.json();
            
            if (data.error) {
                throw new Error(data.error);
            }
            
            // Reload all dashboard data
            await this.loadDashboard();
            
            this.showSuccess('Analysis refreshed successfully!');
            
        } catch (error) {
            console.error('Error refreshing analysis:', error);
            this.showError('Failed to refresh analysis');
        }
    }
    
    showError(message) {
        // Simple error display - could be enhanced with a proper notification system
        alert('Error: ' + message);
    }
    
    showSuccess(message) {
        // Simple success display - could be enhanced with a proper notification system
        console.log('Success:', message);
        
        // Flash the refresh button green briefly
        const btn = document.getElementById('refresh-btn');
        const originalStyle = btn.style.background;
        btn.style.background = '#28a745';
        setTimeout(() => {
            btn.style.background = originalStyle;
        }, 1000);
    }
    
    async loadRawPointers() {
        try {
            const response = await fetch('/api/analysis/raw-pointers');
            const data = await response.json();
            
            if (data.error) {
                console.error('Raw pointers data error:', data.error);
                return;
            }
            
            this.updateRawPointerCharts(data);
            this.updateDangerousPointersTable(data.dangerous_pointers);
            
        } catch (error) {
            console.error('Error loading raw pointers:', error);
        }
    }
    
    updateRawPointerCharts(data) {
        // Update severity chart
        const severityCtx = document.getElementById('rawPointerSeverityChart').getContext('2d');
        
        if (this.rawPointerSeverityChart) {
            this.rawPointerSeverityChart.destroy();
        }
        
        const severityColors = {
            'critical': '#dc3545',
            'high': '#fd7e14',
            'medium': '#ffc107',
            'low': '#28a745'
        };
        
        const severityLabels = Object.keys(data.severity_breakdown);
        const severityValues = Object.values(data.severity_breakdown);
        const severityChartColors = severityLabels.map(label => severityColors[label] || '#6c757d');
        
        this.rawPointerSeverityChart = new Chart(severityCtx, {
            type: 'doughnut',
            data: {
                labels: severityLabels.map(l => l.charAt(0).toUpperCase() + l.slice(1)),
                datasets: [{
                    data: severityValues,
                    backgroundColor: severityChartColors,
                    borderWidth: 2,
                    borderColor: '#fff'
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: {
                        position: 'bottom'
                    },
                    title: {
                        display: true,
                        text: 'Raw Pointer Risk Levels'
                    }
                }
            }
        });
        
        // Update patterns chart
        const patternsCtx = document.getElementById('rawPointerPatternsChart').getContext('2d');
        
        if (this.rawPointerPatternsChart) {
            this.rawPointerPatternsChart.destroy();
        }
        
        const patternLabels = Object.keys(data.pattern_breakdown);
        const patternValues = Object.values(data.pattern_breakdown);
        
        this.rawPointerPatternsChart = new Chart(patternsCtx, {
            type: 'bar',
            data: {
                labels: patternLabels.map(l => l.charAt(0).toUpperCase() + l.slice(1)),
                datasets: [{
                    label: 'Count',
                    data: patternValues,
                    backgroundColor: '#007bff',
                    borderColor: '#0056b3',
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                scales: {
                    y: {
                        beginAtZero: true,
                        ticks: {
                            stepSize: 1
                        }
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    },
                    title: {
                        display: true,
                        text: 'Raw Pointer Usage Patterns'
                    }
                }
            }
        });
    }
    
    updateDangerousPointersTable(pointers) {
        const tbody = document.getElementById('dangerous-pointers-tbody');
        tbody.innerHTML = '';
        
        pointers.forEach(pointer => {
            const row = tbody.insertRow();
            const severityClass = `severity-${pointer.danger_level.toLowerCase()}`;
            
            row.innerHTML = `
                <td><code>${pointer.variable_name}</code></td>
                <td><code>${pointer.type_name}</code></td>
                <td><span class="badge badge-pattern">${pointer.pattern_type}</span></td>
                <td><span class="badge ${severityClass}">${pointer.danger_level}</span></td>
                <td><code>${pointer.file}</code></td>
                <td>${pointer.line}</td>
                <td><small>${pointer.suggested_fix}</small></td>
            `;
            
            // Add click handler for row details
            row.style.cursor = 'pointer';
            row.title = pointer.context;
        });
    }
}

// Initialize dashboard when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    new Dashboard();
});
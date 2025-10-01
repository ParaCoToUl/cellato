import os
import sys
from table_printer import TablePrinter
from show_optimal_params import CSVLoader, IMPLEMENTATIONS, BITS_USED

def generate_html_table(size, automaton_groups, loader):
    title = f"CUDA Performance Comparison - {int(size**0.5)}x{int(size**0.5)} Grid"
    html = f"<h2>{title}</h2>"
    html += "<table>"
    
    # Header row
    implementations = list(IMPLEMENTATIONS.keys())
    html += "<tr>"
    html += f"<th>Automaton</th>"
    html += f"<th>Baseline (ns)</th>"
    for impl in implementations[1:]:
        html += f"<th>{impl}</th>"
    html += "</tr>"
    
    # Data rows
    for automaton, automaton_group in sorted(automaton_groups.items()):
        # Split by implementation type
        impl_groups = loader.split_by_implementation(automaton_group)
        
        # Find baseline implementation
        baseline_result = loader.find_best_implementation(impl_groups["Baseline"], IMPLEMENTATIONS["Baseline"])
        if not baseline_result:
            continue
            
        # Get baseline time
        baseline_time = baseline_result.normalized_time() * 1e9  # Convert to nanoseconds
        
        # Create row with automaton name and baseline time
        automaton_name = automaton
        if automaton in BITS_USED:
            bits_info = BITS_USED[automaton].strip().replace(TablePrinter.COLORS.YELLOW, '').replace(TablePrinter.COLORS.RESET, '')
            automaton_name = f"{automaton} {bits_info}"
        
        # Get baseline params
        baseline_params = format_params(baseline_result.values)
        
        html += "<tr>"
        html += f"<td>{automaton_name}</td>"
        html += f'<td class="tooltip">{baseline_time:.4f} ns<span class="tooltip-text">{baseline_params}</span></td>'
        
        # Add data for each implementation
        for impl in implementations[1:]:  # Skip baseline
            best_result = loader.find_best_implementation(impl_groups[impl], IMPLEMENTATIONS[impl])
            if best_result:
                time = best_result.normalized_time() * 1e9  # Convert to nanoseconds
                speedup = baseline_time / time
                
                # Format speedup with color
                speedup_class = "speedup-positive" if speedup > 1 else "speedup-negative"
                speedup_html = f'<span class="{speedup_class}">{speedup:.2f}x</span>'
                
                # Get parameters
                params = format_params(best_result.values)
                
                html += f'<td class="tooltip">{time:.4f} ns ({speedup_html})<span class="tooltip-text">{params}</span></td>'
            else:
                html += "<td>-</td>"
        
        html += "</tr>"
    
    html += "</table>"
    return html

def format_params(params_dict):
    """Format hyperparameters for tooltip display, focusing on grid search parameters"""
    result = "<strong>Optimized Hyperparameters:</strong><br>"
    
    # Check for CUDA block size y - relevant for all CUDA implementations
    if 'cuda_block_size_y' in params_dict:
        result += f"CUDA block size y: {params_dict['cuda_block_size_y']}<br>"
    
    # Check if this is a temporal implementation
    is_temporal = params_dict.get('traverser', '').startswith('temporal') or \
                 params_dict.get('traverser', '').endswith('_temporal')
    
    # Show temporal parameters only for temporal implementations
    if is_temporal:
        if 'temporal_steps' in params_dict:
            result += f"Temporal steps: {params_dict['temporal_steps']}<br>"
        if 'temporal_tile_size_y' in params_dict:
            result += f"Temporal tile y: {params_dict['temporal_tile_size_y']}<br>"
    
    # If nothing was found, add a note
    if result == "<strong>Optimized Hyperparameters:</strong><br>":
        result += "No grid search parameters available for this implementation"
        
    return result

def generate_report(csv_path, output_path):
    # Load template
    template_path = os.path.join(os.path.dirname(__file__), 'report-template.html')
    with open(template_path, 'r') as f:
        template = f.read()
    
    # Load and process data
    loader = CSVLoader(csv_path)
    size_groups = loader.get_groups_by_sizes([x ** 2 for x in [4096, 8192, 16384, 32768]], tolerance=0.1)
    sorted_groups = sorted(size_groups.items())
    
    # Generate HTML tables
    tables_html = ""
    for size, group in sorted_groups:
        if not group:
            continue
        automaton_groups = loader.split_by_automaton(group)
        tables_html += generate_html_table(size, automaton_groups, loader)
    
    # Replace placeholder with tables
    html_content = template.replace("<!-- TABLES_PLACEHOLDER -->", tables_html)
    
    # Write to file
    with open(output_path, 'w') as f:
        f.write(html_content)
    
    print(f"HTML report generated at {output_path}")

def main():
    # Get CSV file path from command line or use default
    csv_file = sys.argv[1] if len(sys.argv) > 1 else 'results/grid-search-results.csv'
    
    # Get output file path or use default
    output_file = sys.argv[2] if len(sys.argv) > 2 else 'performance_report.html'
    
    generate_report(csv_file, output_file)

if __name__ == "__main__":
    main()

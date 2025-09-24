import pandas as pd
import sys
import os
import re
from table_printer import TablePrinter

bits_used = {
    "game-of-life": f"       {TablePrinter.COLORS.YELLOW}1 bit{TablePrinter.COLORS.RESET}",
    "forest-fire": f"        {TablePrinter.COLORS.YELLOW}2 bits{TablePrinter.COLORS.RESET}",
    "wire": f"               {TablePrinter.COLORS.YELLOW}2 bits{TablePrinter.COLORS.RESET}",
    "greenberg-hastings": f" {TablePrinter.COLORS.YELLOW}3 bits{TablePrinter.COLORS.RESET}",
    "brian": f"              {TablePrinter.COLORS.YELLOW}1 bit{TablePrinter.COLORS.RESET}",
    "cyclic": f"             {TablePrinter.COLORS.YELLOW}5 bits{TablePrinter.COLORS.RESET}",
    "traffic": f"            {TablePrinter.COLORS.YELLOW}2 bits{TablePrinter.COLORS.RESET}",
    "hpp": f"                {TablePrinter.COLORS.YELLOW}4 bits{TablePrinter.COLORS.RESET}",
    "maze": f"               {TablePrinter.COLORS.YELLOW}1 bit{TablePrinter.COLORS.RESET}",
    "critters": f"           {TablePrinter.COLORS.YELLOW}1 bit{TablePrinter.COLORS.RESET}",
}

def main():
    # Read CSV data - default to testout.csv if no file specified
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "testout.csv"
    df = pd.read_csv(csv_path)
    
    # Determine implementation type for each row
    def get_impl_type(row):
        if row.get('reference_impl') == 'baseline':
            return 'baseline'
        elif row.get('evaluator') == 'standard' and row.get('layout') == 'standard':
            return 'standard'
        elif row.get('evaluator') == 'bit_array' and row.get('layout') == 'bit_array':
            return f"bit_array_{int(row.get('precision'))}"
        elif row.get('evaluator') == 'bit_planes' and row.get('layout') == 'bit_planes':
            return f"bit_planes_{int(row.get('precision'))}"
        return None
    
    df['impl_type'] = df.apply(get_impl_type, axis=1)
    
    # Detect if colors are supported
    use_colors = True
    if os.name == 'nt' or 'NO_COLOR' in os.environ:
        use_colors = False
    
    # Process data for both CPU and CUDA
    for device in ['CPU', 'CUDA']:
        device_df = df[df['device'] == device]
        
        # Create dictionary to hold results
        results = {}
        
        # For each automaton type, collect baseline and implementation times
        for automaton, group in device_df.groupby('automaton'):
            baseline_rows = group[group['impl_type'] == 'baseline']
            if baseline_rows.empty:
                continue
                
            baseline_time = baseline_rows['average_time_per_cell_ns'].iloc[0]
            
            automaton_results = {'baseline': baseline_time}
            
            # Get times for each implementation
            for impl in ['standard', 'bit_array_32', 'bit_array_64', 'bit_planes_32', 'bit_planes_64']:
                impl_rows = group[group['impl_type'] == impl]
                if not impl_rows.empty:
                    automaton_results[impl] = impl_rows['average_time_per_cell_ns'].iloc[0]
            
            results[automaton] = automaton_results
        
        # Print the table title
        title = f"{device} Implementation Comparison"
        print(f"\n{TablePrinter.COLORS.YELLOW_r}{TablePrinter.COLORS.BOLD_r}{title}{TablePrinter.COLORS.RESET_r}\n" if use_colors else f"\n{title}\n")
        
        # Create table printer
        printer = TablePrinter()
        printer.set_use_colors(use_colors)
        
        # Add header row
        header = ["Automaton", "Baseline (ns)"]
        for impl in ['standard', 'bit_array_32', 'bit_array_64', 'bit_planes_32', 'bit_planes_64']:
            header.append(impl)
        
        # Add header with colors
        colored_header = []
        for item in header:
            colored_header.append(f"{TablePrinter.COLORS.CYAN}{item}{TablePrinter.COLORS.RESET}" if use_colors else item)
        
        printer.add_row(colored_header)
        
        # Add data rows
        for automaton in sorted(results.keys()):
            data = results[automaton]
            baseline = data['baseline']

            row = [automaton + bits_used[automaton], f"{baseline:.4f} ns"]

            for impl in ['standard', 'bit_array_32', 'bit_array_64', 'bit_planes_32', 'bit_planes_64']:
                if impl in data:
                    time = data[impl]
                    speedup = baseline / time
                    
                    color = TablePrinter.COLORS.GREEN if speedup > 1 else TablePrinter.COLORS.RED
                    speedup_str = f"{color}{speedup:.2f}x{TablePrinter.COLORS.RESET}" if use_colors else f"{speedup:.2f}x"
                    
                    row.append(f"{time:.4f} ns ({speedup_str})")
                else:
                    row.append("-")
            
            printer.add_row(row)
        
        # Print the table
        printer.print()

if __name__ == "__main__":
    main()

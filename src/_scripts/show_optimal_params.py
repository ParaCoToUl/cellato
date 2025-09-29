import os
import sys
from table_printer import TablePrinter

IMPLEMENTATIONS = {
    "Baseline": {'reference_impl': 'baseline'},
    "Standard": {'traverser': 'simple', 'evaluator': 'standard', 'layout': 'standard'},
    "Bit Array (32-bit)": {'traverser': 'simple', 'evaluator': 'bit_array', 'layout': 'bit_array', 'precision': 32},
    "Bit Array (64-bit)": {'traverser': 'simple', 'evaluator': 'bit_array', 'layout': 'bit_array', 'precision': 64},
    "Bit Planes (32-bit)": {'traverser': 'simple', 'evaluator': 'bit_planes', 'layout': 'bit_planes', 'precision': 32},
    "Bit Planes (64-bit)": {'traverser': 'simple', 'evaluator': 'bit_planes', 'layout': 'bit_planes', 'precision': 64},
    "Tiled BP (32-bit)": {'traverser': 'simple', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes', 'precision': 32},
    "Tiled BP (64-bit)": {'traverser': 'simple', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes', 'precision': 64},
    "Temporal (32-bit)": {'traverser': 'tiled_temporal', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes', 'precision': 32},
    "Temporal (64-bit)": {'traverser': 'tiled_temporal', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes', 'precision': 64},
}

# These are the bits used by each automaton - for display purposes
BITS_USED = {
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

class RunResult: 
    def __init__(self, csv_header, csv_line):
        self.values = {}
        headers = csv_header.strip().split(',')
        line_values = csv_line.strip().split(',')
        for header, value in zip(headers, line_values):
            if value.replace('.', '', 1).isdigit():
                if '.' in value:
                    value = float(value)
                else:
                    value = int(value)
            self.values[header] = value

    def is_implementation(self, impl_dict):
        for key, val in impl_dict.items():
            if key not in self.values or str(self.values[key]) != str(val):
                return False
        return True
    
    def falls_roughly_to_size(self, total_elements, tolerance=0.1):
        actual_elements = self.values.get('x_size', 1) * self.values.get('y_size', 1)
        return abs(actual_elements - total_elements) <= tolerance * total_elements

    def normalized_time(self):
        average_time = self.values.get('average_time_ms')
        elem_count = self.values.get('x_size', 1) * self.values.get('y_size', 1)
        steps = self.values.get('steps', 1)

        time_per_step_per_elem = average_time / (steps * elem_count)
        return time_per_step_per_elem


class CSVLoader:
    def __init__(self, csv_path):
        self.results = []
        with open(csv_path, 'r') as f:
            header = f.readline()
            for line in f:
                if line.strip():
                    self.results.append(RunResult(header, line))

    def get_groups_by_sizes(self, sizes, tolerance=0.1):
        groups = {size: [] for size in sizes}
        for result in self.results:
            for size in sizes:
                if result.falls_roughly_to_size(size, tolerance):
                    groups[size].append(result)
                    break
        return groups
    
    def split_by_implementation(self, group):
        impl_groups = {key: [] for key in IMPLEMENTATIONS.keys()}
        for result in group:
            for impl_key, impl_dict in IMPLEMENTATIONS.items():
                if result.is_implementation(impl_dict):
                    impl_groups[impl_key].append(result)
                    break
        return impl_groups
    
    def split_by_automaton(self, group):
        automaton_groups = {}
        for result in group:
            automaton = result.values.get('automaton', 'unknown')
            if automaton not in automaton_groups:
                automaton_groups[automaton] = []
            automaton_groups[automaton].append(result)
        return automaton_groups

    def find_best_implementation(self, group, impl_dict):
        best_result = None
        best_time = float('inf')
        for result in group:
            if result.is_implementation(impl_dict):
                norm_time = result.normalized_time()
                if norm_time < best_time:
                    best_time = norm_time
                    best_result = result
        return best_result


def main():
    # Get CSV file path from command line or use default
    file = sys.argv[1] if len(sys.argv) > 1 else 'results/cuda_test_20250928-153828.csv'

    # Load and process data
    loader = CSVLoader(file)
    size_groups = loader.get_groups_by_sizes([x ** 2 for x in [4096, 8192, 16384, 32768]], tolerance=0.1)
    sorted_groups = sorted(size_groups.items())

    # Detect if colors are supported
    use_colors = True
    if os.name == 'nt' or 'NO_COLOR' in os.environ:
        use_colors = False
    
    # Define table columns (implementations to show)
    implementations = list(IMPLEMENTATIONS.keys())
    
    # Create one table per size group
    for size, group in sorted_groups:
        # Skip if no data for this size
        if not group:
            continue
            
        # Create table title
        title = f"CUDA Performance Comparison - {int(size**0.5)}×{int(size**0.5)} Grid"
        print(f"\n{TablePrinter.COLORS.YELLOW_r}{TablePrinter.COLORS.BOLD_r}{title}{TablePrinter.COLORS.RESET_r}\n" if use_colors else f"\n{title}\n")
        
        # Create table printer
        printer = TablePrinter()
        printer.set_use_colors(use_colors)
        
        # Create header row
        header = ["Automaton", "Baseline (ns)"]
        for impl in implementations[1:]:  # Skip baseline as it's already in the header
            header.append(impl)
            
        # Add colored header
        colored_header = []
        for item in header:
            colored_header.append(f"{TablePrinter.COLORS.CYAN}{item}{TablePrinter.COLORS.RESET}" if use_colors else item)
        
        printer.add_row(colored_header)
        
        # Process data by automaton
        automaton_groups = loader.split_by_automaton(group)
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
                automaton_name += BITS_USED[automaton]
                
            row = [automaton_name, f"{baseline_time:.4f} ns"]
            
            # Add data for each implementation
            for impl in implementations[1:]:  # Skip baseline
                best_result = loader.find_best_implementation(impl_groups[impl], IMPLEMENTATIONS[impl])
                if best_result:
                    time = best_result.normalized_time() * 1e9  # Convert to nanoseconds
                    speedup = baseline_time / time
                    
                    # Format speedup with color
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



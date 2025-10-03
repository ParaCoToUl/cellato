import pandas as pd
import argparse
import sys

def fix_time_per_cell(filepath: str):
    """
    Reads a CSV file, recomputes the 'average_time_per_cell_ps' column,
    and overwrites the file with the corrected data.
    
    Args:
        filepath: The path to the CSV file to process.
    """
    try:
        print(f"Reading data from '{filepath}'...")
        # Read the CSV file into a pandas DataFrame
        df = pd.read_csv(filepath)

        # List of columns required for the calculation
        required_cols = ['average_time_ms', 'x_size', 'y_size', 'steps']

        # Verify that all necessary columns exist in the file
        if not all(col in df.columns for col in required_cols):
            missing = [col for col in required_cols if col not in df.columns]
            print(f"Error: Missing required columns: {', '.join(missing)}", file=sys.stderr)
            sys.exit(1)

        # --- The Calculation ---
        # The formula calculates the time in picoseconds per cell update.
        # 1. Convert average_time_ms to picoseconds: average_time_ms * 1e9
        # 2. Calculate total cell updates: x_size * y_size * steps
        # 3. Divide the total time in picoseconds by the total cell updates.
        print("Recomputing 'average_time_per_cell_ps' column...")
        
        # Ensure the divisor is not zero to avoid errors
        total_cells = df['x_size'] * df['y_size'] * df['steps']
        
        # Perform calculation, handling cases where total_cells is zero
        df['average_time_per_cell_ps'] = (df['average_time_ms'] * 1e9).divide(total_cells).fillna(0)
        
        # Write the updated DataFrame back to the original CSV file
        df.to_csv(filepath, index=False, float_format='%.6f')
        
        print(f"✅ Successfully updated '{filepath}'!")

    except FileNotFoundError:
        print(f"Error: The file '{filepath}' was not found.", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"An unexpected error occurred: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    # Set up the command-line argument parser
    parser = argparse.ArgumentParser(
        description="A script to correct the 'average_time_per_cell_ps' column in a CSV file."
    )
    parser.add_argument(
        "csv_file",
        help="The path to the CSV file that needs to be fixed."
    )
    args = parser.parse_args()

    # Run the main function with the provided file path
    fix_time_per_cell(args.csv_file)
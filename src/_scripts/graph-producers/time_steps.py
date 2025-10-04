import os
import sys
import matplotlib.pyplot as plt
import numpy as np

# --- 1. Data Loading (Your provided code) ---
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from abstractions.results_abstractions import CSVLoader, AUTOMATA

path_to_csv = "../results/grid-search-results.csv"
size=16384
time_steps = [2,4,6,8]
loader = CSVLoader(path_to_csv)
size_group = loader.get_groups_by_sizes([size**2])[size**2]
print(f"Total results for size {size}x{size}: {len(size_group)}")

one_step_linear = {'traverser': 'simple', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}
one_step_tiled = {'traverser': 'simple', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes'}
# one_step_tiled = {'traverser': 'simple', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}

temporal_linear = {'traverser': 'linear_temporal', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}
temporal_tiled = {'traverser': 'tiled_temporal', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes'}

one_step_linear_group = [x for x in size_group if x.is_implementation(one_step_linear)]
one_step_tiled_group = [x for x in size_group if x.is_implementation(one_step_tiled)]
temporal_linear_group = [x for x in size_group if x.is_implementation(temporal_linear)]
temporal_tiled_group = [x for x in size_group if x.is_implementation(temporal_tiled)]

bests_by_automaton = {}
for automaton in AUTOMATA:
    one_step_linear_for_automaton = [x for x in one_step_linear_group if x.values.get('automaton') == automaton]
    one_step_tiled_for_automaton = [x for x in one_step_tiled_group if x.values.get('automaton') == automaton]
    temporal_linear_for_automaton = [x for x in temporal_linear_group if x.values.get('automaton') == automaton]
    temporal_tiled_for_automaton = [x for x in temporal_tiled_group if x.values.get('automaton') == automaton]

    one_step_linear_best = min(one_step_linear_for_automaton, key=lambda x: x.normalized_time())
    one_step_tiled_best = min(one_step_tiled_for_automaton, key=lambda x: x.normalized_time())
    temporal_linear_bests = []
    temporal_tiled_bests = []

    for ts in time_steps:
        temporal_linear_bests.append(min([x for x in temporal_linear_for_automaton if x.values.get('temporal_steps') == ts], key=lambda x: x.normalized_time()))
        temporal_tiled_bests.append(min([x for x in temporal_tiled_for_automaton if x.values.get('temporal_steps') == ts], key=lambda x: x.normalized_time()))

    bests_by_automaton[automaton] = {
        'linear': [one_step_linear_best.normalized_time() * 1e9] + [x.normalized_time() * 1e9 for x in temporal_linear_bests],
        'tiled': [one_step_tiled_best.normalized_time() * 1e9] + [x.normalized_time() * 1e9 for x in temporal_tiled_bests]
    }
print("Finished processing data. Starting plot generation.")

# --- 2. Plotting Phase ---
automaton_names = {
    "game-of-life": "Game of Life", "forest-fire": "Forest Fire", "wire": "Wireworld",
    "greenberg-hastings": "Greenberg-Hastings", "brian": "Brian's Brain", "cyclic": "Cyclic",
    "traffic": "Traffic", "hpp": "HPP Gas", "maze": "Maze", "critters": "Critters"
}

# ⚙️ Graph Configuration
plot_config = {
    'plot_mode': 'subplots',       # 🆕 Options: 'combined' or 'subplots'
    'y_axis_mode': 'speedup',      # 🆕 Options: 'speedup' or 'throughput'
    'figure_size': (16, 8),
    'linear_color': '#08519c',     # Blue for linear
    'tiled_color': '#006d2c'       # Green for tiled
}

# --- Data Preparation ---
x_values = [1] + time_steps
plot_data = {}
y_axis_label = ""

if plot_config['y_axis_mode'] == 'speedup':
    y_axis_label = "Speedup vs. 1-Step Version"
    for automaton, results in bests_by_automaton.items():
        linear_baseline = results['linear'][0]
        tiled_baseline = results['tiled'][0]
        plot_data[automaton] = {
            'linear': [linear_baseline / t for t in results['linear']],
            'tiled': [tiled_baseline / t for t in results['tiled']]
        }
elif plot_config['y_axis_mode'] == 'throughput':
    y_axis_label = "Throughput (Giga Cell Updates Per Second)"
    for automaton, results in bests_by_automaton.items():
        plot_data[automaton] = {
            'linear': [1000 / t for t in results['linear']],
            'tiled': [1000 / t for t in results['tiled']]
        }

# --- Plotting Logic ---
if plot_config['plot_mode'] == 'combined':
    # --- Variant A: Everything in one graph ---
    fig, ax = plt.subplots(figsize=plot_config['figure_size'])
    ax.set_title('Overall Effect of Temporal Blocking')

    is_first_linear = True
    is_first_tiled = True
    for automaton, y_values in plot_data.items():
        # Plot linear line (only label the first one for a clean legend)
        ax.plot(x_values, y_values['linear'], marker='.',
                color=plot_config['linear_color'], alpha=0.5,
                label='Linear' if is_first_linear else "")
        is_first_linear = False

        # Plot tiled line (only label the first one)
        ax.plot(x_values, y_values['tiled'], marker='.',
                color=plot_config['tiled_color'], alpha=0.5,
                label='Tiled' if is_first_tiled else "")
        is_first_tiled = False

    ax.set_xlabel("Temporal Steps")
    ax.set_ylabel(y_axis_label)
    ax.set_xticks(x_values)
    ax.grid(True, which='both', linestyle='--', linewidth=0.5)
    ax.legend()
    fig.tight_layout()
    plt.savefig('temporal_scaling_combined.png', dpi=300)
    print("Combined graph saved as temporal_scaling_combined.png")

elif plot_config['plot_mode'] == 'subplots':
    # --- Variant B: Two subplots, one for linear, one for tiled ---
    fig, (ax1, ax2) = plt.subplots(nrows=1, ncols=2, figsize=plot_config['figure_size'], sharey=True)
    fig.suptitle('Effect of Temporal Blocking by Automaton')

    # Create a color cycle for the automata lines
    colors = plt.cm.viridis(np.linspace(0, 1, len(AUTOMATA)))
    automaton_colors = {name: color for name, color in zip(AUTOMATA, colors)}

    # Plot 1: Linear Implementations
    ax1.set_title('Linear Implementations')
    for automaton, y_values in plot_data.items():
        ax1.plot(x_values, y_values['linear'], marker='.',
                 label=automaton_names[automaton],
                 color=automaton_colors[automaton])

    # Plot 2: Tiled Implementations
    ax2.set_title('Tiled Implementations')
    for automaton, y_values in plot_data.items():
        ax2.plot(x_values, y_values['tiled'], marker='.',
                 label=automaton_names[automaton],
                 color=automaton_colors[automaton])

    # Common styling for both subplots
    for ax in [ax1, ax2]:
        ax.set_xlabel("Temporal Steps")
        ax.set_xticks(x_values)
        ax.grid(True, which='both', linestyle='--', linewidth=0.5)
        ax.legend()
    ax1.set_ylabel(y_axis_label)

    fig.tight_layout(rect=[0, 0.03, 1, 0.95]) # Adjust layout to make room for suptitle
    plt.savefig('temporal_scaling_subplots.png', dpi=300)
    print("Subplots graph saved as temporal_scaling_subplots.png")
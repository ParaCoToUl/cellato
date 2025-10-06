import os
import sys
import matplotlib.pyplot as plt
import numpy as np

# --- 1. Data Loading ---
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from abstractions.results_abstractions import CSVLoader, AUTOMATA

path_to_csv = "../results/16k-hot-results.csv"
size=16384
loader = CSVLoader(path_to_csv)
size_group = loader.get_groups_by_sizes([size**2])[size**2]
print(f"Total results for size {size}x{size}: {len(size_group)}")

baseline_impl = {'reference_impl': 'baseline'}
bit_array_impl = {'traverser': 'simple', 'evaluator': 'bit_array', 'layout': 'bit_array'}
bit_planes_linear = {'traverser': 'simple', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}
bit_planes_tiled = {'traverser': 'simple', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes'}
temporal_linear = {'traverser': 'temporal', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}
temporal_tiled = {'traverser': 'temporal', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes'}

baseline_group = [x for x in size_group if x.is_implementation(baseline_impl)]
bit_array_group = [x for x in size_group if x.is_implementation(bit_array_impl)]
bit_planes_group = [x for x in size_group if x.is_implementation(bit_planes_linear) or x.is_implementation(bit_planes_tiled)]
temporal_group = [x for x in size_group if x.is_implementation(temporal_linear) or x.is_implementation(temporal_tiled)]

bests_by_automaton = {}
for automaton in AUTOMATA:
    baseline_for_automaton = [x for x in baseline_group if x.values.get('automaton') == automaton]
    bit_array_for_automaton = [x for x in bit_array_group if x.values.get('automaton') == automaton]
    bit_planes_for_automaton = [x for x in bit_planes_group if x.values.get('automaton') == automaton]
    temporal_for_automaton = [x for x in temporal_group if x.values.get('automaton') == automaton]

    baseline_best = min(baseline_for_automaton, key=lambda x: x.normalized_time())
    bit_array_best = min(bit_array_for_automaton, key=lambda x: x.normalized_time())
    bit_planes_best = min(bit_planes_for_automaton, key=lambda x: x.normalized_time())
    temporal_best = min(temporal_for_automaton, key=lambda x: x.normalized_time())

    bests_by_automaton[automaton] = {
        'baseline': baseline_best.normalized_time() * 1e9,
        'bit_array': bit_array_best.normalized_time() * 1e9,
        'bit_planes': bit_planes_best.normalized_time() * 1e9,
        'temporal': temporal_best.normalized_time() * 1e9
    }
print("Finished processing data. Starting plot generation.")

# --- 2. Plotting Phase ---
automaton_names = {
    "game-of-life": "Game of Life", "forest-fire": "Forest Fire", "wire": "Wireworld",
    "greenberg-hastings": "Greenberg-Hastings", "brian": "Brian's Brain", "cyclic": "Cyclic",
    "traffic": "Traffic", "hpp": "HPP Gas", "maze": "Maze", "critters": "Critters"
}

# ⚙️ Graph Configuration
scale = 0.8
plot_config = {
    'y_axis_mode': 'speedup',
    'y_axis_scale': 'log',
    'figure_size': (16*scale, 9*scale),
    'bar_width': 0.2,
    'title': f'Performance Comparison for {size}x{size} Grid',
    'show_baseline_bar': False,
    'show_baseline_line': True,
    'add_data_labels': True,
    'label_fontsize': 10,
    'label_use_background': True,
    'label_rotation': 45,
    'label_padding': 3,
    'custom_colors': {
        'baseline': '#003f5c', 'bit_array': '#7a5195',
        'bit_planes': '#ef5675', 'temporal': '#ffa600'
    },
    'bar_hatches': {  # More subtle hatch patterns
        'baseline': '/',      # single diagonal lines
        'bit_array': '\\',    # back diagonal lines
        'bit_planes': '.',    # dots
        'temporal': 'o'       # small circles
    },
    'hatch_density': 0.5      # Controls how dense the hatches appear (lower = more subtle)
}

# --- Data Preparation ---
labels = [automaton_names.get(a, a) for a in AUTOMATA]
implementations = ['baseline', 'bit_array', 'bit_planes', 'temporal']
impl_display_names = {'baseline': 'Baseline', 'bit_array': 'Bit Array', 'bit_planes': 'Bit Planes', 'temporal': 'Temporal'}
data = {}

if not plot_config['show_baseline_bar']:
    implementations.remove('baseline')
for impl in implementations:
    data[impl] = []
if plot_config['y_axis_mode'] == 'speedup':
    y_axis_label = "Speedup Relative to Baseline"
    for automaton in AUTOMATA:
        baseline_time = bests_by_automaton[automaton]['baseline']
        for impl in implementations:
            data[impl].append(1.0 if impl == 'baseline' else baseline_time / bests_by_automaton[automaton][impl])
else:
    y_axis_label = "Execution Time per Cell (ps)"
    for automaton in AUTOMATA:
        for impl in implementations:
            data[impl].append(bests_by_automaton[automaton][impl])

# --- Plotting ---
fig, ax = plt.subplots(figsize=plot_config['figure_size'])
x = np.arange(len(labels))
width = plot_config['bar_width']
num_implementations = len(implementations)
offsets = np.linspace(-width * (num_implementations - 1) / 2, width * (num_implementations - 1) / 2, num_implementations)
bar_containers = {}

for i, impl in enumerate(implementations):
    color = plot_config['custom_colors'].get(impl)
    hatch = plot_config['bar_hatches'].get(impl)
    # Use a single character for more subtle hatching with density control
    if hatch and plot_config.get('hatch_density', 1) < 1:
        hatch = hatch[0]  # Just use one character for subtlety
    bars = ax.bar(x + offsets[i], data[impl], width, label=impl_display_names[impl], color=color, hatch=hatch)
    bar_containers[impl] = bars

# --- Styling and Customization ---
ax.set_ylabel(y_axis_label)
ax.set_title(plot_config['title'])
ax.set_xticks(x)
ax.set_xticklabels(labels, rotation=45, ha="right")
ax.set_yscale(plot_config['y_axis_scale'])
ax.grid(axis='y', linestyle='--', alpha=0.7)

if plot_config['show_baseline_line'] and plot_config['y_axis_mode'] == 'speedup':
    col = 'red'
    width = 1.4
    ax.axhline(y=1, color=col, linestyle='--', linewidth=width)
    ax.plot([], [], color=col, linestyle='--', linewidth=width, label='Baseline Performance (1x)')

if plot_config['add_data_labels']:
    bbox_props = dict(boxstyle="round,pad=0.3", fc="white", ec="none", alpha=0.8) if plot_config['label_use_background'] else None
    for impl, bars in bar_containers.items():
        label_format = '%.1fx' if plot_config['y_axis_mode'] == 'speedup' else '%d'
        ax.bar_label(bars, fmt=label_format,
                     padding=plot_config['label_padding'],
                     fontsize=plot_config['label_fontsize'],
                     bbox=bbox_props,
                     rotation=plot_config['label_rotation'])

if plot_config['show_baseline_line'] and plot_config['y_axis_mode'] == 'speedup':
    current_ylim = ax.get_ylim()
    ax.set_ylim(bottom=min(1.0, current_ylim[0]), top=current_ylim[1] * 1.3)

ax.legend(loc='upper left', bbox_to_anchor=(0.23, 0.98), borderaxespad=0.)
fig.tight_layout()

# --- Saving ---
plt.savefig('performance_graph.png', dpi=300, bbox_inches='tight')
print("Graph successfully saved as performance_graph.png")
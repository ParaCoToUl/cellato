import os
import sys
import matplotlib.pyplot as plt
import numpy as np

# --- 1. Data Loading ---
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from abstractions.results_abstractions import CSVLoader, AUTOMATA

path_to_csv = "../results/grid-search-results.csv"
size=16384
loader = CSVLoader(path_to_csv)
size_group = loader.get_groups_by_sizes([size**2])[size**2]
print(f"Total results for size {size}x{size}: {len(size_group)}")

# --- Implementation definitions for the new comparison ---
bit_planes_linear_impl = {'traverser': 'simple', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}
bit_planes_tiled_impl = {'traverser': 'simple', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes'}
temporal_linear_impl = {'traverser': 'linear_temporal', 'evaluator': 'bit_planes', 'layout': 'bit_planes'}
temporal_tiled_impl = {'traverser': 'tiled_temporal', 'evaluator': 'tiled_bit_planes', 'layout': 'tiled_bit_planes'}

# --- Group results for the four selected implementations ---
bit_planes_linear_group = [x for x in size_group if x.is_implementation(bit_planes_linear_impl)]
bit_planes_tiled_group = [x for x in size_group if x.is_implementation(bit_planes_tiled_impl)]
temporal_linear_group = [x for x in size_group if x.is_implementation(temporal_linear_impl)]
temporal_tiled_group = [x for x in size_group if x.is_implementation(temporal_tiled_impl)]

# --- Find the best time for each of the four implementations ---
bests_by_automaton = {}
for automaton in AUTOMATA:
    bpl_best = min([x for x in bit_planes_linear_group if x.values.get('automaton') == automaton], key=lambda x: x.normalized_time())
    bpt_best = min([x for x in bit_planes_tiled_group if x.values.get('automaton') == automaton], key=lambda x: x.normalized_time())
    tl_best = min([x for x in temporal_linear_group if x.values.get('automaton') == automaton], key=lambda x: x.normalized_time())
    tt_best = min([x for x in temporal_tiled_group if x.values.get('automaton') == automaton], key=lambda x: x.normalized_time())

    bests_by_automaton[automaton] = {
        'bit_planes_linear': bpl_best.normalized_time() * 1e9,
        'bit_planes_tiled': bpt_best.normalized_time() * 1e9,
        'temporal_linear': tl_best.normalized_time() * 1e9,
        'temporal_tiled': tt_best.normalized_time() * 1e9
    }
print("Finished processing data. Starting plot generation.")

# --- 2. Plotting Phase ---
automaton_names = {
    "game-of-life": "Game of Life", "forest-fire": "Forest Fire", "wire": "Wireworld",
    "greenberg-hastings": "Greenberg-Hastings", "brian": "Brian's Brain", "cyclic": "Cyclic",
    "traffic": "Traffic", "hpp": "HPP Gas", "maze": "Maze", "critters": "Critters"
}

# ⚙️ New Graph Configuration
plot_config = {
    'y_axis_scale': 'linear',
    'figure_size': (16, 9),
    'bar_width': 0.18,
    'group_gap': 0.02,
    'title': f'Linear vs. Tiled Performance for {size}x{size} Grid (Absolute Time)',
    'add_data_labels': True,
    'label_fontsize': 10,
    'label_rotation': 45, # Adjusted rotation for better look with "Xx" format
    'label_padding': 5,
    'custom_colors': {
        'bit_planes_linear': '#6baed6', 'bit_planes_tiled': '#74c476',
        'temporal_linear': '#08519c', 'temporal_tiled': '#006d2c'
    },
    'bar_hatches': {
        'bit_planes_linear': '/', 'bit_planes_tiled': '//',
        'temporal_linear': 'x', 'temporal_tiled': 'xx'
    }
}

# --- Data Preparation ---
labels = [automaton_names.get(a, a) for a in AUTOMATA]
implementations = ['bit_planes_linear', 'bit_planes_tiled', 'temporal_linear', 'temporal_tiled']
impl_display_names = {
    'bit_planes_linear': 'Bit Planes (Linear)', 'bit_planes_tiled': 'Bit Planes (Tiled)',
    'temporal_linear': 'Temporal (Linear)', 'temporal_tiled': 'Temporal (Tiled)'
}
data = {impl: [] for impl in implementations}

y_axis_label = "Execution Time per Cell (ps) — Lower is Better"
for automaton in AUTOMATA:
    for impl in implementations:
        data[impl].append(bests_by_automaton[automaton][impl])

# --- Plotting ---
fig, ax = plt.subplots(figsize=plot_config['figure_size'])
x = np.arange(len(labels))
width = plot_config['bar_width']
gap = plot_config['group_gap']
offsets = {
    'bit_planes_linear': -1.5*width - gap, 'bit_planes_tiled': -0.5*width - gap,
    'temporal_linear': 0.5*width + gap, 'temporal_tiled': 1.5*width + gap
}
bar_containers = {}

for impl in implementations:
    color = plot_config['custom_colors'].get(impl)
    hatch = plot_config['bar_hatches'].get(impl)
    bars = ax.bar(x + offsets[impl], data[impl], width, label=impl_display_names[impl], color=color, hatch=hatch, edgecolor='white')
    bar_containers[impl] = bars

# --- Styling and Customization ---
ax.set_ylabel(y_axis_label)
ax.set_title(plot_config['title'])
ax.set_xticks(x)
ax.set_xticklabels(labels, rotation=45, ha="right")
ax.set_yscale(plot_config['y_axis_scale'])
ax.grid(axis='y', linestyle='--', alpha=0.7)

# 🆕 --- Modified Data Label Logic ---
if plot_config['add_data_labels']:
    # 1. Calculate and apply speedup labels for Tiled Bit Planes
    bpt_speedup_labels = [f'{(linear / tiled):.2f}x' for linear, tiled in zip(data['bit_planes_linear'], data['bit_planes_tiled'])]
    ax.bar_label(bar_containers['bit_planes_tiled'], labels=bpt_speedup_labels,
                 padding=plot_config['label_padding'],
                 fontsize=plot_config['label_fontsize'],
                 rotation=plot_config['label_rotation'])

    # 2. Calculate and apply speedup labels for Tiled Temporal
    tt_speedup_labels = [f'{(linear / tiled):.2f}x' for linear, tiled in zip(data['temporal_linear'], data['temporal_tiled'])]
    ax.bar_label(bar_containers['temporal_tiled'], labels=tt_speedup_labels,
                 padding=plot_config['label_padding'],
                 fontsize=plot_config['label_fontsize'],
                 rotation=plot_config['label_rotation'])
    # Linear bars are intentionally left without labels

current_ylim = ax.get_ylim()
ax.set_ylim(top=current_ylim[1] * 1.3) # Increased top margin for rotated labels

ax.legend(loc='upper right')
fig.tight_layout()

# --- Saving ---
plt.savefig('linear_vs_tiled_performance.png', dpi=300, bbox_inches='tight')
print("Graph successfully saved as linear_vs_tiled_performance.png")
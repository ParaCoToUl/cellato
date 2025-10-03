import os
import sys
from results_abstractions import CSVLoader, IMPLEMENTATIONS, RunResult


path_to_csv = "../results/grid-search-results.csv"
loader = CSVLoader(path_to_csv)

impl_name="Temporal Linear (32-bit)"
impl_dict = IMPLEMENTATIONS[impl_name]

size=16384**2
group = loader.get_groups_by_sizes([size], tolerance=0.1)[size]

automaton: list[RunResult] = loader.split_by_automaton(group)["game-of-life"]

for r in sorted(automaton, key=lambda r: r.normalized_time()):
    if r.is_implementation(impl_dict):
        print(f"{r.normalized_time() * 1e9:.3f} ps, temporal_steps: {r.values['temporal_steps']}, temporal_tile_size_y: {r.values['temporal_tile_size_y']}")


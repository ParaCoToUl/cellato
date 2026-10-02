#!/usr/bin/env python3

"""Exercise user-facing diagnostics without requiring a CUDA device."""

import argparse
import subprocess
import unittest

parser = argparse.ArgumentParser()
parser.add_argument("--binary", required=True)
parser.add_argument("--compile-mode", default="")
parser.add_argument("--cuda-enabled", type=int, choices=(0, 1), default=1)
options = parser.parse_args()


class CLIValidation(unittest.TestCase):
    base = dict(automaton="game-of-life", device="CPU", traverser="simple",
                evaluator="standard", layout="standard", x_size=128, y_size=128, steps=8)

    def run_cli(self, params):
        command = [options.binary]
        for name, value in params.items():
            if value is not None:
                command.extend(("--" + name, str(value)))
        return subprocess.run(command, text=True, capture_output=True, timeout=30, check=False)

    def expect_error(self, fragments, **changes):
        result = self.run_cli(self.base | changes)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertTrue(result.stderr.startswith("Error: "), result.stderr)
        for fragment in fragments:
            self.assertIn(fragment, result.stderr)
        self.assertNotIn("No suitable", result.stderr)
        self.assertEqual(result.stdout, "", result.stdout)

    def temporal(self, **changes):
        return dict(device="CUDA", traverser="temporal", evaluator="bit_planes",
                    layout="bit_planes", word_size=64, x_size=3840, y_size=48,
                    temporal_steps=4, temporal_tile_size_y=32, cuda_block_size_x=32,
                    cuda_block_size_y=4 if options.compile_mode == "VERIFICATION" else 8) | changes

    def test_unknown_names(self):
        for option, valid in (("automaton", "game-of-life"), ("device", "CPU"),
                              ("traverser", "simple"), ("evaluator", "bit_planes"),
                              ("layout", "tiled_bit_planes")):
            with self.subTest(option=option):
                self.expect_error([f"Unknown --{option} 'typo'", "Available values:", valid], **{option: "typo"})
        devices = "CPU, CUDA" if options.cuda_enabled else "CPU"
        self.expect_error(["Unknown --device 'cuda'", f"Available values: {devices}."], device="cuda")
        self.expect_error(["Unknown --traverser 'standard'", "simple"], traverser="standard")

    def test_unknown_name_precedes_combination_and_dependent_flags(self):
        self.expect_error(["Unknown --layout 'typo'"], evaluator="bit_planes", layout="typo")
        if options.cuda_enabled:
            self.expect_error(["Unknown --layout 'typo'"], traverser="temporal", layout="typo")
        self.expect_error(["Unknown --automaton 'typo'"], automaton="typo", evaluator="bit_planes")

    def test_known_but_unsupported_combinations(self):
        self.expect_error(["Unsupported combination:", "--evaluator bit_planes --layout bit_array",
                           "Supported --layout values", "bit_planes"], evaluator="bit_planes", layout="bit_array")

    @unittest.skipUnless(options.cuda_enabled, "CUDA suites are disabled")
    def test_known_but_unsupported_cuda_combinations(self):
        self.expect_error(["Unsupported combination:", "--device CPU --traverser temporal",
                           "Supported --traverser values", "simple"], traverser="temporal")
        self.expect_error(["Unsupported combination:", "--device CPU --traverser spatial_blocking"],
                          traverser="spatial_blocking")
        self.expect_error(["Unsupported combination:", "--traverser temporal --evaluator standard",
                           "bit_planes", "tiled_bit_planes"], device="CUDA", traverser="temporal")
        self.expect_error(["Unsupported combination:", "--traverser spatial_blocking --evaluator bit_planes",
                           "standard"], device="CUDA", traverser="spatial_blocking", evaluator="bit_planes")

    @unittest.skipIf(options.cuda_enabled, "CUDA suites are enabled")
    def test_disabled_cuda(self):
        for reference in ("none", "baseline"):
            with self.subTest(reference=reference):
                self.expect_error(["CUDA support is disabled", "Available --device values: CPU.",
                                   "-DCELLATO_ENABLE_CUDA=ON"], device="CUDA", reference_impl=reference)
        for traverser in ("temporal", "spatial_blocking"):
            with self.subTest(traverser=traverser):
                self.expect_error([f"Unknown --traverser '{traverser}'", "Available values: simple."],
                                  traverser=traverser)

    def test_word_size(self):
        packed = dict(evaluator="bit_planes", layout="bit_planes")
        self.expect_error(["Missing required option: --word_size"], **packed)
        for size in (0, 8, 16, 128):
            with self.subTest(size=size):
                self.expect_error([f"Unsupported --word_size {size}", "32, 64"], **packed, word_size=size)

    @unittest.skipUnless(options.cuda_enabled, "CUDA suites are disabled")
    def test_spatial_tiles(self):
        spatial = dict(device="CUDA", traverser="spatial_blocking")
        self.expect_error(["Missing required option: --x_tile_size"], **spatial)
        self.expect_error(["Missing required option: --y_tile_size"], **spatial, x_tile_size=1)
        for x, y in ((0, 0), (2, 1), (1, 8)):
            with self.subTest(x=x, y=y):
                self.expect_error(["Unsupported spatial tile:", f"--x_tile_size {x} --y_tile_size {y}",
                                   "Available combinations:", "(--x_tile_size 1 --y_tile_size 4)"],
                                  **spatial, x_tile_size=x, y_tile_size=y)

    def test_missing_and_malformed_values(self):
        self.expect_error(["Missing required option: --automaton"], automaton=None)
        self.expect_error(["Invalid integer for --steps: nope"], steps="nope")
        self.expect_error(["Invalid integer for --steps: 8tail"], steps="8tail")
        self.expect_error(["Integer out of range for --steps"], steps="999999999999999999999999")
        result = subprocess.run([options.binary, "--automaton"], text=True, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 1)
        self.assertIn("Missing required option:", result.stderr)

    def test_numeric_ranges(self):
        for name, value in (("x_size", 0), ("y_size", -1), ("steps", -1),
                            ("rounds", 0), ("warmup_rounds", -1)):
            with self.subTest(option=name):
                self.expect_error([f"Invalid --{name} {value}"], **{name: value})

    @unittest.skipUnless(options.cuda_enabled, "CUDA suites are disabled")
    def test_cuda_numeric_ranges(self):
        for name in ("cuda_block_size_x", "cuda_block_size_y"):
            self.expect_error([f"Invalid --{name} 0"], device="CUDA", **{name: 0})
        self.expect_error(["Invalid CUDA block", "must not exceed 1024"], device="CUDA", cuda_block_size_x=1024)

    def test_layout_grid_dimensions(self):
        self.expect_error(["Invalid --x_size 100", "divisible by 64", "--layout bit_planes"],
                          evaluator="bit_planes", layout="bit_planes", word_size=64, x_size=100)
        self.expect_error(["Invalid --y_size 10", "divisible by 8", "--layout tiled_bit_planes"],
                          evaluator="tiled_bit_planes", layout="tiled_bit_planes", word_size=64, y_size=10)

    @unittest.skipUnless(options.cuda_enabled, "CUDA suites are disabled")
    def test_cuda_grid_dimensions(self):
        self.expect_error(["Invalid --x_size 128", "divisible by 2048", "--cuda_block_size_x 32"],
                          device="CUDA", evaluator="bit_planes", layout="bit_planes", word_size=64)

    @unittest.skipUnless(options.cuda_enabled, "CUDA suites are disabled")
    def test_temporal_compiled_options(self):
        self.expect_error(["Missing required option: --temporal_steps"], **self.temporal(temporal_steps=None))
        self.expect_error(["Missing required option: --temporal_tile_size_y"], **self.temporal(temporal_tile_size_y=None))
        for name, value in (("temporal_steps", 997), ("temporal_tile_size_y", 1),
                            ("cuda_block_size_x", 16), ("cuda_block_size_y", 1)):
            with self.subTest(option=name):
                self.expect_error([f"Unsupported --{name} {value}", "Values compiled into this build:"],
                                  **self.temporal(**{name: value}))
        allowed_steps = {"": "4.", "VERIFICATION": "4, 8, 12, 20.",
                         "BENCHMARK": "2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 17, 18, 20, 22, 24."}
        self.expect_error(["Values compiled into this build: " + allowed_steps[options.compile_mode]],
                          **self.temporal(temporal_steps=997))

    @unittest.skipUnless(options.cuda_enabled, "CUDA suites are disabled")
    def test_temporal_divisibility(self):
        self.expect_error(["Invalid --steps 9", "divisible by 4", "--temporal_steps"], **self.temporal(steps=9))
        self.expect_error(["Invalid --x_size 4096", "divisible by 1920", "effective temporal tile width"],
                          **self.temporal(x_size=4096))
        self.expect_error(["Invalid --y_size 49", "divisible by 24", "effective temporal tile height"],
                          **self.temporal(y_size=49))

        if options.compile_mode:
            self.expect_error(["Invalid temporal tile:", "effective tile of 30x0"],
                              **self.temporal(temporal_tile_size_y=8))

    def test_reference_dispatch(self):
        self.expect_error(["Unknown --reference_impl 'typo'", "none, baseline"], reference_impl="typo")
        self.expect_error(["Unknown --automaton 'typo'"], automaton="typo", reference_impl="baseline")
        self.expect_error(["Missing required option: --device"], reference_impl="none", device=None)
        # Reference selection bypasses evaluator/traverser options, and keeps its fire alias.
        self.expect_error(["Invalid --x_size 0"], reference_impl="baseline", automaton="fire", x_size=0,
                          device=None, traverser=None, evaluator="bit_planes", layout=None)

    def test_successful_cpu_runs_and_checksum(self):
        expected = "0-0-0-0-0-0-1-0-0-3-1-1-2-0-0-0"
        result = self.run_cli(self.base | dict(x_size=8, y_size=8, steps=2, seed=1))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip().split(",")[-1], expected)
        for layout in ("standard", "bit_array", "bit_planes", "tiled_bit_planes"):
            with self.subTest(layout=layout):
                result = self.run_cli(self.base | dict(evaluator=layout, layout=layout, word_size=64, steps=0))
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("game-of-life,CPU,simple," + layout, result.stdout)

    def test_cpu_layouts_match_baseline(self):
        help_result = subprocess.run([options.binary, "--help"], text=True, capture_output=True,
                                     timeout=30, check=False)
        self.assertEqual(help_result.returncode, 0, help_result.stderr)
        automata_line = next(line for line in help_result.stdout.splitlines() if "--automaton <name>" in line)
        automata = automata_line.split("<name>", 1)[1].strip().split(", ")
        layouts = [("standard", 0)] + [(layout, word_size)
                                      for layout in ("bit_array", "bit_planes", "tiled_bit_planes")
                                      for word_size in (32, 64)]
        for automaton in automata:
            for seed in (1, 42):
                # Align every packed layout, including 3-bit and 5-bit cells in 32/64-bit words.
                params = self.base | dict(automaton=automaton, x_size=6720, y_size=16, steps=4, seed=seed)
                baseline = self.run_cli(params | dict(reference_impl="baseline", device=None,
                                                     traverser=None, evaluator=None, layout=None))
                self.assertEqual(baseline.returncode, 0, baseline.stderr)
                expected = baseline.stdout.strip().split(",")[-1]
                for layout, word_size in layouts:
                    with self.subTest(automaton=automaton, seed=seed, layout=layout, word_size=word_size):
                        result = self.run_cli(params | dict(evaluator=layout, layout=layout, word_size=word_size))
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout.strip().split(",")[-1], expected)

    def test_help_and_csv(self):
        for flag in ("--help", "--print_csv_header"):
            result = subprocess.run([options.binary, flag], text=True, capture_output=True, timeout=30, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stderr, "")
            if flag == "--help":
                for value in ("simple", "tiled_bit_planes", "forest-fire"):
                    self.assertIn(value, result.stdout)
                if options.cuda_enabled:
                    self.assertIn("temporal", result.stdout)
                    self.assertIn("CUDA", result.stdout)
                else:
                    self.assertNotIn("temporal", result.stdout)
                    self.assertNotIn("CUDA", result.stdout)
            else:
                self.assertTrue(result.stdout.startswith("automaton,device,traverser,"))


if __name__ == "__main__":
    unittest.main(argv=[__file__], verbosity=2)

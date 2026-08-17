# Fuzzy Python Simulation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a manual Python simulator that reproduces the ESP32 Mamdani fuzzy algorithm and compares its integer setpoint with a hardware value entered by the user.

**Architecture:** Keep the simulator independent from PlatformIO. Use `scikit-fuzzy` and NumPy for membership functions and the discrete output universe, while implementing the firmware's 27-rule evaluation, max aggregation, centroid calculation, fallback, clamping, and positive-value rounding explicitly in one focused CLI module. Verify the module with standard-library `unittest` tests using fixed expected values and boundary cases.

**Tech Stack:** Python 3, NumPy, scikit-fuzzy, argparse, unittest.

---

## File Map

- Create: `tools/simulasi_fuzzy.py` - fuzzy parameters copied from `include/config.h`, inference functions, result formatting, and interactive/CLI entry points.
- Create: `tools/requirements.txt` - simulator-only Python dependencies; PlatformIO dependencies remain unchanged.
- Create: `tests/test_simulasi_fuzzy.py` - deterministic tests for membership functions, rule aggregation, centroid behavior, rounding, and a representative end-to-end case.
- No firmware files will be modified.

## Task 1: Add simulator dependencies and test scaffold

**Files:**
- Create: `tools/requirements.txt`
- Create: `tests/test_simulasi_fuzzy.py`

- [ ] **Step 1: Add the isolated dependency file**

Create `tools/requirements.txt` with:

```text
numpy>=1.24
scikit-fuzzy>=0.4.2
```

This file must not be added to `platformio.ini`; it is only for the host-side simulator.

- [ ] **Step 2: Add a failing test module with the public API contract**

Create `tests/test_simulasi_fuzzy.py` with:

```python
import math
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from simulasi_fuzzy import (  # noqa: E402
    OUTPUT_UNIVERSE,
    compute_fuzzy,
    round_firmware,
    trap_membership,
    triangle_membership,
)


class MembershipFunctionTests(unittest.TestCase):
    def test_trapezoid_matches_firmware_boundaries_and_peak(self):
        mf = (16.0, 18.0, 22.0, 25.0)
        self.assertEqual(trap_membership(16.0, mf), 0.0)
        self.assertAlmostEqual(trap_membership(17.0, mf), 0.5)
        self.assertEqual(trap_membership(20.0, mf), 1.0)
        self.assertAlmostEqual(trap_membership(23.5, mf), 0.5)
        self.assertEqual(trap_membership(25.0, mf), 0.0)

    def test_triangle_matches_firmware_boundaries_and_peak(self):
        mf = (23.0, 25.0, 28.0)
        self.assertEqual(triangle_membership(23.0, mf), 0.0)
        self.assertAlmostEqual(triangle_membership(24.0, mf), 0.5)
        self.assertEqual(triangle_membership(25.0, mf), 1.0)
        self.assertAlmostEqual(triangle_membership(26.5, mf), 0.5)
        self.assertEqual(triangle_membership(28.0, mf), 0.0)


class InferenceTests(unittest.TestCase):
    def test_output_universe_has_firmware_range_and_step(self):
        self.assertEqual(len(OUTPUT_UNIVERSE), 141)
        self.assertAlmostEqual(float(OUTPUT_UNIVERSE[0]), 16.0)
        self.assertAlmostEqual(float(OUTPUT_UNIVERSE[-1]), 30.0)
        self.assertAlmostEqual(float(OUTPUT_UNIVERSE[1] - OUTPUT_UNIVERSE[0]), 0.1)

    def test_no_rule_firing_uses_firmware_default(self):
        result = compute_fuzzy(10.0, 10.0, 20.0)
        self.assertEqual(result.firing, (0.0, 0.0, 0.0))
        self.assertEqual(result.crisp_setpoint, 23.0)
        self.assertEqual(result.setpoint_int, 23)

    def test_representative_case_produces_expected_output(self):
        result = compute_fuzzy(27.32, 56.23, 5.0)
        self.assertAlmostEqual(result.firing[0], 0.0, places=6)
        self.assertAlmostEqual(result.firing[1], 0.2266666667, places=6)
        self.assertAlmostEqual(result.firing[2], 0.0, places=6)
        self.assertAlmostEqual(result.crisp_setpoint, 26.3381223, places=5)
        self.assertEqual(result.setpoint_int, 26)

    def test_rounding_is_not_python_bankers_rounding(self):
        self.assertEqual(round_firmware(22.5), 23)
        self.assertEqual(round_firmware(23.5), 24)
        self.assertEqual(round_firmware(24.49), 24)


if __name__ == "__main__":
    unittest.main()
```

The representative expected values come from the firmware-equivalent calculation:
only the `Nyaman`, `Normal`, `Sedang` and `Panas`, `Normal`, `Sedang` rule paths
fire, and both paths map to output `Sedang`. The expected crisp value is based
on the `Sedang` output membership clipped at `0.2266666667`.

- [ ] **Step 3: Run the scaffold to verify it fails for the missing module**

Run:

```bash
python -m unittest discover -s tests -p "test_*.py" -v
```

Expected: FAIL because `tools/simulasi_fuzzy.py` does not exist yet. Do not treat a missing `numpy` or `skfuzzy` installation as a passing result; install the requirements before continuing to implementation.

## Task 2: Implement firmware-equivalent fuzzy inference

**Files:**
- Create: `tools/simulasi_fuzzy.py`

- [ ] **Step 1: Define the parameter model and constants**

Implement the following module-level types and constants:

```python
from dataclasses import dataclass
from typing import Sequence

import numpy as np
import skfuzzy as fuzz

Trap = tuple[float, float, float, float]
Triangle = tuple[float, float, float]

OUTPUT_UNIVERSE = np.arange(16.0, 30.0 + 0.05, 0.1)

TEMP_MF = {
    "dingin": (16.0, 18.0, 22.0, 25.0),
    "nyaman": (23.0, 25.0, 28.0),
    "panas": (27.0, 29.0, 31.0, 35.0),
}
HUMID_MF = {
    "kering": (20.0, 30.0, 50.0, 55.0),
    "normal": (50.0, 65.0, 80.0),
    "lembab": (70.0, 80.0, 90.0, 100.0),
}
OCC_MF = {
    "sedikit": (0.0, 1.0, 3.0, 4.0),
    "sedang": (3.0, 4.0, 7.0),
    "banyak": (6.0, 8.0, 9.0, 10.0),
}
SETPOINT_MF = {
    "rendah": (17.0, 18.0, 22.0, 24.0),
    "sedang": (23.0, 25.0, 30.0),
    "tinggi": (25.0, 30.0, 32.0, 33.0),
}

RULES = (
    ((2, 2, 1), (2, 2, 1), (1, 1, 0)),
    ((2, 2, 1), (1, 1, 0), (1, 1, 1)),
    ((1, 1, 0), (1, 1, 0), (0, 0, 0)),
)

@dataclass(frozen=True)
class FuzzyResult:
    crisp_setpoint: float
    setpoint_int: int
    firing: tuple[float, float, float]
    memberships: dict[str, dict[str, float]]
```

`RULES[suhu][kelembaban][hunian]` must preserve the exact 3x3x3 order in `config.h`: each innermost tuple is for hunian `sedikit`, `sedang`, `banyak`, and labels remain `0=Rendah`, `1=Sedang`, `2=Tinggi`.

- [ ] **Step 2: Implement scikit-fuzzy membership wrappers with firmware boundary behavior**

Implement these functions:

```python
def trap_membership(x: float, points: Trap) -> float:
    a, _, _, d = points
    if x <= a or x >= d:
        return 0.0
    values = fuzz.trapmf(
        np.asarray([x], dtype=float), np.asarray(points, dtype=float)
    )
    return float(values[0])


def triangle_membership(x: float, points: Triangle) -> float:
    a, _, c = points
    if x <= a or x >= c:
        return 0.0
    values = fuzz.trimf(
        np.asarray([x], dtype=float), np.asarray(points, dtype=float)
    )
    return float(values[0])
```

Because evaluating a membership function at only its parameter points is not correct for arbitrary sensor values, the actual implementation must instead evaluate on a small local universe containing the breakpoints and `x`, then select the value at `x`, while explicitly returning `0.0` when firmware's condition `x <= a || x >= d` or `x <= a || x >= c` applies. For interior values, use the corresponding `skfuzzy.trapmf` or `skfuzzy.trimf` result. Keep the wrappers as public functions so the boundary tests can verify them.

The implementation must use the same strict outer-boundary behavior as `FuzzyMamdani.cpp`, including triangle endpoints. It must not apply temperature or humidity offsets.

- [ ] **Step 3: Implement fuzzification and explicit rule aggregation**

Implement:

```python
def _membership_for_input(value, definitions):
    result = {}
    for name, points in definitions.items():
        if len(points) == 4:
            result[name] = trap_membership(value, points)
        else:
            result[name] = triangle_membership(value, points)
    return result


def _aggregate_firing(memberships):
    temp = [memberships["suhu"][name] for name in ("dingin", "nyaman", "panas")]
    humid = [memberships["kelembaban"][name] for name in ("kering", "normal", "lembab")]
    occ = [memberships["orang"][name] for name in ("sedikit", "sedang", "banyak")]
    firing = [0.0, 0.0, 0.0]
    for i in range(3):
        for j in range(3):
            for k in range(3):
                value = min(temp[i], humid[j], occ[k])
                label = RULES[i][j][k]
                firing[label] = max(firing[label], value)
    return tuple(firing)
```

The final implementation may use more precise type annotations or local helper names, but it must keep the same loop order and `min`/`max` operations. Return all input membership values in `FuzzyResult.memberships` for display and verification.

- [ ] **Step 4: Implement centroid, fallback, clamp, and Arduino-compatible rounding**

Implement the equivalent of `FuzzyMamdani::defuzzify`:

```python
def _defuzzify(firing):
    if sum(firing) == 0.0:
        return 23.0
    output_low = np.minimum(firing[0], fuzz.trapmf(OUTPUT_UNIVERSE, SETPOINT_MF["rendah"]))
    output_mid = np.minimum(firing[1], fuzz.trimf(OUTPUT_UNIVERSE, SETPOINT_MF["sedang"]))
    output_high = np.minimum(firing[2], fuzz.trapmf(OUTPUT_UNIVERSE, SETPOINT_MF["tinggi"]))
    aggregate = np.maximum.reduce((output_low, output_mid, output_high))
    return float(np.sum(OUTPUT_UNIVERSE * aggregate) / np.sum(aggregate))


def round_firmware(value: float) -> int:
    return int(np.floor(value + 0.5))
```

After defuzzification, clamp with `min(max(crisp, 16.0), 30.0)` before calling `round_firmware`, matching the firmware's `constrain` and `round` for positive setpoints. The defuzzifier must use 141 output samples including both 16.0 and 30.0. If the denominator is zero, return 23.0 before division.

- [ ] **Step 5: Add the public `compute_fuzzy` function**

Implement:

```python
def compute_fuzzy(suhu: float, kelembaban: float, orang: float) -> FuzzyResult:
    memberships = {
        "suhu": _membership_for_input(suhu, TEMP_MF),
        "kelembaban": _membership_for_input(kelembaban, HUMID_MF),
        "orang": _membership_for_input(orang, OCC_MF),
    }
    firing = _aggregate_firing(memberships)
    crisp = min(max(_defuzzify(firing), 16.0), 30.0)
    return FuzzyResult(crisp, round_firmware(crisp), firing, memberships)
```

Reject non-finite values with a clear `ValueError` before fuzzification. Permit valid values outside membership ranges so the simulator can demonstrate the firmware's zero-membership/default behavior. Do not silently clamp input values.

## Task 3: Implement command-line and interactive output

**Files:**
- Modify: `tools/simulasi_fuzzy.py`

- [ ] **Step 1: Add formatting helpers**

Add formatting that prints, in Indonesian and with stable decimal precision:

```text
=== Simulasi Fuzzy Mamdani ===
Input (sudah terkalibrasi)
  Suhu        : 27.32 °C
  Kelembaban  : 56.23 %
  Jumlah orang: 5.00

Membership input
  Suhu        | Dingin: ... | Nyaman: ... | Panas: ...
  Kelembaban  | Kering: ... | Normal: ... | Lembab: ...
  Hunian      | Sedikit: ... | Sedang: ... | Banyak: ...

Firing strength output
  Rendah: ... | Sedang: ... | Tinggi: ...

Output
  Setpoint crisp   : ... °C
  Setpoint simulasi: ... °C
```

If `--hardware-setpoint` is supplied, append:

```text
  Setpoint hardware : 24 °C
  Perbandingan      : COCOK
```

Use the integer result for comparison and retain the crisp result for the calculation table.

- [ ] **Step 2: Add argparse and interactive input collection**

Implement a `main(argv=None) -> int` with these arguments:

```python
parser.add_argument("--suhu", type=float)
parser.add_argument("--kelembaban", type=float)
parser.add_argument("--orang", type=float)
parser.add_argument("--hardware-setpoint", type=int)
```

When an input argument is missing, prompt for it. For `--hardware-setpoint`, accept a blank interactive response and omit the comparison. If all three inputs are supplied on the command line, do not prompt. Convert invalid interactive text into a short Indonesian error and return exit code `2`; `argparse` handles invalid command-line numeric values. Validate hardware setpoint values in the firmware range 16–30 and return exit code `2` when outside it.

- [ ] **Step 3: Add the executable entry point**

End the module with:

```python
if __name__ == "__main__":
    raise SystemExit(main())
```

Run the command-line smoke test:

```bash
python tools/simulasi_fuzzy.py --suhu 27.32 --kelembaban 56.23 --orang 5 --hardware-setpoint 24
```

Expected: a labeled result containing all three membership groups, three firing strengths, crisp setpoint, integer setpoint, and `COCOK`/`BERBEDA`. The command must not apply the firmware offsets again.

## Task 4: Complete verification and align expected test values

**Files:**
- Modify: `tests/test_simulasi_fuzzy.py`
- Modify: `tools/simulasi_fuzzy.py` only if a test exposes a mismatch with the firmware

- [ ] **Step 1: Install host dependencies**

Run:

```bash
python -m pip install -r tools/requirements.txt
```

Expected: NumPy and scikit-fuzzy are importable with:

```bash
python -c "import numpy, skfuzzy; print(numpy.__version__)"
```

- [ ] **Step 2: Run unit tests and replace the red-case placeholders with measured expected values**

Run:

```bash
python -m unittest discover -s tests -p "test_*.py" -v
```

For the representative case, retain the fixed expected values already specified
in Task 1. The test must assert stable numeric values with an explicit
tolerance; it must not call `compute_fuzzy` to generate its own expected result.

Add these regression tests before declaring completion:

```python
def test_all_zero_input_membership_uses_default(self):
    result = compute_fuzzy(16.0, 20.0, 0.0)
    self.assertEqual(result.setpoint_int, 23)


def test_hardware_comparison_uses_integer_setpoint(self):
    result = compute_fuzzy(27.32, 56.23, 5.0)
    self.assertEqual(result.setpoint_int, int(math.floor(result.crisp_setpoint + 0.5)))
```

If the chosen representative case has nonzero firing, keep its fixed expected values. Use a separate out-of-range case for the default test so the behavior is unambiguous.

- [ ] **Step 3: Run the complete verification set**

Run:

```bash
python -m unittest discover -s tests -p "test_*.py" -v
python tools/simulasi_fuzzy.py --suhu 27.32 --kelembaban 56.23 --orang 5
python tools/simulasi_fuzzy.py --help
```

Expected:

- all unit tests pass;
- the smoke test prints a complete manual comparison table;
- `--help` prints usage without asking for input;
- no PlatformIO source or configuration file changes are present.

- [ ] **Step 4: Check the final diff and document the verification result**

Run:

```bash
git status --short
git diff -- tools/simulasi_fuzzy.py tools/requirements.txt tests/test_simulasi_fuzzy.py
```

Confirm that only the simulator, its requirements, tests, and the approved design/plan documents are involved. Do not commit or alter unrelated worktree changes unless explicitly requested.

## Self-Review

- Spec coverage: manual and CLI modes are covered by Task 3; calibrated inputs and no duplicate offsets by Tasks 2 and 3; scikit-fuzzy membership functions by Task 2; explicit 27-rule inference by Task 2; centroid/default/clamp/rounding by Task 2; manual hardware comparison by Task 3; dependency isolation by Task 1; verification and boundary cases by Task 4.
- Placeholder scan: the only temporary expected values are explicitly identified in Task 1 and must be replaced in Task 4; no implementation TODO is left in the final result.
- Type consistency: `FuzzyResult`, `compute_fuzzy`, `round_firmware`, `trap_membership`, `triangle_membership`, and `OUTPUT_UNIVERSE` are defined in Task 2 and imported by Task 1; Task 3 consumes the same result fields.

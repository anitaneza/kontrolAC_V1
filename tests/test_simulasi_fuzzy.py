import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from simulasi_fuzzy import compute_fuzzy, trap_membership  # noqa: E402


class FuzzySimulationTests(unittest.TestCase):
    def test_representative_hardware_input(self):
        result = compute_fuzzy(27.32, 56.23, 5.0)

        self.assertAlmostEqual(result.firing[0], 0.0, places=6)
        self.assertAlmostEqual(result.firing[1], 0.2266666667, places=6)
        self.assertAlmostEqual(result.firing[2], 0.0, places=6)
        self.assertAlmostEqual(result.crisp_setpoint, 26.3381223, places=5)
        self.assertEqual(result.setpoint_int, 26)

    def test_firmware_boundary_and_no_firing_fallback(self):
        self.assertEqual(trap_membership(16.0, (16.0, 18.0, 22.0, 25.0)), 0.0)

        result = compute_fuzzy(10.0, 10.0, 20.0)
        self.assertEqual(result.firing, (0.0, 0.0, 0.0))
        self.assertEqual(result.crisp_setpoint, 23.0)
        self.assertEqual(result.setpoint_int, 23)


if __name__ == "__main__":
    unittest.main()

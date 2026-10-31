import copy
import importlib.util
import pathlib
import time
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("worker", ROOT / "reparo_cpsat.py")
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)


def request():
    return {"version": 1, "n": 3, "fleet": 1, "horizon": 19, "truck_limit": 16,
            "seconds": 5, "seed": 123, "free": [1, 2],
            "truck_times": [[0, 10, 8], [10, 0, 2], [8, 2, 0]],
            "drone_times": [[None], [8], [20]], "route": [1], "missions": [[2, 1, 0, 20]]}


class ReparoCPTest(unittest.TestCase):
    def test_joint_assignment_required(self):
        result = worker.solve(request())
        self.assertEqual(result["status"], "FEASIBLE")
        self.assertEqual(result["route"], [2])
        self.assertEqual([m[:2] for m in result["missions"]], [[1, 1]])
        self.assertLessEqual(max(m[3] for m in result["missions"]), 19)

    def test_fixed_outside_neighborhood(self):
        data = request()
        data["free"] = []
        self.assertEqual(worker.solve(data)["status"], "INFEASIBLE")

    def test_cumulative_capacity_not_just_workload(self):
        data = {"version": 1, "n": 4, "fleet": 3, "horizon": 4, "truck_limit": 0,
                "seconds": 5, "seed": 4, "free": [], "route": [],
                "missions": [[1, 2, 0, 2], [2, 2, 2, 4], [3, 2, 4, 6]],
                "truck_times": [[0]*4 for _ in range(4)],
                "drone_times": [[None]*3, [None, 2, None], [None, 2, None], [None, 2, None]]}
        # Duas missoes ja nao cabem em paralelo: demanda 4 > frota 3.
        self.assertEqual(worker.solve(data)["status"], "INFEASIBLE")

    def test_alternative_quantities(self):
        data = {"version": 1, "n": 3, "fleet": 2, "horizon": 5, "truck_limit": 0,
                "seconds": 5, "seed": 7, "free": [], "route": [],
                "missions": [[1, 2, 0, 3], [2, 2, 3, 6]],
                "truck_times": [[0]*3 for _ in range(3)],
                "drone_times": [[None]*2, [5, 3], [5, 3]]}
        result = worker.solve(data)
        self.assertEqual(result["status"], "FEASIBLE")
        self.assertEqual([m[1] for m in result["missions"]], [1, 1])

    def test_outside_truck_blocks_preserve_clients(self):
        data = request()
        data["horizon"], data["truck_limit"] = 20, 20
        data["free"] = [2]
        result = worker.solve(data)
        self.assertEqual(result["status"], "FEASIBLE")
        self.assertIn(1, result["route"])

    def test_expired_budget_is_not_infeasible(self):
        self.assertEqual(worker.solve(request(), time.monotonic()-20)["status"], "UNKNOWN")

    def test_invalid_coverage_and_fractional_matrix(self):
        data = request(); data["route"] = [2]
        with self.assertRaises(ValueError): worker.solve(data)
        data = request(); data["truck_times"][0][1] = .5
        with self.assertRaises(ValueError): worker.solve(data)


if __name__ == "__main__":
    unittest.main()

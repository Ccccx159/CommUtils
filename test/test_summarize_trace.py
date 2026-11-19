import importlib.util
import pathlib
import sys
import unittest

spec = importlib.util.spec_from_file_location(
    "summarize_trace", pathlib.Path(__file__).resolve().parents[1] / "scripts/summarize_trace.py")
sys.dont_write_bytecode = True
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class SummarizeTraceTest(unittest.TestCase):
    def test_groups_by_thread_and_name_and_sorts_by_total_cpu(self):
        def event(tid, name, cpu, wall):
            return {"ph": "E", "tid": tid, "name": name,
                    "args": {"cpu_time_us": cpu, "wall_time_us": wall}}
        events = [event(1, "frequent", 10, 20), event(1, "frequent", 10, 30),
                  event(1, "slow", 15, 500), event(2, "frequent", 100, 200),
                  {"ph": "B"}, {"ph": "E", "args": {"unmatched_begin": True}}]
        rows = module.summarize(events)
        self.assertEqual([(row["tid"], row["name"]) for row in rows],
                         [(2, "frequent"), (1, "frequent"), (1, "slow")])
        self.assertEqual(rows[1]["calls"], 2)
        self.assertEqual(rows[1]["cpu_us"], 20)
        self.assertEqual(rows[1]["wall_us"], 50)
        self.assertEqual(module.summarize(events, tid=1), rows[1:])
        self.assertEqual(module.summarize(events, tid=99), [])


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
"""按线程和接口汇总 Tracer 的累计 CPU 时间（包含嵌套调用）。"""

import argparse
import json


def summarize(events, tid=None):
    totals = {}
    for event in events:
        if event.get("ph") != "E" or (tid is not None and event.get("tid") != tid):
            continue
        args = event.get("args", {})
        if "cpu_time_us" not in args or "wall_time_us" not in args:
            continue
        key = (event["tid"], event["name"])
        row = totals.setdefault(key, {"tid": key[0], "name": key[1], "calls": 0,
                                      "cpu_us": 0.0, "wall_us": 0.0})
        row["calls"] += 1
        row["cpu_us"] += args["cpu_time_us"]
        row["wall_us"] += args["wall_time_us"]
    return sorted(totals.values(), key=lambda row: (-row["cpu_us"], row["tid"], row["name"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", help="Tracer JSON 文件")
    parser.add_argument("--tid", type=int, help="仅分析指定 Linux 线程 TID")
    parser.add_argument("--limit", type=int, default=20, help="输出接口数量，默认 20")
    args = parser.parse_args()
    if args.limit <= 0:
        parser.error("--limit 必须大于 0")
    try:
        with open(args.report, encoding="utf-8") as report:
            rows = summarize(json.load(report)["traceEvents"], args.tid)
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        parser.error(str(error))
    print("TID\t调用次数\tCPU(ms)\t墙钟(ms)\tCPU/墙钟(%)\t接口")
    for row in rows[:args.limit]:
        duty = 100 * row["cpu_us"] / row["wall_us"] if row["wall_us"] > 0 else 0
        print(f'{row["tid"]}\t{row["calls"]}\t{row["cpu_us"] / 1000:.3f}\t'
              f'{row["wall_us"] / 1000:.3f}\t{duty:.2f}\t{row["name"]}')
    print("注：CPU 时间包含嵌套调用，父子接口的累计时间不能直接相加。")


if __name__ == "__main__":
    main()

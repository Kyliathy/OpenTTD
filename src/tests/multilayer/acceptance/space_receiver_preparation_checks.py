#!/usr/bin/env python3
"""Earn real receiver stock through bounded original mining/handling applications.

@file space_receiver_preparation_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group31C.2.
Each child resumes its predecessor's actual native save and offers at most4000
ordinary opportunities. No packet surgery, field refill, elapsed-clock edit or
free service is part of preparation. Progress reports/logs stay in logs/space-fixes;
native checkpoints, configuration and captures stay in test-cache/space-fixes.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]


def main():
    """Resume separate bounded native preparation stages until real refusal.

    @return Zero after the genuine saved NoReceiver wait, two if the declared
        stage budget expires, or the original failing child's nonzero status.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, help="Original role15 module-world input or an actual earlier preparation save")
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--run", required=True, help="Fresh lowercase artifact prefix")
    parser.add_argument("--max-stages", type=int, default=40, help="Bounded number of separate4000-opportunity applications")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9-]+", args.run) or not 1 <= args.max_stages <= 60:
        raise ValueError("select a lowercase run identity and1..60 bounded stages")
    source = (ROOT / args.source).resolve()
    report_path = ROOT / "logs/space-fixes" / (args.run + "-progress.json")
    if report_path.exists():
        raise ValueError("run identity already owns a preparation report")
    report = {"_documentation": __doc__, "complete": False, "stages": []}
    # Only actual saved native owners feed the following process. The loop
    # cannot assign inventory or infer successful stock from elapsed time.
    for stage in range(1, args.max_stages + 1):
        run = args.run + f"-{stage:02d}"
        command = [sys.executable, "-B", str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--source", source.relative_to(ROOT).as_posix(), "--configuration", args.configuration,
                   "--mining-controls", "receiver-prepare", "--run", run]
        with (ROOT / "logs/space-fixes" / (run + "-driver.log")).open("w", encoding="utf-8") as log:
            child = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=118)
        if child.returncode:
            report["failure"] = {"run": run, "exit_code": child.returncode}
            report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
            return child.returncode
        native = (ROOT / "logs/space-fixes" / (run + ".log")).read_text(encoding="utf-8")
        match = re.search(r"RECEIVER_PREPARATION stock_before=(\d+) stock_after=(\d+) capacity=(\d+) actual_holding=(\d+) ordinary_opportunities=(\d+) allowance=(\d+) complete=(true|false)", native)
        if match is None:
            raise ValueError("native child supplied no actual preparation receipt")
        values = tuple(int(value) for value in match.groups()[:6])
        complete = match.group(7) == "true"
        report["stages"].append(dict(run=run, stock_before=values[0], stock_after=values[1], capacity=values[2],
                                     holding=values[3], opportunities=values[4], allowance=values[5], complete=complete))
        source = ROOT / "test-cache/space-fixes" / run / ("policy-no-receiver.sav" if complete else "policy-receiver-prepared.sav")
        report["last_save"] = source.relative_to(ROOT).as_posix()
        report["complete"] = complete
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(report["stages"][-1]), flush=True)
        if complete:
            return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

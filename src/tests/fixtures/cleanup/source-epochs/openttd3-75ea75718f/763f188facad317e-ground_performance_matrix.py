"""Execute explicit ground baseline sizes sequentially with resumable exact observation selection.

@file ground_performance_matrix.py
@see r2mlvs16-integrated-acceptance-and-balance.md Group21C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
One native child at a time;bounded resource refusals remain retained failures.
"""

import argparse
import json

from ground_network import ROOT
from construction_provider_return import identity
from ground_network_report import verify_reference
from ground_performance import PerformanceRefusal, PROTOCOL, run_one


def main():
    """Continue only explicit exact prior successes;never infer latest files or repeat a failed case as a pass."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--axis", type=int, action="append", required=True)
    parser.add_argument("--prior", action="append", default=[])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    protocol = json.loads((ROOT / PROTOCOL).read_text(encoding="utf-8"))
    rows, done = [], set()
    for name in args.prior:
        ref = identity(ROOT / name)
        actual = json.loads(verify_reference(ref).read_text(encoding="utf-8"))
        assert actual["status"] == "observed" and actual["protocol"] == identity(ROOT / PROTOCOL)
        for field in ("controller", "binary", "config", "startup"):
            verify_reference(actual[field])
        for reference in actual["sources"]:
            verify_reference(reference)
        key = (actual["configuration"], actual["profile"], actual["axis"])
        assert key not in done
        done.add(key)
        rows.append({"configuration": key[0], "profile": key[1], "axis": key[2], "status": "observed", "report": ref})
    index = {"_documentation": "r2mlvs16-integrated-acceptance-and-balance.md21C;mutable sequential selection,not acceptance",
             "controller": identity(__file__), "protocol": identity(ROOT / PROTOCOL), "records": rows, "complete": False}
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    for axis in args.axis:
        for cfg in protocol["configurations"]:
            for profile in protocol["profiles"]:
                if (cfg, profile, axis) in done:
                    continue
                try:
                    report = run_one(cfg, profile, axis)
                    status = "observed"
                except PerformanceRefusal as error:
                    report, status = error.report, "refused"
                    failure = json.loads(report.read_text(encoding="utf-8"))["failure"]
                    if not failure.startswith(("TimeoutError:", "RuntimeError: native workload crossed")):
                        raise
                rows.append({"configuration": cfg, "profile": profile, "axis": axis, "status": status, "report": identity(report)})
                target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    index["complete"] = True
    target.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"selection": identity(target), "observed": sum(r["status"] == "observed" for r in rows),
                      "refused": sum(r["status"] == "refused" for r in rows)}), flush=True)


if __name__ == "__main__":
    main()

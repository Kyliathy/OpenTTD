"""Select exact current-run graphics evidence independently of diagnostic logs.

@file evidence.py Public sprite identities remain with their existing media owners.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group16.
Schema1 relocation is explicitly historical; current recipe selections use schema2.
"""

import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
# Migration input for source converters until Group17; never a current default.
CACHE = ROOT / "logs/proofs/mlm/native-art"
DEFAULT_MANIFEST = None
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from artifact_paths import RunContext, contained, identifier


def identity(path, root=ROOT):
    """Bind a contained artifact to exact bytes.

    @param path Existing source or generated artifact.
    @param root Explicit source checkout, including a disposable test checkout.
    @return Repository-relative path and SHA256.
    """
    path = contained(path, root)
    return {"path": path.relative_to(root).as_posix(), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def selection_document(context, configuration):
    """Describe current outputs without reserving a second recipe invocation.

    @param context Explicit RunContext supplied by the selected recipe or launcher.
    @param configuration Exact native Debug or RelWithDebInfo configuration.
    @return Empty schema2 selection to populate only with verified current products.
    """
    if configuration not in ("Debug", "RelWithDebInfo"):
        raise ValueError("evidence selection requires an explicit native configuration")
    outputs = {"packed": context.data / "packed-manifest.json",
               "runtime": context.reports / "runtime-manifest.json",
               "sound": context.reports / "sound-runtime.json",
               "gallery": context.reports / "m1-runtime-gallery.json"}
    return dict(_documentation="Owner: r2fc-file-cleanup3.md16; generated technical evidence, no human acceptance",
                schema_version=2, provenance="native_run", feature=context.feature, scenario=context.scenario,
                run_id=context.run_id, configuration=configuration, reports={}, evidence={}, diagnostics={},
                outputs={name: path.relative_to(context.root).as_posix() for name, path in outputs.items()})


def bind_capture(context, report_path, configuration):
    """Adapt an actual current capture into the selected terminal schema.

    @param context Explicit recipe invocation receiving the selected capture.
    @param report_path Exact existing semantic report from its current native wrapper.
    @param configuration Selected native configuration required for its process.
    @return Schema2 capture with exact process, transcript and writer identities.
    """
    root = context.root
    report_path = contained(report_path, root / "mlm/reviews")
    parts = report_path.relative_to(root / "mlm/reviews").parts
    # The adapter cannot re-label an older semantic report by copying its
    # metadata into the current run; its physical owner must match as well.
    if len(parts) != 5 or tuple(parts[:3]) != (context.feature, context.scenario, context.run_id) or parts[3] != "reports":
        raise ValueError("capture report belongs to another explicit invocation")
    report = json.loads(report_path.read_text(encoding="utf-8"))
    expected = (context.run_id, configuration)
    # This is a storage adapter for actual current outputs. Historical imports
    # and previously adapted reports require explicit inspection/regeneration.
    if tuple(report.get(key) for key in ("run_id", "configuration")) != expected or report.get("provenance", {}).get("kind") == "historical_relocation" or "terminal" in report:
        raise ValueError("capture adapter requires an unambiguous current native report")
    processes, terminals = [], {}
    # Select by exact native frame/container provenance. Optional diagnostic
    # copies may already be absent; only retained process transcripts are read.
    for path in report_path.parent.glob("native-*.json"):
        process = json.loads(path.read_text(encoding="utf-8"))
        # Semantic capture/scene reports may share the native- prefix. Only
        # actual measured process records can own frame and transcript artifacts.
        if not isinstance(process.get("artifacts"), list):
            continue
        actual = tuple(process.get(key) for key in ("producer", "run_id", "configuration"))
        if actual != (report["producer"], *expected):
            continue
        process_identity = identity(path, root)
        owns_frame = False
        # A current semantic report can select only images actually produced
        # by this child, including unchanged PPM-to-PNG container conversions.
        for frame in report["frames"]:
            reference = {key: frame[key] for key in ("path", "sha256")}
            if reference in process["artifacts"]:
                owns_frame = True
                continue
            image = contained(root / frame["path"], root / "mlm/reviews")
            ledger = image.parent.parent / "reports/capture-conversions.jsonl"
            if ledger.is_file():
                rows = [json.loads(line) for line in ledger.read_text(encoding="utf-8").splitlines()]
                owns_frame = owns_frame or any({key: row[key] for key in ("path", "sha256")} == reference
                    and row.get("pixels_unchanged") is True and process_identity in row["native_reports"]
                    and row["source_container"] in process["artifacts"] for row in rows)
        if owns_frame:
            processes.append(process_identity)
            for reference in process.get("transcripts", []):
                terminals[reference["path"]] = reference
    if len(terminals) != 1:
        raise ValueError("capture requires one exact retained native transcript; regenerate its producer")
    catalogue = json.loads((root / "mlm/review-producers.json").read_text(encoding="utf-8"))
    producer = next(row for row in catalogue["producers"] if row["producer"] == report["producer"])
    report.pop("log", None)
    report.update(schema_version=2, original_report=identity(report_path, root),
                  terminal=next(iter(terminals.values())), native_processes=processes,
                  producer_source=identity(root / report["producer"], root),
                  native_writers=[identity(root / path, root) for path in producer.get("native_writers", [])])
    # Converted PNGs retain the exact native container/process witness even
    # after the temporary PPM is disposed of by the original capture wrapper.
    for frame in report["frames"]:
        image = contained(root / frame["path"], root / "mlm/reviews")
        conversions = image.parent.parent / "reports/capture-conversions.jsonl"
        if conversions.is_file():
            frame["conversion"] = identity(conversions, root)
    return report


class EvidenceSet:
    """One explicit recipe invocation, or explicitly requested historical inspection.

    @see r2fc-file-cleanup3.md16; no implicit old-cache/latest-run fallback.
    """

    def __init__(self, path, context=None, root=ROOT, allow_historical=False):
        """Validate schema and ownership before exposing selected products.

        @param path Exact manifest; None refuses implicit cache discovery.
        @param context Optional recipe RunContext that must match the selection.
        @param root Explicit checkout containing all admitted artifacts.
        @param allow_historical Explicit read-only access to relocated schema1.
        @return None; ambiguous schemas fail with bounded regeneration guidance.
        """
        if path is None:
            raise ValueError("select an explicit schema2 evidence manifest for the recipe run")
        self.root = Path(root).resolve()
        self.path = contained(self.root / path, self.root)
        if not self.path.is_file():
            raise ValueError("selected evidence is absent; regenerate the selected recipe prerequisites")
        self.document = json.loads(self.path.read_text(encoding="utf-8"))
        self.historical = self.document.get("schema_version") == 1
        if self.historical:
            # Relocating yesterday's capture cannot turn it into current proof.
            if not allow_historical or self.document.get("provenance") != "historical_relocation" or context is not None or "evidence" in self.document:
                raise ValueError("schema1 evidence is historical only; regenerate schema2 for current verification")
            contained(self.path, self.root / "logs/proofs/mlm/native-art")
            self.context = None
            return
        # A mixed migration must not silently choose one of two terminal owners.
        if self.document.get("schema_version") != 2 or "logs" in self.document or self.document.get("provenance") != "native_run":
            raise ValueError("unsupported/mixed evidence schema; regenerate an explicit schema2 selection")
        parts = self.path.relative_to(self.root / "mlm/reviews").parts
        # Selection files must be reports in the declared run, so changing the
        # manifest location cannot confer ownership of another output tree.
        if len(parts) != 5 or parts[3] != "reports":
            raise ValueError("selection must belong to its invocation's reports owner")
        self.context = RunContext(self.root, identifier(parts[0]), identifier(parts[1]), identifier(parts[2]))
        actual = (self.root, *parts[:3])
        # A supplied recipe context is stronger authority than metadata read
        # from disk; both must agree before any current output is exposed.
        if context is not None and (context.root.resolve(), context.feature, context.scenario, context.run_id) != actual:
            raise ValueError("evidence selection belongs to another recipe invocation")
        if tuple(self.document.get(key) for key in ("feature", "scenario", "run_id")) != tuple(parts[:3]):
            raise ValueError("selection routing disagrees with its explicit invocation")
        if self.document.get("configuration") not in ("Debug", "RelWithDebInfo"):
            raise ValueError("selection requires an explicit native configuration")
        # No declared output may redirect a recipe into sources or another run.
        for name in self.document["outputs"]:
            self.output(name)

    def generated(self, path, role, own_feature=False):
        """Check exact current generated ownership without reading a file.

        @param path Repository-relative selected artifact.
        @param role Required images, reports, evidence or data owner.
        @param own_feature Whether the selection feature/scenario must also match.
        @return Canonical contained artifact path.
        """
        path = contained(self.root / path, self.root / "mlm/reviews")
        parts = path.relative_to(self.root / "mlm/reviews").parts
        # Matching basenames cannot borrow another run or bypass role isolation.
        if len(parts) != 5 or parts[2] != self.context.run_id or parts[3] != role:
            raise ValueError("selected artifact has the wrong invocation or generated role")
        if own_feature and tuple(parts[:2]) != (self.context.feature, self.context.scenario):
            raise ValueError("selected output belongs to another feature/scenario")
        return path

    def checked(self, record):
        """Verify exact bytes for a selected contained identity.

        @param record Repository-relative path and SHA256.
        @return Canonical unchanged existing artifact.
        """
        path = contained(self.root / record["path"], self.root)
        if identity(path, self.root) != {key: record[key] for key in ("path", "sha256")}:
            raise AssertionError("selected evidence changed: " + record["path"])
        return path

    def selected(self, section, name):
        """Resolve a required role without diagnostic-log fallback.

        @param section Required reports/evidence collection, or historical logs.
        @param name Semantic identity within the declared collection.
        @return Existing hash-verified path with its required generated owner.
        """
        record = self.document[section][name]
        if self.historical:
            contained(self.root / record["path"], self.root / "logs/proofs/mlm/native-art")
        else:
            if section not in ("reports", "evidence"):
                raise ValueError("required roles use reports/evidence; diagnostics are optional")
            self.generated(record["path"], section)
        return self.checked(record)

    def terminal(self, name):
        """Resolve required process evidence independently of optional diagnostics.

        @param name Declared native check or capture obligation.
        @return Hash-verified repository-relative transcript path.
        """
        section = "logs" if self.historical else "evidence"
        return self.selected(section, name).relative_to(self.root).as_posix()

    def capture_terminal(self, report):
        """Verify the report-to-terminal edge through the selected collection.

        @param report Current schema2 capture or explicitly historical capture.
        @return Required transcript path; missing or undeclared bytes refuse.
        """
        reference = {"path": report["log"]} if self.historical else report["terminal"]
        section = "logs" if self.historical else "evidence"
        matches = [name for name, row in self.document[section].items() if row["path"] == reference["path"]]
        if not matches:
            raise AssertionError("capture transcript was not declared by the selected evidence manifest")
        path = self.selected(section, matches[0])
        # Historical records retain their original log spelling. Current
        # reports must additionally bind the exact required terminal identity.
        if not self.historical and identity(path, self.root) != reference:
            raise AssertionError("report-to-terminal identity differs from selected evidence")
        return path.relative_to(self.root).as_posix()

    def report(self, name):
        """Read and validate selected frame, writer, source and process provenance.

        @param name Semantic capture identity in the reports collection.
        @return Parsed capture, retaining its original historical/current status.
        """
        report = json.loads(self.selected("reports", name).read_text(encoding="utf-8"))
        self.validate_capture(report)
        return report

    def output(self, name):
        """Resolve a generated destination inside the recipe invocation.

        @param name Declared output role, such as runtime, packed, sound or gallery.
        @return Canonical generated path; historical selections cannot publish.
        """
        if self.historical:
            raise ValueError("historical relocation cannot publish current outputs; regenerate schema2")
        role = "data" if name == "packed" else "reports"
        return self.generated(self.document["outputs"][name], role, own_feature=True)

    def validate_capture(self, report):
        """Refuse stale pixels, source changes, foreign writers and old relocations.

        @param report Actual semantic native capture with current byte identities.
        @return None; historical inspection never promotes a record to current proof.
        """
        historical = report.get("provenance", {}).get("kind") == "historical_relocation"
        if historical != self.historical:
            raise ValueError("historical and current native evidence cannot be mixed")
        if not historical:
            # One current report has one terminal schema and one selected native
            # configuration; old log aliases would obscure its actual read-set.
            if report.get("schema_version") != 2 or "log" in report or report.get("run_id") != self.context.run_id or report.get("configuration") != self.document["configuration"]:
                raise ValueError("unsupported capture schema/configuration/run; regenerate schema2")
            catalogue = json.loads((self.root / "mlm/review-producers.json").read_text(encoding="utf-8"))
            producer = next(row for row in catalogue["producers"] if row["producer"] == report["producer"])
            assignments = {(row["feature"], row["scenario"]) for row in producer["features"]}
            if (report["feature"], report["scenario"]) not in assignments:
                raise AssertionError("capture feature/scenario is not assigned to its producer")
            writers = producer.get("native_writers", [])
            # An empty or substituted writer list cannot certify native art;
            # only the catalogue's implemented owners can supply this capture.
            if not writers or [row["path"] for row in report["native_writers"]] != writers:
                raise AssertionError("capture native writer differs from the producer catalogue")
            # Editing the wrapper or any declared native writer requires a new
            # capture; retaining an old terminal cannot certify changed source.
            for reference in [report["producer_source"], *report["native_writers"]]:
                self.checked(reference)
            if report["producer_source"]["path"] != report["producer"]:
                raise AssertionError("capture wrapper identity differs from its producer")
            processes = self.validate_processes(report, producer)
            if not report["frames"]:
                raise AssertionError("capture has no actual frames")
        # Shared galleries may combine only exact images in declared features.
        for frame in report["frames"]:
            path = self.checked(frame)
            if historical:
                contained(path, self.root / "logs/proofs/mlm/native-art")
            else:
                self.generated(frame["path"], "images")
                parts = path.relative_to(self.root / "mlm/reviews").parts
                if tuple(parts[:2]) not in assignments or tuple(frame.get(key) for key in ("feature", "scenario", "run_id")) != tuple(parts[:3]):
                    raise AssertionError("frame routing/producer metadata disagrees")
                self.validate_frame_origin(frame, processes)
        self.checked({"path": report["source"], "sha256": report["source_sha256"]})
        self.capture_terminal(report)

    def validate_processes(self, report, producer):
        """Bind the terminal to successful current native process records.

        @param report Capture containing exact process and terminal identities.
        @param producer Canonical catalogue assignment and native test tags.
        @return Checked native process records keyed by their exact report path.
        """
        if not report.get("native_processes"):
            raise AssertionError("capture has no native process provenance")
        terminal_seen = False
        processes = {}
        # Every contributing process must be an actual successful child of the
        # selected producer/configuration, rather than a similarly named report.
        for reference in report["native_processes"]:
            self.generated(reference["path"], "reports")
            process = json.loads(self.checked(reference).read_text(encoding="utf-8"))
            processes[reference["path"]] = process
            expected = (report["producer"], report["run_id"], report["configuration"], report["feature"], report["scenario"])
            if tuple(process.get(key) for key in ("producer", "run_id", "configuration", "feature", "scenario")) != expected or process.get("exit_status") != 0:
                raise AssertionError("capture process belongs to a failed/foreign producer or configuration")
            arguments = producer.get("native_arguments")
            # Actual ordinary-game captures use their real startup arguments;
            # Catch fixtures keep their actual declared test tag requirement.
            if arguments is not None:
                if not arguments or not all(argument in process["argv"] for argument in arguments):
                    raise AssertionError("ordinary capture did not execute its declared native arguments")
            elif not any(tag in process["argv"] for tag in producer.get("test_tags", [])):
                raise AssertionError("capture process did not execute a declared native writer tag")
            executable = self.checked(process["executable"])
            if Path(process["argv"][0]).resolve() != executable:
                raise AssertionError("native process executable differs from its actual command")
            if report["terminal"] in process.get("transcripts", []):
                terminal_seen = True
                self.generated(report["terminal"]["path"], "evidence")
                if report["terminal"] not in process["artifacts"]:
                    raise AssertionError("native transcript is not an actual process artifact")
        if not terminal_seen:
            raise AssertionError("required terminal has no matching actual native process")
        return processes

    def validate_frame_origin(self, frame, processes):
        """Require native output or exact unchanged native-container conversion.

        @param frame Selected PNG and optional conversion-ledger identity.
        @param processes Already checked current native process records.
        @return None; a generated image without native writer provenance refuses.
        """
        reference = {key: frame[key] for key in ("path", "sha256")}
        if any(reference in row["artifacts"] for row in processes.values()):
            return
        ledger = frame.get("conversion")
        if ledger is None:
            raise AssertionError("selected frame has no native output/conversion witness")
        self.generated(ledger["path"], "reports")
        rows = [json.loads(line) for line in self.checked(ledger).read_text(encoding="utf-8").splitlines()]
        matches = [row for row in rows if {key: row[key] for key in ("path", "sha256")} == reference]
        if len(matches) != 1:
            raise AssertionError("selected PNG has an ambiguous/missing conversion witness")
        conversion = matches[0]
        if conversion.get("pixels_unchanged") is not True:
            raise AssertionError("conversion did not preserve actual native pixels")
        # The conversion ledger must identify a selected native child and the
        # exact container that child produced; a source-only PNG cannot pass.
        for parent in conversion["native_reports"]:
            path = parent["path"]
            if path in processes and identity(self.root / path, self.root) == parent and conversion["source_container"] in processes[path]["artifacts"]:
                return
        raise AssertionError("PNG conversion has no selected native writer/container")

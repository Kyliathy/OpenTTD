"""Shared paths for multi-layer test inputs, runtime homes and review evidence.

@file artifact_paths.py Development-only artifact boundary; no simulation ownership.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md Groups 3-10.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups9-10.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""

from dataclasses import dataclass
from functools import lru_cache
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import uuid


def repository_root(start=None):
    """Find the source checkout independently of caller working directory.

    @param start Optional file/directory from which to search; defaults to this module.
    @return Canonical source root containing CMakeLists.txt and src/.
    @throws ValueError When no source checkout encloses the search location.
    """
    origin = Path(start or __file__).resolve()
    if origin.is_file():
        origin = origin.parent
    # Build and runtime directories sit below the source root. Stop at the first
    # actual source checkout rather than treating an arbitrary parent as owned.
    for candidate in (origin, *origin.parents):
        if (candidate / "CMakeLists.txt").is_file() and (candidate / "src").is_dir():
            return candidate
    raise ValueError(f"no source checkout above {origin}")


def identifier(value):
    """Validate one caller-declared routing component, never a filesystem path.

    @param value Feature, scenario, profile, process or invocation identifier.
    @return Unchanged lowercase identifier.
    @throws ValueError On empty names, traversal, drive names or separators.
    """
    if not re.fullmatch(r"[a-z0-9][a-z0-9_-]*", value):
        raise ValueError(f"invalid artifact identifier: {value!r}")
    return value


def contained(path, parent):
    """Reject output escaping its owner through traversal or existing links.

    @param path Candidate existing or future path.
    @param parent Authorized root; it must itself remain in the checkout.
    @return Canonical candidate path strictly beneath the root.
    @throws ValueError When canonical containment fails.
    """
    owner = Path(parent).resolve()
    candidate = Path(path).resolve()
    # Equality cannot identify an output file, and following a junction must
    # not grant the test permission to overwrite another workspace or fixture.
    if candidate == owner or not candidate.is_relative_to(owner):
        raise ValueError(f"artifact path escapes {owner}: {candidate}")
    return candidate


def owned_root(root, relative):
    """Resolve a namespace while refusing a redirected directory outside source.

    @param root Canonical checkout root.
    @param relative Repository-relative namespace owned by this subsystem.
    @return Namespace path, with all existing parent links resolved.
    """
    return contained(Path(root) / relative, root)


@dataclass(frozen=True)
class RunContext:
    """One feature view of an invocation and its process-local native data home.

    @param root Canonical source checkout.
    @param feature Player-facing feature explicitly assigned by the producer.
    @param scenario Named case within that feature.
    @param run_id Reserved invocation identifier shared with native children.
    @param profile Native launch profile, independent of captured feature.
    @param process Separate native process role, such as main, host or client.
    """

    root: Path
    feature: str
    scenario: str
    run_id: str
    profile: str = "native"
    process: str = "main"

    @property
    def directory(self):
        """Return this feature/scenario's directory under the reserved run.

        @return Canonical review directory; never a source/fixture path.
        """
        owner = owned_root(self.root, "mlm/reviews")
        return contained(owner / self.feature / self.scenario / self.run_id, owner)

    @property
    def runtime_dir(self):
        """Return the process data home shared by this invocation's features.

        @return Canonical native data directory under mlm/runtime/.
        """
        owner = owned_root(self.root, "mlm/runtime")
        return contained(owner / self.profile / self.run_id / self.process, owner)

    @property
    def config_path(self):
        """Return the default materialized native-art configuration path.

        @return Writable runtime copy; the source template is never returned.
        """
        return self.runtime_dir / "native-art.cfg"

    @property
    def images(self):
        """Return the capture directory for this declared feature/scenario.

        @return Canonical images directory.
        """
        return self.directory / "images"

    @property
    def saves(self):
        """Return the directory for newly generated native checkpoints.

        @return Canonical saves directory, separate from immutable fixtures.
        """
        return self.directory / "saves"

    @property
    def data(self):
        """Return generated selected payloads, distinct from immutable fixture inputs.

        @return This feature/scenario/invocation's generated data directory.
        """
        return self.directory / "data"

    @property
    def evidence(self):
        """Return required current-run transcripts and other non-frame observations.

        @return This feature/scenario/invocation's disposable evidence directory.
        """
        return self.directory / "evidence"

    @property
    def reports(self):
        """Return the directory for this feature's machine-readable evidence.

        @return Canonical reports directory.
        """
        return self.directory / "reports"

    @property
    def logs(self):
        """Return the log directory associated with this feature/run.

        @return Canonical path under repository logs/.
        """
        owner = owned_root(self.root, "logs")
        return contained(owner / self.feature / self.scenario / self.run_id, owner)

    def for_feature(self, feature, scenario):
        """Select another feature within the same reserved invocation.

        @param feature Explicit player-facing feature, never inferred from filenames.
        @param scenario Explicit case owned by that feature.
        @return Context sharing the invocation and process home.
        """
        return _materialize(RunContext(self.root, identifier(feature), identifier(scenario), self.run_id, self.profile, self.process))

    def for_process(self, process):
        """Isolate a native peer while retaining shared invocation provenance.

        @param process Distinct native role such as host or client.
        @return Context with a separate writable native data home.
        """
        return _materialize(RunContext(self.root, self.feature, self.scenario, self.run_id, self.profile, identifier(process)))

    def output(self, kind, name):
        """Resolve a newly written artifact in this feature's declared namespace.

        @param kind One of images, saves, data, reports, evidence, scratch or logs.
        @param name Artifact basename; callers may not redirect output directories.
        @return Canonical output path whose parent already exists.
        """
        if kind not in ("images", "saves", "data", "reports", "evidence", "scratch", "logs"):
            raise ValueError(f"unknown artifact kind: {kind}")
        # A producer chooses an identity inside its case, not a new destination
        # that could replace immutable fixture inputs or another feature's data.
        if Path(name).name != name or name in ("", ".", ".."):
            raise ValueError("artifact name must be a basename")
        folder = self.logs if kind == "logs" else self.directory / kind
        return contained(folder / name, folder)

    def environment(self, base=None):
        """Supply native children with the same validated invocation contract.

        @param base Optional source environment; defaults to the current process.
        @return New environment dictionary without mutating the parent process.
        """
        result = dict(os.environ if base is None else base)
        result.update(OTTD_TEST_REPOSITORY=str(self.root), OTTD_TEST_RUN_ID=self.run_id,
                      OTTD_TEST_PROFILE=self.profile, OTTD_TEST_PROCESS=self.process,
                      OTTD_TEST_FEATURE=self.feature, OTTD_TEST_SCENARIO=self.scenario,
                      OTTD_TEST_RUNTIME_DIR=str(self.runtime_dir))
        return result

    def capture_paths(self, pattern="*.ppm"):
        """Collect only this invocation's captures across explicitly routed features.

        @param pattern Basename glob used by the consuming capture wrapper.
        @return Sorted concrete capture paths; never results from another run.
        """
        if "/" in pattern or "\\" in pattern:
            raise ValueError("capture selection must be a basename pattern")
        owner = owned_root(self.root, "mlm/reviews")
        paths = []
        # Mixed native tests emit into several feature/scenario directories.
        # The reserved invocation component, not latest-file discovery, binds
        # those outputs to the wrapper collecting them.
        for path in owner.glob(f"*/*/{self.run_id}/images/{pattern}"):
            actual = contained(path, owner)
            if actual.is_file():
                paths.append(actual)
        return sorted(paths)

    def prepare_runtime(self, template="native-art.cfg"):
        """Materialize config and required repository distribution for native use.

        @param template Existing build configuration basename, or the native-art fixture.
        @return Path to a writable config in this process's isolated data home.
        """
        if Path(template).name != template:
            raise ValueError("runtime template must be a basename")
        self.runtime_dir.mkdir(parents=True, exist_ok=True)
        source = self.root / "src/tests/fixtures/config/multilayer" / template
        if not source.is_file():
            source = self.root / "build" / template
        target = contained(self.runtime_dir / template, self.runtime_dir)
        if not target.exists():
            shutil.copyfile(source, target)
        # Copy only repository-owned distribution inputs. Per-process copies
        # prevent startup/config/autosave writes from modifying the shared build
        # or a peer; existing runtime scripts must survive repeated preparation.
        for name in ("baseset", "newgrf", "lang", "scripts"):
            origin = self.root / "build" / name
            destination = self.runtime_dir / name
            if origin.is_dir() and not destination.exists():
                shutil.copytree(origin, destination)
        return target

    def write_report(self, name, producer, artifacts, inputs=(), configuration="unknown", native_writer=None):
        """Bind output evidence to its declared producer and exact file identities.

        @param name Report basename inside this feature's reports directory.
        @param producer Repository-relative executed wrapper/test entry point.
        @param artifacts Iterable of paths produced by this invocation.
        @param inputs Iterable of explicitly selected immutable/prior-run inputs.
        @param configuration Actual native build configuration, or unknown.
        @param native_writer Optional exact native function/test producing captures.
        @return Written report path; provenance does not imply user acceptance.
        """
        def identity(path):
            """Bind one concrete file to a repository-relative identity.

            @param path Existing input or produced artifact.
            @return Path/hash record with no machine-specific absolute prefix.
            """
            resolved = contained(path, self.root)
            return {"path": resolved.relative_to(self.root).as_posix(), "sha256": hashlib.sha256(resolved.read_bytes()).hexdigest()}
        records = []
        # Reports may combine this invocation's features, but must not silently
        # adopt a similarly named frame from another historical run.
        for path in artifacts:
            resolved = contained(path, owned_root(self.root, "mlm/reviews"))
            parts = resolved.relative_to(self.root / "mlm/reviews").parts
            if len(parts) < 5 or parts[2] != self.run_id:
                raise ValueError("artifact belongs to another invocation")
            if parts[3] not in ("images", "saves", "data", "reports", "evidence", "scratch"):
                raise ValueError("artifact has no declared generated role")
            # Broader evidence roles do not authorize moving native/derived
            # frames out of their image owner or weakening frame provenance.
            if resolved.suffix.lower() in (".png", ".ppm", ".bmp", ".pcx") and parts[3] != "images":
                raise ValueError("frame artifacts require the images owner")
            records.append(identity(resolved))
        document = {"_documentation": "Owner: docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup.md; observed artifact identities",
                    "producer": producer, "native_writer": native_writer, "feature": self.feature,
                    "scenario": self.scenario, "run_id": self.run_id, "configuration": configuration,
                    "inputs": [identity(path) for path in inputs], "artifacts": records}
        output = self.output("reports", name)
        output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
        return output


def _materialize(context):
    """Create only the validated output directories belonging to a context.

    @param context Invocation/feature/process identity already validated by its owner.
    @return The same immutable context after its output parents exist.
    """
    # Materialize only generated roles; adding payload/transcript directories
    # never creates a fixture or changes how immutable source inputs are chosen.
    for kind in ("images", "saves", "data", "reports", "evidence", "scratch"):
        contained(context.directory / kind, context.root).mkdir(parents=True, exist_ok=True)
    context.logs.mkdir(parents=True, exist_ok=True)
    context.runtime_dir.mkdir(parents=True, exist_ok=True)
    return context


def create_run_context(feature, scenario, run_id=None, root=None, profile="native", process="main"):
    """Reserve a fresh invocation or join one explicitly inherited from a parent.

    @param feature Explicit feature chosen by the caller's producer contract.
    @param scenario Explicit case beneath that feature.
    @param run_id Optional new identifier; an already reserved value is rejected.
    @param root Optional source root for isolated verification workspaces.
    @param profile Native launch profile used for runtime separation.
    @param process Process role; host and client must use distinct values.
    @return New feature context whose invocation cannot collide with prior runs.
    """
    source = repository_root(root) if root is not None else repository_root()
    feature, scenario = identifier(feature), identifier(scenario)
    inherited = os.environ.get("OTTD_TEST_RUN_ID") if run_id is None else None
    name = identifier(run_id or inherited or datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S-") + uuid.uuid4().hex[:10])
    profile = identifier(os.environ.get("OTTD_TEST_PROFILE", profile) if inherited else profile)
    process = identifier(os.environ.get("OTTD_TEST_PROCESS", process) if inherited else process)
    marker = contained(owned_root(source, "mlm/runtime") / ".runs" / name, source)
    marker.parent.mkdir(parents=True, exist_ok=True)
    # Only an explicitly inherited invocation may join an existing reservation.
    # Supplying a duplicate ID at the public entry point must not overwrite it.
    if inherited and marker.is_dir():
        pass
    else:
        marker.mkdir()
    return _materialize(RunContext(source, feature, scenario, name, profile, process))


@lru_cache(maxsize=4)
def _input_catalogue(path, signature):
    """Read one immutable catalogue epoch without reparsing it for every save.

    @param path Explicit multilayer-inputs.json path inside this checkout.
    @param signature File size/mtime/inode tuple, invalidating replaced catalogues.
    @return Parsed stable-key table; actual selected file bytes are never cached.
    """
    document = json.loads(path.read_text(encoding="utf-8"))
    after = path.stat()
    if (after.st_size, after.st_mtime_ns, after.st_ino) != signature:
        raise ValueError("input catalogue changed while loading")
    # The established schema1 identities and schema2 retained-first migration
    # share one owner; unknown formats cannot silently fall back to old files.
    if document.get("schema_version") not in (1, 2) or not isinstance(document.get("inputs"), dict):
        raise ValueError("unsupported input catalogue schema")
    return document["inputs"]


def resolve_input_path(path, root=None):
    """Resolve a declared existing input without granting permission to write it.

    @param path Existing path or stable fixture key recorded in the input manifest.
    @param root Optional checkout root for isolated verification.
    @return Hash-verified indexed fixture or explicit unindexed prior-run input.
    @throws ValueError For missing, ambiguous or out-of-owner inputs.
    """
    source = repository_root(root) if root is not None else repository_root()
    candidate = Path(path)
    if not candidate.is_absolute():
        candidate = source / candidate
    candidate = contained(candidate, source)
    manifest = source / "src/tests/fixtures/multilayer-inputs.json"
    expected = None
    # A declared stable key always chooses its retained identity, even while an
    # old file still exists. Unindexed explicit prior-run paths stay direct;
    # no filesystem scan or latest-run fallback can substitute another input.
    if manifest.is_file():
        status = manifest.stat()
        entries = _input_catalogue(manifest, (status.st_size, status.st_mtime_ns, status.st_ino))
        key = candidate.relative_to(source).as_posix()
        if key in entries:
            reference = entries[key]
            expected = reference.get("sha256")
            if not isinstance(expected, str) or not re.fullmatch("[0-9a-f]{64}", expected):
                raise ValueError("indexed input requires an exact SHA256")
            candidate = contained(source / reference["path"], source)
    owners = ("src/tests/fixtures", "savegames/retained", "mlm/reviews", "regression", "build/save")
    if not candidate.is_file() or not any(candidate.is_relative_to(owned_root(source, owner)) for owner in owners):
        raise ValueError(f"input is missing or outside declared owners: {path}")
    if expected is not None:
        before = candidate.stat()
        digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
        after = candidate.stat()
        # An existing stale alias and a concurrently changing retained input
        # must both fail rather than supplying a different regression baseline.
        if digest != expected or (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
            raise ValueError(f"indexed input hash changed: {path}")
    return candidate

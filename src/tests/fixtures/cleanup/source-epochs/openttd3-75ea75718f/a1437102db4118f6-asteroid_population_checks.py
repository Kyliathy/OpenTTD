"""Verify production asteroid population settings, native widgets and saved fields."""
import argparse
import json
import subprocess
from PIL import Image
from space_preview_checks import ROOT, chunks, identity, Reader
from runner import current_run_context, run_native, console_executable, save_native_capture

DENSITIES = (("sparse", 6), ("normal", 10), ("dense", 14))


def saved_settings(raw):
    """Decode the actual flat PATS table; fail on any unexpected native shape."""
    reader = Reader(raw)
    header = Reader(reader.take(reader.gamma() - 1))
    fields = []
    while True:
        kind = header.byte()
        if kind == 0:
            break
        fields.append((kind, header.take(header.gamma()).decode("utf-8")))
    assert header.pos == len(header.data)
    row = Reader(reader.take(reader.gamma() - 1))
    result = {}
    widths = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}
    for kind, name in fields:
        primitive = kind & 15
        if primitive == 10:
            result[name] = row.take(row.gamma())
        else:
            assert primitive in widths and not kind & 16, (kind, name)
            result[name] = int.from_bytes(row.take(widths[primitive]), "big", signed=primitive in (1, 3, 5, 7))
    assert row.pos == len(row.data)
    assert reader.gamma() == 0 and reader.pos == len(reader.data)
    return result


def configure(context, profile, density):
    context.prepare_runtime()
    lines = context.config_path.read_text(encoding="utf-8").splitlines()
    keys = {"gameplay_profile", "asteroid_density", "starting_year"}
    lines = [line for line in lines if line.split("=", 1)[0].strip() not in keys]
    context.config_path.write_text("\n".join(lines) + f"\ngameplay_profile = {profile}\nasteroid_density = {density}\nstarting_year = 2000\n", encoding="utf-8")


def launch(context, configuration, source=None):
    output = context.saves / "population.sav"
    script = f'pause\nset savegame_format lzma\nsave "{output.with_suffix("").as_posix()}"\nquit\n'
    (context.runtime_dir / "scripts/game_start.scr").write_text(script, encoding="utf-8")
    executable = console_executable(context, ROOT / "build" / configuration / "openttd.exe")
    command = [str(executable), "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", str(context.config_path), "-g"]
    command += ["-G", "12345"] if source is None else [str(source)]
    with (context.logs / "population.log").open("wb") as sink:
        run_native(context, command, cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=120)
    return output


def widgets(context, configuration):
    context.prepare_runtime()
    (context.runtime_dir / "scripts/game_start.scr").write_text("population_ui_check\n", encoding="utf-8")
    with (context.logs / "widgets.log").open("wb") as sink:
        run_native(context, [str(ROOT / "build" / configuration / "openttd_test.exe"), "[.asteroid-population-ui]"], cwd=ROOT / "build", stdout=sink, stderr=subprocess.STDOUT, check=True, timeout=120)
    evidence = (context.logs / "widgets.log").read_text(encoding="utf-8", errors="replace")
    assert "All tests passed" in evidence and "dropdown=true classic_disabled=true restored=true" in evidence
    captures = []
    for ppm in sorted(context.images.glob("population-*.ppm")):
        with Image.open(ppm) as raster:
            assert len(set(raster.get_flattened_data())) > 20
            target = ppm.with_suffix(".png")
            save_native_capture(context, raster, ppm, target)
            captures.append(identity(target))
    assert len(captures) == 5
    return captures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    args = parser.parse_args()
    context = current_run_context("space", "population-verify")
    captures = widgets(context.for_feature("space", "population-ui"), args.configuration)
    results = []
    for profile in ("multilayer_industry", "classic"):
        for density, (name, count) in enumerate(DENSITIES):
            case = profile.replace("_", "-") + "-" + name
            owner = context.for_feature("space", "population-new-" + case)
            configure(owner, profile, name)
            output = launch(owner, args.configuration)
            original = identity(output)
            version, data = chunks(output)
            assert version >= 406
            assert saved_settings(data[b"PATS"])["game_creation.asteroid_density"] == density
            header = data[b"ASTR"][3:54]
            active = profile == "multilayer_industry"
            assert int.from_bytes(header[27:31], "big") == (count if active else 0)
            assert int.from_bytes(header[31:35], "big") == 0
            if active:
                assert header[26] == density
            reload = context.for_feature("space", "population-reload-" + case)
            configure(reload, profile, DENSITIES[(density + 1) % 3][0])
            continued = launch(reload, args.configuration, output)
            new_version, after = chunks(continued)
            assert new_version == version and after[b"ASTR"] == data[b"ASTR"]
            assert saved_settings(after[b"PATS"])["game_creation.asteroid_density"] == density
            assert identity(output) == original
            results.append({"profile": profile, "density": name, "bodies": count if active else 0, "original": original, "reload": identity(continued), "config_override_ignored": True})
            print(f"Population {case}/{args.configuration}: saved setting and exact field reload passed", flush=True)
    report = context.reports / "population-verification.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({"configuration": args.configuration, "cases": results, "native_widget_captures": captures}, indent=2) + "\n", encoding="utf-8")
    print(f"Population verification: {report.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    main()

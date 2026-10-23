"""Move existing parser functions to pure owners without changing their executable AST.

@file isolate_saved_readers.py Bounded source refactor for r2fc-file-cleanup3.md8.0.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.
"""

import ast
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def extract(relative, names):
    """Read exact selected function bodies before removing their duplicate definitions.

    @param relative Repository-relative original source file.
    @param names Explicit existing functions to move into a pure shared owner.
    @return Original source, AST nodes and exact source text keyed by function name.
    """
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    nodes = {node.name: node for node in ast.parse(text).body if isinstance(node, ast.FunctionDef) and node.name in names}
    if set(nodes) != set(names):
        raise ValueError(f"missing original reader: {relative}: {names}")
    return text, nodes, {name: ast.get_source_segment(text, node) for name, node in nodes.items()}


def remove_definitions(text, nodes):
    """Remove only exact moved definitions while leaving every orchestration body intact.

    @param text Original source whose function AST has already been captured.
    @param nodes Exact top-level functions moved to their new shared pure owner.
    @return Source with those complete definitions removed, preserving all other lines.
    """
    lines = text.splitlines(keepends=True)
    # Delete spans from the bottom so source positions of earlier definitions
    # cannot drift and accidentally remove an unrelated native producer.
    for node in sorted(nodes.values(), key=lambda item: item.lineno, reverse=True):
        del lines[node.lineno - 1:node.end_lineno]
    return "".join(lines)


def main():
    """Install pure shared readers and preserve the original API via explicit re-exports.

    @return None; record AST equality before any new checkpoint expectation is evaluated.
    """
    table_path = "src/tests/tbm_logistics_save_fixture.py"
    geology_path = "src/tests/geology_save_fixture.py"
    factory_path = "src/tests/multilayer/acceptance/surface_entry_chemical_checks-4.py"
    lives_path = "src/tests/multilayer/acceptance/surface_entry_capital_checks-1.py"
    structures_path = "src/tests/multilayer/acceptance/orbital_save_checks-1.py"
    table_text, table_nodes, table_functions = extract(table_path, ("table_header", "table_record", "saved_table", "owned_chunks"))
    geology_text, geology_nodes, geology_functions = extract(geology_path, ("chunks",))
    factory_text, factory_nodes, factory_functions = extract(factory_path, ("observe_factory",))
    lives_text, lives_nodes, lives_functions = extract(lives_path, ("lives",))
    structures_text, structures_nodes, structures_functions = extract(structures_path, ("structures",))
    widths = next(node for node in ast.parse(table_text).body if isinstance(node, ast.Assign)
        and any(isinstance(target, ast.Name) and target.id == "WIDTHS" for target in node.targets))
    table_header = '''"""Read-only native save-table transport; importing creates no runtime context.

@file savegame_tables.py Sole owner of the existing logistics/geology decoder primitives.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0.
@see docs/zz-reach2/architecture/savegame/archi-savegame.md.
Functions are moved without executable AST changes; native fixture entry points
re-export them. Saved named schemas, not this helper, own gameplay field meanings.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "mlm/tools/savegames"))
from savegame_plane_digest import Reader, read_stream, CHUNK_RIFF, CHUNK_RECORDS

# Primitive widths decode the original named schema; they add no gameplay fields.
'''
    pure_tables = table_header + ast.get_source_segment(table_text, widths) + "\n\n\n" + geology_functions["chunks"] + "\n\n\n" + "\n\n\n".join(table_functions.values()) + "\n"
    state_header = '''"""Pure original Space save observations, independent of native orchestration imports.

@file space_saved_state.py Existing factory/life/SPST decoding with unchanged executable AST.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group8.0.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md.
The saved native owner tables remain authoritative; no body, stock, clock or
financial state is generated or changed here.
"""

from space_preview_checks import chunks
from launch_center_checks import riff_location
from savegame_tables import saved_table

TABLE = saved_table
# Preserve the original observation wiring used by existing ledger APIs.
OWNERS = {"saved_table": saved_table}

'''
    pure_state = state_header + "\n\n\n".join([factory_functions["observe_factory"], lives_functions["lives"], structures_functions["structures"]]) + '\n\n# Compatibility wiring retains the original factory/table lookup API.\nFACTORY = {"observe_factory": observe_factory, "OWNERS": OWNERS}\n'
    # AST equality protects the actual decoder/observer business rules while
    # only their import ownership changes. Comments/docs do not supply expected data.
    checks = []
    for filename, generated, originals in (
        ("src/tests/multilayer/support/savegame_tables.py", pure_tables, {**geology_functions, **table_functions}),
        ("src/tests/multilayer/acceptance/space_saved_state.py", pure_state, {**factory_functions, **lives_functions, **structures_functions})):
        parsed = {node.name: node for node in ast.parse(generated).body if isinstance(node, ast.FunctionDef)}
        for name, original in originals.items():
            if ast.dump(ast.parse(original).body[0], include_attributes=False) != ast.dump(parsed[name], include_attributes=False):
                raise ValueError(f"reader executable AST changed: {name}")
            checks.append(dict(function=name, unchanged_ast=True, owner=filename))
        (ROOT / filename).write_text(generated, encoding="utf-8")
    table_result = remove_definitions(table_text, table_nodes)
    table_result = table_result.replace('from geology_generation_fixture import Reader, read_stream\nfrom geology_save_fixture import chunks\n', '')
    table_result = table_result.replace(ast.get_source_segment(table_text, widths) + "\n", "")
    table_result = table_result.replace('from artifact_paths import resolve_input_path\n', 'from artifact_paths import resolve_input_path\nfrom savegame_tables import Reader, read_stream, chunks, WIDTHS, table_header, table_record, saved_table, owned_chunks\n')
    (ROOT / table_path).write_text(table_result, encoding="utf-8")
    (ROOT / geology_path).write_text(remove_definitions(geology_text, geology_nodes).replace('import hashlib\n', 'import hashlib\nfrom savegame_tables import chunks\n'), encoding="utf-8")
    (ROOT / factory_path).write_text(remove_definitions(factory_text, factory_nodes).replace('from space_preview_checks import ROOT, chunks, identity\n', 'from space_preview_checks import ROOT, chunks, identity\nfrom space_saved_state import observe_factory\n'), encoding="utf-8")
    (ROOT / lives_path).write_text(remove_definitions(lives_text, lives_nodes).replace('from space_preview_checks import ROOT, chunks, identity\n', 'from space_preview_checks import ROOT, chunks, identity\nfrom space_saved_state import lives\n'), encoding="utf-8")
    (ROOT / structures_path).write_text(remove_definitions(structures_text, structures_nodes).replace('from launch_center_checks import riff_location\n', 'from space_saved_state import structures\n'), encoding="utf-8")
    rewrites = {
        "surface_chemical_ledger.py": [('FACTORY = runpy.run_path(str(Path(__file__).with_name("surface_entry_chemical_checks-4.py")))\nTABLE = FACTORY["OWNERS"]["saved_table"]', 'from space_saved_state import FACTORY, TABLE')],
        "surface_regional_ledger.py": [('ORIGINAL_LIVES = runpy.run_path(str(Path(__file__).with_name("surface_entry_capital_checks-1.py")))["lives"]', 'from space_saved_state import lives as ORIGINAL_LIVES')],
        "surface_gateway_core_ledger.py": [('LAYOUT = runpy.run_path(str(DIRECTORY / "orbital_save_checks-1.py"))["structures"]', 'from space_saved_state import structures as LAYOUT')],
    }
    for name, replacements in rewrites.items():
        path = ROOT / "src/tests/multilayer/acceptance" / name
        text = path.read_text(encoding="utf-8")
        for old, new in replacements:
            if old not in text:
                raise ValueError(f"original data dependency changed: {name}")
            text = text.replace(old, new)
        text = text.replace("import runpy\n", "")
        path.write_text(text, encoding="utf-8")
    (ROOT / "test-cache/cleanup/reader-isolation.json").write_text(json.dumps(dict(_documentation=__doc__, functions=checks), indent=2) + "\n", encoding="utf-8")
    print("Moved existing read-only functions without AST changes:",len(checks))


if __name__ == "__main__":
    main()

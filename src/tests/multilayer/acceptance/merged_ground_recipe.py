"""Expose bounded original-world recovery through the sole producer catalogue.

@file merged_ground_recipe.py Selected reconstruction, never a replay of all acceptance history.
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
"""
import json
from pathlib import Path

from artifact_paths import RunContext, contained
from artifact_rebuild import identity, verify_current_build
from normal_ground_merge import checked, read, select_original, migrate_original
from normal_ground_recovery import run_selected
from savegame_tables import first_tick_worlds


def execute_stage(request, root):
    """Run one declared existing recovery owner and publish its actual child proof.

    @param request Exact catalogue stage with retained input roles and current build.
    @param root Explicit checkout; isolated copies have no access to older reviews.
    @return Actual preparation or native checks for this selected leaf only.
    """
    recipe = request['declaration']
    mechanism = recipe['original_parameters']['mechanism']
    # Recovery cannot publish canonical assets or reopen paid Space work.
    # Only these two executor-owned stages may invoke the selected observer.
    if request['stage'] not in ('prepare', 'verify') or request['paid_work_authorized'] is not False or request['publishes_source'] is not False:
        raise ValueError('merged recovery requires an explicit bounded read-only stage')
    inputs = {role: checked(reference, root) for role, reference in request['inputs'].items()}
    selected = select_original('standard', root) if mechanism == 'original-migration' else None
    if request['stage'] == 'prepare':
        return dict(checks=['declared retained originals and current selected image verified'], mechanism=mechanism)
    verify_current_build(request['build'], request['configuration'], recipe, root)
    build_reference = request['build']['result']
    context = RunContext(root, request['feature'], request['scenario'], request['run_id'])
    if mechanism == 'original-migration':
        child = migrate_original('standard', build_reference, root)
        observation = read(child, root)
        facts = dict(child=child, compared_families=observation['compared_families'],
            original=selected['source'], output=observation['output'], gameplay_ticks_executed=0,
            classification='genuine_paid_original_zero_tick_migration')
    elif mechanism == 'useful-first-frame':
        contract = json.loads(inputs['recovery-contract'].read_text())
        source = contract['useful_wait']
        checked(source, root)
        child_context, proof, text = run_selected(build_reference, 'normal-ground-recovery', '[.normal-ground-wait]', source, root=root)
        replay = first_tick_worlds(child_context.saves / 'useful-wait')
        if 'MERGED_FIRST_TICK_NATIVE' not in text:
            raise ValueError('selected useful wait lacks its actual first-frame observer')
        facts = dict(native=proof, compared_families=replay['families'],
            saves={name: identity(replay[name], root) for name in ('before', 'direct', 'reloaded')},
            native_ticks_per_branch=1, classification='genuine_original_useful_wait_recovery')
    elif mechanism == 'supply-lifecycle':
        _, proof, _ = run_selected(build_reference, 'merged-supply-recovery', '[surface-supply-window]', root=root)
        facts = dict(native=proof, classification='prepared_native_player_control_component')
    elif mechanism == 'utility-performance':
        from bounded_performance import observe
        child_context, observation, _ = observe(build_reference, '[utility-power-performance]', {},
            ('src/tests/utility_power_performance.cpp',), 'merged-utility', Path(__file__))
        if observation['failure'] is not None:
            raise ValueError(observation['failure'])
        child = child_context.reports / 'observed-component.json'
        child.write_text(json.dumps(dict(_documentation=__doc__, **observation), indent=2) + '\n')
        facts = dict(child=identity(child, root), classification='prepared_native_utility_timing_component',
            acceptance='observation only; frozen workload budget comparison remains separate')
    else:
        raise ValueError('unknown implemented merged recovery mechanism')
    checked(read(build_reference, root)['targets']['openttd_tube_test'], root)
    target = contained(root / request['outputs']['manifest'], context.reports)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True,
        kind='merged_ground_recovery_recipe', mechanism=mechanism, configuration=request['configuration'],
        build=build_reference, retained_inputs=request['inputs'], facts=facts,
        historical_acceptance_replayed=False, human_acceptance=False), indent=2) + '\n')
    return dict(checks=['actual selected native recovery completed with declared originals and current image',
        'producing image/source and retained inputs unchanged'], **facts)

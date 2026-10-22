"""Close the current checkpoint only after its selected validation has passed."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]

def identity(relative):
    path = root / relative
    return {'path': path.relative_to(root).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

state_path = root / 'logs/space-survey-consolidated-build-state.json'
state = json.loads(state_path.read_text(encoding='utf-8'))
for row in state['source'] + [state['debug_only_build_setting']]:
    assert identity(row['path'])['sha256'] == row['sha256'], ('unrecorded source edit', row['path'])

passes = []
for configuration, count in (('RelWithDebInfo', 5), ('Debug', 13)):
    relative = f'logs/space-survey-{configuration}-consolidated-checks.json'
    current = json.loads((root / relative).read_text(encoding='utf-8'))
    assert len(current['checks']) == count, ('pass still running', configuration)
    assert all(row['exit_code'] == 0 for row in current['checks']), ('pass has failures', configuration)
    for row in current['binaries']:
        assert identity(row['path']) == row, ('binary replaced after verification', row['path'])
    reports = []
    for check in current['checks']:
        log = check['log']
        assert identity(log['path']) == log
        data = (root / log['path']).read_bytes()
        text = data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8', errors='replace')
        if check['check'] == 'contracts':
            assert 'All tests passed (685 assertions in 15 test cases)' in text
        for match in re.findall(r'mlm[\\/]reviews[\\/][^\r\n]*?\.json', text):
            path = match.replace('\\', '/')
            if (root / path).is_file() and path not in [row['path'] for row in reports]:
                reports.append(identity(path))
    passes.append({'configuration': configuration, 'check_count': count, 'outcomes': identity(relative),
                   'binaries': current['binaries'], 'reports': reports})

record = {
    'recorded_at_utc': datetime.now(timezone.utc).isoformat(),
    'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
    'save_version': 413,
    'status': 'survey/service checkpoint passed; full Plan9/10 goal remains active',
    'full_goal_complete': False,
    'source': state['source'],
    'debug_only_build_setting': state['debug_only_build_setting'],
    'builds': [
        {'configuration': 'RelWithDebInfo', 'exit_code': 0, 'log': identity('logs/space-survey-RelWithDebInfo-consolidated-build.log')},
        {'configuration': 'Debug', 'exit_code': 1, 'reason': 'MSVC C1128 in the complete network command table', 'log': identity('logs/space-survey-Debug-consolidated-build.log')},
        {'configuration': 'Debug', 'exit_code': 0, 'log': identity('logs/space-survey-Debug-bigobj-build.log')},
    ],
    'verification': passes,
    'inspection': identity('logs/space-survey-inspector-review.json'),
    'historical_initial_rwd_pass': identity('logs/space-survey-RelWithDebInfo-checks.json'),
    'historical_scope': 'Earlier 413 RelWithDebInfo broader checks remain dated evidence; the current affected native suites and all Debug checks are listed separately above.',
    'remaining_scope': ['orbital resupply and exhausted-mining pause', 'construction probes, first Core and orbital Steel/Goods production/reuse',
                        'full finite-resource disposition accounting', 'native vehicle UI and final art', 'integrated useful-delivery, balance and actual network acceptance'],
    'next_batch': {'groups': ['Plan9 Group11B', 'Plan10 Group12B'], 'characterization': identity('logs/space-resupply-characterization.json')},
}
target = root / 'logs/space-survey-verification.json'
target.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
state['status'] = record['status']
state['debug_validation_exit_code'] = 0
state['debug_validation_finished_observed_at_utc'] = record['recorded_at_utc']
state['verification_manifest'] = identity('logs/space-survey-verification.json')
state_path.write_text(json.dumps(state, indent=2) + '\n', encoding='utf-8')
print(f'Closed 413 checkpoint: {target.relative_to(root)}; full goal remains active')

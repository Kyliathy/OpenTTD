"""Close resupply verification only when compiled sources and runtime evidence agree."""
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

def read_log(relative):
    data = (root / relative).read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8', errors='replace')

state_path = root / 'logs/space-resupply-build-state.json'
state = json.loads(state_path.read_text(encoding='utf-8-sig'))
sources = {row['path']: row for row in state['source']}
for row in state['compile_corrections'] + state.get('validation_additions', []):
    sources[row['path']] = {'path': row['path'], 'sha256': row['sha256']}
for row in sources.values():
    assert identity(row['path']) == row, ('unrecorded source change', row['path'])
passes = []
for configuration, count in (('RelWithDebInfo', 7), ('Debug', 15)):
    relative = f'logs/space-resupply-{configuration}-consolidated-checks.json'
    current = json.loads((root / relative).read_text(encoding='utf-8'))
    assert len(current['checks']) == count, ('runtime pass incomplete', configuration)
    assert all(row['exit_code'] == 0 for row in current['checks']), ('runtime pass has failures', configuration)
    for row in current['binaries']:
        assert identity(row['path']) == row, ('binary changed after checks', row['path'])
    reports = []
    for check in current['checks']:
        assert identity(check['log']['path']) == check['log']
        data = read_log(check['log']['path'])
        if check['check'] == 'contracts':
            assert 'All tests passed (713 assertions in 17 test cases)' in data
        for match in re.findall(r'mlm[\\/]reviews[\\/][^\r\n]*?\.json', data):
            path = match.replace('\\', '/')
            if (root / path).is_file() and path not in [row['path'] for row in reports]:
                report = json.loads((root / path).read_text(encoding='utf-8-sig'))
                assert report.get('configuration', configuration) == configuration
                reports.append(identity(path))
    if configuration == 'RelWithDebInfo':
        migration_log = f'logs/space-resupply-{configuration}-migration.log'
        data = read_log(migration_log)
        assert 'Ordinary413-to414 migration verified:' in data
        for match in re.findall(r'mlm[\\/]reviews[\\/][^\r\n]*?\.json', data):
            reports.append(identity(match.replace('\\', '/')))
        migration = {'log': identity(migration_log), 'exit_code': state['rwd_migration_exit_code']}
        assert migration['exit_code'] == 0
    else:
        migration = {'included_check': 'migration'}
    passes.append({'configuration': configuration, 'check_count': count, 'outcomes': identity(relative),
                   'initial_outcomes': current.get('initial_outcomes'),
                   'binaries': current['binaries'], 'reports': reports, 'migration': migration})

record = {
    'recorded_at_utc': datetime.now(timezone.utc).isoformat(),
    'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
    'save_version': 414, 'status': 'paid resupply/repeat checkpoint passed; remaining Plan9/10 work paused at user request',
    'full_goal_complete': False, 'source': list(sources.values()),
    'compile_corrections': state['compile_corrections'], 'validation_additions': state.get('validation_additions', []),
    'user_compiler_preference': state['user_compiler_preference'],
    'builds': [
        {'configuration': 'RelWithDebInfo', 'exit_code': 1, 'reason': 'explicit native Owner conversion in copied company lookup', 'log': identity('logs/space-resupply-RelWithDebInfo-build.log')},
        {'configuration': 'RelWithDebInfo', 'exit_code': 0, 'log': identity('logs/space-resupply-RelWithDebInfo-corrected-build.log')},
        {'configuration': 'Debug', 'exit_code': 0, 'log': identity('logs/space-resupply-Debug-build.log')},
    ],
    'verification': passes, 'inspection': identity('logs/space-resupply-inspector-review.json'),
    'remaining_scope': ['blueprints, construction probes, first Core and orbital Steel/Goods production/reuse',
                        'complete finite-resource disposition accounting and recovery', 'native fleet UI and final art',
                        'useful Surface delivery, balance and actual network acceptance'],
}
target = root / 'logs/space-resupply-verification.json'
target.write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
state['status'] = record['status']
state['verification_manifest'] = identity('logs/space-resupply-verification.json')
state_path.write_text(json.dumps(state, indent=2) + '\n', encoding='utf-8')
print(f'Closed414 resupply checkpoint: {target.relative_to(root)}; stop after this batch requested')

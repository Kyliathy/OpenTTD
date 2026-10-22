"""Rerun only compatibility checks affected by the shared native output filename."""
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import subprocess
import time

root = Path(__file__).resolve().parents[1]
target = root / 'logs/space-resupply-Debug-consolidated-checks.json'
result = json.loads(target.read_text(encoding='utf-8'))
assert len(result['checks']) == 15
failed = [row for row in result['checks'] if row['exit_code']]
assert {row['check'] for row in failed} == {'surface_handling', 'launch_center'}
initial = root / 'logs/space-resupply-Debug-initial-checks.json'
assert not initial.exists()
initial.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')

def identity(path):
    return {'path': path.relative_to(root).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

def rerun(row):
    name = row['check']
    log = root / f'logs/space-resupply-Debug-corrected-{name}.log'
    start = time.monotonic()
    print(f'Rechecking {name}/Debug without rebuilding', flush=True)
    with log.open('wb') as sink:
        run = subprocess.run(row['command'], cwd=root, stdout=sink, stderr=subprocess.STDOUT)
    print(f'{name}/Debug: exit {run.returncode}', flush=True)
    print('\n'.join(log.read_text(encoding='utf-8', errors='replace').splitlines()[-7:]), flush=True)
    return {'check': name, 'exit_code': run.returncode, 'elapsed_seconds': round(time.monotonic() - start, 2),
            'command': row['command'], 'log': identity(log), 'previous_attempt': row,
            'correction': 'Helper consumes the native producer filename launch-center-413.sav; actual header remains asserted414'}

result['initial_outcomes'] = identity(initial)
with ThreadPoolExecutor(max_workers=2) as pool:
    for future in as_completed([pool.submit(rerun, row) for row in failed]):
        row = future.result()
        result['checks'] = [row if previous['check'] == row['check'] else previous for previous in result['checks']]
        target.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
raise SystemExit(1 if any(row['exit_code'] for row in result['checks']) else 0)

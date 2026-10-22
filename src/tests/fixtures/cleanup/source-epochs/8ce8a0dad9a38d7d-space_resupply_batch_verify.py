"""Collect the connected resupply batch's runtime outcomes without rebuilding."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--configuration', choices=('RelWithDebInfo', 'Debug'), required=True)
parser.add_argument('--extended', action='store_true')
parser.add_argument('--jobs', type=int, choices=(1, 2), default=1)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
prefix = root / 'logs' / f'space-resupply-{args.configuration}-consolidated'
base = root / 'src/tests/multilayer/acceptance'
tags = '[space-resupply-contract],[space-survey-contract],[space-logistics-contract],[launch-assembly-contract],[space-order-contract],[space-content],[launch-center-geometry]'
commands = [('contracts', [str(root / 'build' / args.configuration / 'openttd_test.exe'), tags])]
names = ['space_resupply', 'space_descent', 'space_survey', 'launch_logistics', 'launch_flight', 'space_order']
if args.extended:
    names += ['launch_assembly', 'surface_handling', 'launch_center']
commands += [(name, [sys.executable, '-B', str(base / f'{name}_checks.py'), '--configuration', args.configuration]) for name in names]
commands += [('migration', [sys.executable, '-B', str(base / 'space_resupply_checks.py'), '--configuration', args.configuration,
                           '--migration-manifest', str(root / 'logs/space-resupply-migration-inputs.json')])]
if args.extended:
    commands += [
        ('profiles', [sys.executable, '-B', str(base / 'space_order_checks.py'), '--profiles-only', '--configuration', args.configuration]),
        ('underground', [sys.executable, '-B', str(base / 'underground_demo_checks.py'), 'verify', '--configuration', args.configuration]),
        ('space-preview', [sys.executable, '-B', str(base / 'space_preview_checks.py'), 'verify', '--configuration', args.configuration]),
        ('space-mining', [sys.executable, '-B', str(base / 'space_mining_checks.py'), 'verify', '--configuration', args.configuration]),
    ]

def identity(path):
    return {'path': path.relative_to(root).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

result = {'configuration': args.configuration, 'extended': args.extended, 'jobs': args.jobs,
          'binaries': [identity(root / 'build' / args.configuration / name) for name in ('openttd.exe', 'openttd_test.exe')], 'checks': []}
target = Path(str(prefix) + '-checks.json')
def run_check(name, command):
    log = Path(str(prefix) + f'-{name}.log')
    start = time.monotonic()
    print(f'Starting {name}/{args.configuration}', flush=True)
    with log.open('wb') as output:
        run = subprocess.run(command, cwd=root, stdout=output, stderr=subprocess.STDOUT)
    print(f'{name}/{args.configuration}: exit {run.returncode}', flush=True)
    print('\n'.join(log.read_text(encoding='utf-8', errors='replace').splitlines()[-7:]), flush=True)
    return {'check': name, 'exit_code': run.returncode, 'elapsed_seconds': round(time.monotonic() - start, 2), 'log': identity(log), 'command': command}

with ThreadPoolExecutor(max_workers=args.jobs) as pool:
    checks = [pool.submit(run_check, name, command) for name, command in commands]
    for completed in as_completed(checks):
        result['checks'].append(completed.result())
        result['checks'].sort(key=lambda row: next(i for i, command in enumerate(commands) if command[0] == row['check']))
        target.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
sys.exit(1 if any(row['exit_code'] for row in result['checks']) else 0)

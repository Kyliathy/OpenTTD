"""Run an explicitly selected consolidated validation pass without rebuilding."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--configuration', choices=('RelWithDebInfo', 'Debug'), required=True)
parser.add_argument('--focused', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
prefix = root / 'logs' / f'space-survey-{args.configuration}-consolidated'
base = root / 'src/tests/multilayer/acceptance'
tags = '[space-survey-contract],[space-logistics-contract],[launch-assembly-contract],[space-order-contract],[space-content],[launch-center-geometry]'
commands = [('contracts', [str(root / 'build' / args.configuration / 'openttd_test.exe'), tags])]
names = ('space_descent', 'space_survey', 'launch_logistics') if args.focused else (
    'space_descent', 'space_survey', 'launch_logistics', 'launch_flight', 'launch_assembly',
    'space_order', 'surface_handling', 'launch_center')
commands += [(name, [sys.executable, '-B', str(base / f'{name}_checks.py'), '--configuration', args.configuration]) for name in names]
if not args.focused:
    commands += [
        ('profiles', [sys.executable, '-B', str(base / 'space_order_checks.py'), '--profiles-only', '--configuration', args.configuration]),
        ('underground', [sys.executable, '-B', str(base / 'underground_demo_checks.py'), 'verify', '--configuration', args.configuration]),
        ('space-preview', [sys.executable, '-B', str(base / 'space_preview_checks.py'), 'verify', '--configuration', args.configuration]),
        ('space-mining', [sys.executable, '-B', str(base / 'space_mining_checks.py'), 'verify', '--configuration', args.configuration]),
    ]

def identity(path):
    return {'path': path.relative_to(root).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}

result = {'configuration': args.configuration, 'focused': args.focused,
          'binaries': [identity(root / 'build' / args.configuration / name) for name in ('openttd.exe', 'openttd_test.exe')],
          'checks': []}
target = Path(str(prefix) + '-checks.json')
for name, command in commands:
    log = Path(str(prefix) + f'-{name}.log')
    start = time.monotonic()
    print(f'Starting {name}/{args.configuration}', flush=True)
    with log.open('wb') as output:
        run = subprocess.run(command, cwd=root, stdout=output, stderr=subprocess.STDOUT)
    result['checks'].append({'check': name, 'exit_code': run.returncode, 'elapsed_seconds': round(time.monotonic() - start, 2), 'log': identity(log), 'command': command})
    target.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f'{name}/{args.configuration}: exit {run.returncode}', flush=True)
    print('\n'.join(log.read_text(encoding='utf-8', errors='replace').splitlines()[-5:]), flush=True)
sys.exit(1 if any(row['exit_code'] for row in result['checks']) else 0)

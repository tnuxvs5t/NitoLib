#!/usr/bin/env python3
"""Build the bounded design experiment; never modify or replace the V3 gate."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--report', type=Path)
parser.add_argument('--mode', choices=['debug', 'opt', 'san', 'all'], default='all')
args = parser.parse_args()
cxx = os.environ.get('CXX', 'g++')
common = ['-std=c++23', '-Wall', '-Wextra', '-Wpedantic', '-Wshadow', '-Werror']
modes = {
    'debug': ['-O0', '-g', '-D_GLIBCXX_ASSERTIONS'],
    'opt': ['-O2', '-DNDEBUG'],
    'san': ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'],
}
selected = modes if args.mode == 'all' else {args.mode: modes[args.mode]}
expected = {
    'grouping': '2: 10\n4: 15\n9: 1\n',
    'shortest_path': 'distance: 0 2 1 4\npath: 0 2 1 3\n',
    'ordered_path': 'DBACF\nDBCF\nDXACF\n',
    'affine_sum': '56\n26\n',
}
report = {'created_utc': datetime.now(timezone.utc).isoformat(),
          'compiler': subprocess.check_output([cxx, '--version'], text=True).splitlines()[0],
          'flags': common, 'index_mode': 'fixed int', 'runs': [], 'headers': [], 'examples': {}}

def limits():
    resource.setrlimit(resource.RLIMIT_STACK, (8 * 1024**2, 8 * 1024**2))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

def execute(binary):
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
    return subprocess.run([str(binary)], check=True, capture_output=True, text=True,
                          timeout=90, preexec_fn=limits, env=env).stdout

with tempfile.TemporaryDirectory(prefix='nitori-design-') as temporary:
    build = Path(temporary)
    for header in sorted((ROOT / 'include').glob('*.hpp')):
        subprocess.run([cxx, *common, '-x', 'c++', '-fsyntax-only', '-'],
                       input=f'#include "{header}"\n', text=True, check=True)
        report['headers'].append(header.name)
    for mode, flags in selected.items():
        for source in sorted((ROOT / 'tests').glob('*.cpp')):
            if source.stem == 'cost': continue
            binary = build / f'{source.stem}-{mode}'
            print(f'compile + run {source.stem} [{mode}, 8 MiB stack]', flush=True)
            subprocess.run([cxx, *common, *flags, str(source), '-o', str(binary)], check=True)
            output = execute(binary)
            report['runs'].append({'test': source.stem, 'mode': mode, 'stack_mib': 8, 'output': output.strip()})
    for name, oracle in expected.items():
        source = ROOT / 'examples' / f'{name}.cpp'
        binary = build / name
        subprocess.run([cxx, *common, '-O2', str(source), '-o', str(binary)], check=True)
        output = execute(binary)
        if output != oracle: raise RuntimeError(f'{name}: {output!r} != {oracle!r}')
        report['examples'][name] = output
    binary = build / 'cost'
    subprocess.run([cxx, *common, '-O2', str(ROOT / 'tests/cost.cpp'), '-o', str(binary)], check=True)
    report['cost'] = json.loads(execute(binary))

report['source_sha256'] = {
    str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
    for folder in ('include', 'examples', 'tests') for p in sorted((ROOT / folder).glob('*')) if p.is_file()
}
report['status'] = 'passed'
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
print(json.dumps({k: report[k] for k in ('status', 'cost')}, indent=2))
print(f"PASS: {len(report['runs'])} property/deep runs, {len(expected)} examples, {len(report['headers'])} standalone headers")

#!/usr/bin/env python3
"""Run concurrentqueue's own unit tests (tests/unittests of the release)
built against import mcpplibs.mwrap.concurrentqueue instead of the headers.

The release tarball is downloaded once into target/upstream and checked
against upstream/documents.json. Its test harness is copied unchanged into
target/conformance/unittests (tests/common/*, tests/corealgos.h), except for
these mechanical changes, each written out below:

- unittests.cpp and mallocmacro.cpp: the three `#include "../../…h"` of the
  queue become `import mcpplibs.mwrap.concurrentqueue;` (and `.c_api`, built
  with the c-api feature), after the standard headers those three included,
  so nothing of the queue is declared by a header;
- the syntax macros the tests use are spelled as upstream defines them in
  this configuration (MOODYCAMEL_NOEXCEPT -> noexcept,
  MOODYCAMEL_DELETE_FUNCTION -> = delete, MOODYCAMEL_ALIGNAS(x) ->
  alignas(x)), and the one `#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED`
  block becomes `if constexpr` on the module's constant of that name;
- details::circular_less_than, a `static` helper the tests reach into, is the
  module's adaptation, mcpplibs::mwrap::concurrentqueue::details::circular_less_than;
- minitest.h declares abi::__cxa_demangle, the one function it uses, instead
  of including <cxxabi.h>: with GCC 16, a unit that includes <cxxabi.h>
  cannot instantiate the module's thread-exit path (docs/usage.md).

Every test runs (`--iterations`, default 2, bounds the time; upstream's
default is 8) and the run must end with upstream's "All tests passed.".
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch  # noqa: E402

# The standard headers concurrentqueue.h, blockingconcurrentqueue.h and
# lightweightsemaphore.h include, in their order: what the tests had through
# them. They come before the imports, so tests/corealgos.h (included later)
# finds them already included.
STANDARD = ['atomic', 'cassert', 'cstddef', 'cstdint', 'cstdlib', 'type_traits', 'algorithm', 'utility', 'limits',
            'climits', 'array', 'thread', 'mutex', 'memory', 'chrono', 'ctime', 'new']

IMPORTS = '''// [mwrap] the queue, from the module (was: the three headers).
{includes}
import mcpplibs.mwrap.concurrentqueue;
{c_api}'''

MANIFEST = '''[package]
name        = "concurrentqueue-unittests"
version     = "1.0.5"
description = "concurrentqueue 1.0.5's tests/unittests, built against import mcpplibs.mwrap.concurrentqueue (tools/run_conformance.py)"

[targets.unittests]
kind = "bin"
main = "tests/unittests/unittests.cpp"

[build]
sources = ["tests/common/*.cpp", "tests/unittests/mallocmacro.cpp"]

[dependencies]
"mcpplibs.mwrap.concurrentqueue" = {{ path = {root}, features = ["c-api"] }}
'''


def port_unittests(text: str) -> str:
    out, inserted = [], False
    for line in text.split('\n'):
        m = re.match(r'\s*#\s*include\s+"\.\./\.\./(concurrentqueue|blockingconcurrentqueue|c_api/concurrentqueue)\.h"', line)
        if m:
            if not inserted:
                out.append(IMPORTS.format(includes='\n'.join(f'#include <{h}>' for h in STANDARD),
                                          c_api='import mcpplibs.mwrap.concurrentqueue.c_api;'))
                inserted = True
            continue
        out.append(line)
    assert inserted
    text = '\n'.join(out)
    text = text.replace('MOODYCAMEL_DELETE_FUNCTION', '= delete')
    text = re.sub(r'\bMOODYCAMEL_NOEXCEPT\b', 'noexcept', text)
    text = re.sub(r'\bMOODYCAMEL_ALIGNAS\(', 'alignas(', text)
    text = re.sub(r'\bdetails::circular_less_than\b', 'mcpplibs::mwrap::concurrentqueue::details::circular_less_than', text)
    # The one #ifdef block on the configuration: if constexpr on the mapped constant.
    lines = text.split('\n')
    start = next(i for i, l in enumerate(lines) if l.strip() == '#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED')
    end = next(i for i in range(start + 1, len(lines)) if lines[i].strip().startswith('#'))
    assert lines[end].strip() == '#endif', lines[end]
    lines[start] = '\t\tif constexpr (mcpplibs::mwrap::concurrentqueue::macros::MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED) {  // [mwrap] was #ifdef'
    lines[end] = '\t\t}  // [mwrap] was #endif'
    text = '\n'.join(lines)
    leftover = re.findall(r'(?<!macros::)\bMOODYCAMEL_\w+|(?<!concurrentqueue::)\bdetails::\w+', text)
    leftover = [x for x in leftover if not x.startswith('details::') or 'corealgos' in x]
    assert not leftover, leftover
    return text


def port_mallocmacro(text: str) -> str:
    return re.sub(r'#include "\.\./\.\./blockingconcurrentqueue\.h"',
                  IMPORTS.format(includes='', c_api='').strip() + '\n', text)


def port_minitest(text: str) -> str:
    old = '#include <cxxabi.h>'
    assert old in text
    return text.replace(old, '// [mwrap] abi::__cxa_demangle, declared as <cxxabi.h> declares it (see the module docs)\n'
                             '#include <cstddef>\n'
                             'namespace __cxxabiv1 { extern "C" char* __cxa_demangle(const char*, char*, std::size_t*, int*); }\n'
                             'namespace abi = __cxxabiv1;')


def build(project: Path, mcpp: str, toolchain: str | None) -> Path:
    command = [mcpp, 'build', '--profile', 'release'] + (['--toolchain', toolchain] if toolchain else [])
    print('+', ' '.join(command), flush=True)
    subprocess.run(command, cwd=project, check=True)
    exe = 'unittests' + ('.exe' if os.name == 'nt' else '')
    found = sorted((p for p in (project / 'target').rglob(exe) if p.parent.name == 'bin'), key=lambda p: p.stat().st_mtime)
    if not found:
        sys.exit(f'{project} was not built')
    return found[-1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcpp', default='mcpp')
    parser.add_argument('--toolchain')
    parser.add_argument('--iterations', type=int, default=2)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    upstream = fetch(root)
    project = root / 'target/conformance/unittests'
    if (project / 'tests').exists():
        shutil.rmtree(project / 'tests')
    (project / 'tests/unittests').mkdir(parents=True)
    shutil.copytree(upstream / 'tests/common', project / 'tests/common')
    shutil.copyfile(upstream / 'tests/corealgos.h', project / 'tests/corealgos.h')
    src = upstream / 'tests/unittests'
    (project / 'tests/unittests/unittests.cpp').write_text(port_unittests(src.joinpath('unittests.cpp').read_text(encoding='utf-8')), encoding='utf-8')
    (project / 'tests/unittests/mallocmacro.cpp').write_text(port_mallocmacro(src.joinpath('mallocmacro.cpp').read_text(encoding='utf-8')), encoding='utf-8')
    (project / 'tests/unittests/minitest.h').write_text(port_minitest(src.joinpath('minitest.h').read_text(encoding='utf-8')), encoding='utf-8')
    (project / 'mcpp.toml').write_text(MANIFEST.format(root='"' + root.as_posix() + '"'), encoding='utf-8')
    exe = build(project, args.mcpp, args.toolchain)
    command = [str(exe), '--disable-prompt', '--iterations', str(args.iterations)]
    print('+', ' '.join(command), flush=True)
    run = subprocess.run(command, cwd=project, capture_output=True, text=True, timeout=3600)
    output = run.stdout + run.stderr
    print(output, flush=True)
    registered = re.findall(r'^\s*REGISTER_TEST\((.+?)\);', src.joinpath('unittests.cpp').read_text(encoding='utf-8'), flags=re.M)
    ran = re.findall(r'^moodycamel::ConcurrentQueueTests::(.+?)\.\.\. \n\s+passed$', output.replace('\r\n', '\n'), flags=re.M)
    missing = [name for name in registered if name.replace(' ', '') not in {r.replace(' ', '') for r in ran}]
    print(f'{len(ran)} of the {len(registered)} registered tests passed, {args.iterations} iteration(s) each; exit {run.returncode}')
    if missing:
        print('not run or not passed:', ', '.join(missing))
    if run.returncode != 0 or 'All tests passed.' not in output or missing:
        sys.exit(1)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""The public surface of concurrentqueue's headers, from the pinned release.

Read by tools/check_exports.py, which audits the modules against it. The
headers come from the upstream tarball, downloaded once into target/upstream
and checked against upstream/documents.json: the same bytes
compat.concurrentqueue provides.

The headers choose between platform branches with the preprocessor (the
thread id: Win32, std::thread::id or a thread_local address; the semaphore:
Win32, Mach or POSIX). Their conditionals are evaluated here for one target
(x86-64 Linux, GCC, exceptions on, upstream's defaults); the entities they
declare are the same on every target, only their definitions differ. Every
`#define`, in any branch, is listed.
"""
from dataclasses import dataclass, field
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import urllib.request

HEADERS = ('concurrentqueue.h', 'blockingconcurrentqueue.h', 'lightweightsemaphore.h')
C_API = 'c_api/concurrentqueue.h'

TARGET = {'__GNUC__': '16', '__GNUC_MINOR__': '1', '__GNUG__': '16', '__cplusplus': '202302L', '__EXCEPTIONS': '1',
          '__linux__': '1', '__unix__': '1', '__x86_64__': '1', '__amd64__': '1', '__GLIBCXX__': '20260101',
          '__GLIBC__': '2', '__GLIBC_MINOR__': '39', '_GNU_SOURCE': '1', '__has_feature': ''}


def fetch(root: Path) -> Path:
    pins = json.loads((root / 'upstream/documents.json').read_text(encoding='utf-8'))
    cache = root / 'target/upstream'
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / 'concurrentqueue-release.tar.gz'
    if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest() != pins['upstream_sha256']:
        print('+ fetch', pins['upstream'], flush=True)
        with urllib.request.urlopen(pins['upstream'], timeout=120) as response:
            archive.write_bytes(response.read())
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != pins['upstream_sha256']:
        sys.exit(f'upstream archive checksum {digest} != {pins["upstream_sha256"]}')
    with tarfile.open(archive) as tar:
        top = tar.getmembers()[0].name.split('/')[0]
        if not (cache / top).exists():
            tar.extractall(cache, filter='data')
    return cache / top


def strip_comments(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', lambda m: ' ' + '\n' * m.group(0).count('\n'), text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


class Preprocessor:
    """Conditionals and #define / #undef only; nothing is expanded in the text."""

    def __init__(self, predefined: dict[str, str]):
        self.defines = dict(predefined)
        self.active: list[str] = []
        self.defined_anywhere: set[str] = set()

    def value(self, expr: str, depth: int = 0) -> int:
        expr = re.sub(r'\bdefined\s*\(\s*(\w+)\s*\)|\bdefined\s+(\w+)',
                      lambda m: '1' if (m.group(1) or m.group(2)) in self.defines else '0', expr)
        expr = re.sub(r'\b\w+\s*\([^()]*\)', '0', expr)  # __has_feature(…), __GLIBC_PREREQ(…)

        def name(m):
            word = m.group(0)
            if word in self.defines and depth < 8:
                body = self.defines[word].strip()
                return f'({self.value(body, depth + 1)})' if body else '1'
            return '0'
        expr = re.sub(r'\b[A-Za-z_]\w*\b', name, expr)
        expr = re.sub(r'\b(0x[0-9A-Fa-f]+|\d+)[uUlL]+\b', r'\1', expr)
        expr = expr.replace('&&', ' and ').replace('||', ' or ')
        expr = re.sub(r'!(?!=)', ' not ', expr)
        return int(eval(expr, {'__builtins__': {}}))  # noqa: S307 - integer constant expressions

    def run(self, text: str):
        stack: list[list[bool]] = []
        active = True
        for line in strip_comments(text).replace('\\\n', ' ').split('\n'):
            m = re.match(r'\s*#\s*(\w+)\s*(.*)', line)
            if not m:
                if active:
                    self.active.append(line)
                continue
            directive, rest = m.group(1), m.group(2).strip()
            if directive in ('if', 'ifdef', 'ifndef'):
                if directive == 'if':
                    cond = active and bool(self.value(rest))
                elif directive == 'ifdef':
                    cond = active and rest.split()[0] in self.defines
                else:
                    cond = active and rest.split()[0] not in self.defines
                stack.append([cond, cond, active])
                active = cond
            elif directive == 'elif':
                frame = stack[-1]
                cond = frame[2] and not frame[1] and bool(self.value(rest))
                frame[0], frame[1] = cond, frame[1] or cond
                active = cond
            elif directive == 'else':
                frame = stack[-1]
                cond = frame[2] and not frame[1]
                frame[0], frame[1] = cond, True
                active = cond
            elif directive == 'endif':
                active = stack.pop()[2]
            elif directive == 'define':
                d = re.match(r'(\w+)(\([^)]*\))?\s*(.*)', rest)
                self.defined_anywhere.add(d.group(1))
                if active:
                    self.defines[d.group(1)] = '' if d.group(2) else d.group(3)
            elif directive == 'undef' and active:
                self.defines.pop(rest.split()[0], None)


@dataclass
class Surface:
    entities: list[str] = field(default_factory=list)      # namespace moodycamel, outside details
    details: list[str] = field(default_factory=list)       # moodycamel::details: classes, typedefs, functions, variables
    tu_local: list[str] = field(default_factory=list)      # … of which `static` functions (TU-local)
    tu_local_templates: list[str] = field(default_factory=list)  # … of which function templates
    c_api: list[str] = field(default_factory=list)         # c_api/concurrentqueue.h: types and functions
    macros: dict[str, str] = field(default_factory=dict)   # every #define -> header
    read_macros: set[str] = field(default_factory=set)     # configuration inputs the headers test


def scan(code: str, s: Surface):
    """Namespace-scope declarations of moodycamel and moodycamel::details."""
    stack: list[str | None] = []  # namespace names; None for any other brace
    last = 0
    for m in re.finditer(r'\bnamespace\s+(\w+)\s*\{|\{|\}|;', code):
        segment = ' '.join(code[last:m.start()].split())
        inside = [n for n in stack]
        at_ns = all(n is not None for n in inside)
        where = '::'.join(inside) if at_ns else None
        token = m.group(0)
        if token.startswith('namespace'):
            stack.append(m.group(1))
        elif token == '{':
            if where in ('moodycamel', 'moodycamel::details'):
                record(segment, where, s, definition=True)
            stack.append(None)
        elif token == '}':
            stack.pop()
        elif where in ('moodycamel', 'moodycamel::details'):
            record(segment, where, s, definition=False)
        last = m.end()


def record(segment: str, where: str, s: Surface, definition: bool):
    segment = re.sub(r'^(?:template\s*<.*?>\s*)+', lambda m: m.group(0), segment)
    is_template = segment.startswith('template')
    body = re.sub(r'^template\s*<[^{};]*?>\s*', '', segment)
    name = None
    m = re.match(r'(?:struct|class|union)\s+(?:\w+\s+)*?(\w+)\s*(?:<[^{]*>)?\s*(?::[^{]*)?$', body)
    if body.startswith('typedef') and definition:
        return  # `typedef union { … } name;`: the name follows the body
    if re.match(r'^\w+$', body) and not definition:
        name = body  # … here
    elif m:
        name = m.group(1)
    elif body.startswith('typedef'):
        name = re.findall(r'(\w+)\s*$', body)[0]
    elif body.startswith('using '):
        name = body.split()[1]
    else:
        # A declarator's name: not a member defined out of its class
        # (`ProducerToken::ProducerToken(…) : producer(…)`).
        head = body.split(') :')[0] + ')' if ') :' in body else body
        f = re.search(r'(?<![:\w])(?:\(\s*)?(\w+)\s*\)?\s*\((?:[^()]|\([^()]*\))*\)\s*(?:MOODYCAMEL_NOEXCEPT|noexcept|const)?\s*(?:->.*)?$', head)
        if f and re.search(r'\b\w+::' + f.group(1) + r'\s*\(', head):
            return
        if f:
            name = f.group(1)
            if where == 'moodycamel::details' and re.match(r'static\b', body):
                s.tu_local.append(name)
                if is_template:
                    s.tu_local_templates.append(name)
        else:
            v = re.match(r'(?:static\s+)?(?:const\s+)?[\w:<>]+\s+(\w+)\s*(?:=.*)?$', body)
            if v:
                name = v.group(1)
    if not name or name in ('MOODYCAMEL_ALIGNAS',):
        return
    (s.entities if where == 'moodycamel' else s.details).append(name)


def surface(upstream: Path) -> Surface:
    s = Surface()
    pp = Preprocessor(TARGET)
    for name in HEADERS:
        text = (upstream / name).read_text(encoding='utf-8')
        before = set(pp.defined_anywhere)
        pp.run(text)
        for macro in pp.defined_anywhere - before:
            s.macros.setdefault(macro, name)
        s.read_macros |= set(re.findall(r'#\s*(?:if|ifdef|ifndef|elif)\b[^\n]*?\b((?:MOODYCAMEL|MCDBGQ)_\w+)', text))
    scan('\n'.join(pp.active), s)
    c_api = strip_comments((upstream / C_API).read_text(encoding='utf-8'))
    for macro in re.findall(r'^\s*#\s*define\s+(\w+)', c_api, flags=re.M):
        s.macros.setdefault(macro, C_API)
    s.c_api = sorted(set(re.findall(r'typedef\s+[\w\s\*]+?\b(\w+)\s*;', c_api)) |
                     set(re.findall(r'MOODYCAMEL_EXPORT\s+[\w\s\*]+?\b(\w+)\s*\(', c_api)))
    s.entities = sorted(set(s.entities))
    s.details = sorted(set(s.details))
    s.tu_local = sorted(set(s.tu_local))
    s.tu_local_templates = sorted(set(s.tu_local_templates))
    s.read_macros -= set(s.macros)
    return s


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    sf = surface(fetch(root))
    print('moodycamel:', ' '.join(sf.entities))
    print('details:', ' '.join(sf.details))
    print('TU-local:', ' '.join(sf.tu_local), '| templates:', ' '.join(sf.tu_local_templates))
    print('C API:', ' '.join(sf.c_api))
    print('macros:', ' '.join(sorted(sf.macros)))
    print('configuration inputs:', ' '.join(sorted(sf.read_macros)))

#!/usr/bin/env python3
"""Audit the modules against concurrentqueue's headers (the pinned release's).

- src/concurrentqueue.cppm exports every entity of namespace moodycamel
  (outside moodycamel::details) that concurrentqueue.h,
  blockingconcurrentqueue.h and lightweightsemaphore.h declare, every
  overload of swap included, and nothing else.
- Every name of moodycamel::details is listed below as adapted (its TU-local
  helpers, declared in mcpplibs::mwrap::concurrentqueue::details) or as the
  queue's implementation, with the reason it is not exported.
- src/c_api/c_api.cppm exports every type and function of c_api/concurrentqueue.h.
- Every macro the headers define is mapped in src/macros.cppm or kept as a
  macro in <mwrap/concurrentqueue/macros.hpp> with its reason; every
  configuration input they test is mapped.
"""
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inventory import fetch, surface  # noqa: E402

SIDE_HEADER = {
    'MOODYCAMEL_NOEXCEPT': 'a specifier (noexcept, or nothing without exceptions)',
    'MOODYCAMEL_NOEXCEPT_CTOR': 'an expression over its argument expression (the configuration point, see config.hpp)',
    'MOODYCAMEL_NOEXCEPT_ASSIGN': 'an expression over its argument expression (the configuration point, see config.hpp)',
    'MOODYCAMEL_DELETE_FUNCTION': 'a function definition (= delete)',
    'MOODYCAMEL_ALIGNAS': 'an attribute-specifier (alignas)',
    'MOODYCAMEL_ALIGNOF': 'an operator applied to a type (alignof)',
    'MOODYCAMEL_ALIGNED_TYPE_LIKE': 'a type with an alignment specifier',
    'MOODYCAMEL_TRY': 'control flow (try, or if constexpr (true))',
    'MOODYCAMEL_CATCH': 'control flow (catch, or else if constexpr (false))',
    'MOODYCAMEL_RETHROW': 'control flow (throw;)',
    'MOODYCAMEL_THROW': 'control flow (throw expr)',
    'MOODYCAMEL_CONSTEXPR_IF': 'control flow (if constexpr)',
    'MOODYCAMEL_MAYBE_UNUSED': 'an attribute ([[maybe_unused]])',
    'MOODYCAMEL_THREADLOCAL': 'a storage class (__thread / thread_local)',
    'MOODYCAMEL_NO_TSAN': 'an attribute (no_sanitize("thread"))',
    'MOODYCAMEL_EXPORT': "the C API's linkage annotation (dllimport / dllexport)",
}

# moodycamel::details: the TU-local helpers adapted under the same names.
ADAPTED = {'thread_id', 'likely', 'unlikely', 'hash_thread_id', 'circular_less_than', 'ceil_to_pow_2', 'thread_id_t'}

NOT_EXPORTED = {
    'align_for': 'TU-local template over any element type: the queue instantiates upstream\'s own',
    'swap_relaxed': 'TU-local template over any atomic: the queue instantiates upstream\'s own',
    'nomove': 'TU-local template over any type: the queue instantiates upstream\'s own',
    'deref_noexcept': 'TU-local template over any iterator: the queue instantiates upstream\'s own',
    'nomove_if': "the queue's implementation (bulk enqueue's copy-or-move)",
    'ConcurrentQueueProducerTypelessBase': "the queue's implementation (the producers' base, reachable through the tokens)",
    'ThreadExitListener': "the queue's implementation (thread-exit recycling of implicit producers)",
    'ThreadExitNotifier': "the queue's implementation (thread-exit recycling of implicit producers)",
    'Semaphore': "LightweightSemaphore's implementation (the OS semaphore)",
    'thread_id_converter': "the queue's implementation (thread-id hashing)",
    'hash_32_or_64': "the queue's implementation (thread-id hashing)",
    '_hash_32_or_64': "the queue's implementation (thread-id hashing)",
    'const_numeric_max': "the queue's implementation (index arithmetic)",
    'static_is_lock_free': "the queue's implementation (is_lock_free())",
    'static_is_lock_free_num': "the queue's implementation (is_lock_free())",
    'is_trivially_destructible': "the queue's implementation (a type-trait shim)",
    'identity': "the queue's implementation (a type identity)",
    'max_align_t': "the queue's implementation (block alignment)",
    'std_max_align_t': "the queue's implementation (block alignment)",
    'invalid_thread_id': "TU-local constant of the queue's implementation (the empty hash slot)",
    'invalid_thread_id2': "TU-local constant of the queue's implementation (the empty hash slot)",
}


def exported(text: str, pattern: str) -> set[str]:
    return set(re.findall(pattern, text[text.index('export module'):], flags=re.M))


def main():
    root = Path(__file__).resolve().parents[1]
    sf = surface(fetch(root))
    problems = []
    cppm = (root / 'src/concurrentqueue.cppm').read_text(encoding='utf-8')
    have = exported(cppm, r'^using moodycamel::(\w+);')
    if set(sf.entities) - have:
        problems.append('src/concurrentqueue.cppm lacks: ' + ', '.join(sorted(set(sf.entities) - have)))
    if have - set(sf.entities):
        problems.append('src/concurrentqueue.cppm exports what the headers do not declare: ' + ', '.join(sorted(have - set(sf.entities))))
    adaptations = cppm[cppm.index('export namespace mcpplibs::mwrap::concurrentqueue::details'):]
    adaptations = adaptations[:adaptations.index('}  // namespace')]
    adapted = set(re.findall(r'\b(\w+)\s*\(', adaptations)) | set(re.findall(r'using\s+(\w+)\s*=', adaptations))
    if adapted != ADAPTED:
        problems.append(f'adapted helpers differ from the list: {sorted(adapted ^ ADAPTED)}')
    details = set(sf.details)
    unaccounted = details - ADAPTED - NOT_EXPORTED.keys()
    if unaccounted:
        problems.append('moodycamel::details names neither adapted nor listed: ' + ', '.join(sorted(unaccounted)))
    stale = (ADAPTED | NOT_EXPORTED.keys()) - details
    if stale:
        problems.append('listed but not in moodycamel::details: ' + ', '.join(sorted(stale)))
    tu_local = set(sf.tu_local) - ADAPTED
    if tu_local - NOT_EXPORTED.keys():
        problems.append('TU-local helpers neither adapted nor listed: ' + ', '.join(sorted(tu_local - NOT_EXPORTED.keys())))
    c_api = exported((root / 'src/c_api/c_api.cppm').read_text(encoding='utf-8'), r'export using ::(\w+);')
    if c_api != set(sf.c_api):
        problems.append(f'the C API module differs from c_api/concurrentqueue.h: {sorted(c_api ^ set(sf.c_api))}')
    mapped = exported((root / 'src/macros.cppm').read_text(encoding='utf-8'), r'inline constexpr bool (\w+) =')
    macros = set(sf.macros)
    unaccounted = macros - mapped - SIDE_HEADER.keys()
    if unaccounted:
        problems.append('macros neither mapped nor kept as macros: ' + ', '.join(sorted(unaccounted)))
    both = SIDE_HEADER.keys() & mapped
    if both:
        problems.append('kept as macros yet mapped: ' + ', '.join(sorted(both)))
    stale = SIDE_HEADER.keys() - macros
    if stale:
        problems.append('listed but not defined by the headers: ' + ', '.join(sorted(stale)))
    if sf.read_macros - mapped:
        problems.append('configuration inputs not mapped: ' + ', '.join(sorted(sf.read_macros - mapped)))
    if mapped - macros - sf.read_macros:
        problems.append('mapped but neither defined nor tested by the headers: ' + ', '.join(sorted(mapped - macros - sf.read_macros)))
    if problems:
        sys.exit('\n'.join(problems))
    print(f'{len(sf.entities)} names of moodycamel exported ({", ".join(sf.entities)}); moodycamel::details: '
          f'{len(details & ADAPTED)} adapted, {len(details - ADAPTED)} implementation; C API: {len(c_api)} names; '
          f'{len(macros)} macros: {len(macros & mapped)} mapped, {len(SIDE_HEADER)} kept as macros; '
          f'{len(sf.read_macros)} configuration inputs mapped')


if __name__ == '__main__':
    main()

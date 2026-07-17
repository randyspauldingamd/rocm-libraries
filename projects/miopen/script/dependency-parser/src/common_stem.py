#!/usr/bin/env python3
# Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT
"""Stem bridge (Option A) for the dependency parser.

Additive attribution pass that runs after the ninja-deps (include-graph) mapping.
It closes the common-`.cpp`-body gap: a change to a source like ``conv_common.cpp``
is not ``#include``d by any test, so the include graph maps it only to a
fixture-less key and it would fail open to the entire category filter.

Heuristic: a source ``foo.cpp`` almost always exposes its interface through a
sibling header ``foo.hpp`` that its consumers ``#include``. So we attribute
``foo.cpp`` to whatever the include graph already attributed ``foo.hpp`` to.

This operates purely on the mapping keys (already project-relative), so it does
not depend on the parser's path-normalization internals. It is additive and
idempotent -- it only unions executables into existing entries, never removes.

Known limitations (see the plan / symbol bridge for the exact alternative):
- Over-attributes: a test that includes ``foo.hpp`` but never calls ``foo.cpp``'s
  out-of-line symbols is still selected (wasted time, not a miss).
- Under-attributes: if ``foo.cpp``'s symbols are declared in a non-sibling header,
  the true consumers are missed. The symbol bridge (Option B) does not have this.
"""

_HEADER_EXTS = (".hpp", ".h", ".hxx", ".hh", ".hpp.in")


def apply(parser):
    """Union each source's sibling-header executable set into the source's entry."""
    f2e = parser.file_to_executables
    added = 0
    touched = 0
    for cpp in [k for k in list(f2e.keys()) if k.endswith(".cpp")]:
        stem = cpp[: -len(".cpp")]
        for ext in _HEADER_EXTS:
            header = stem + ext
            if header in f2e and f2e[header]:
                before = len(f2e[cpp])
                f2e[cpp].update(f2e[header])
                gained = len(f2e[cpp]) - before
                if gained:
                    added += gained
                    touched += 1
    print(
        f"[bridge:stem] sibling-header attribution: "
        f"{added} edges added across {touched} sources"
    )

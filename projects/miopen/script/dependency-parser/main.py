#!/usr/bin/env python3
# Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
# SPDX-License-Identifier: MIT

"""
Unified CLI for Ninja Dependency Analysis and Selective Testing

Features:
- Dependency parsing (from build.ninja)
- Selective test filtering (between git refs)
- Code auditing (--audit)
- Build optimization (--optimize-build)
"""

import argparse
import subprocess
import sys


def run_dependency_parser(args):
    from src.enhanced_ninja_parser import main as ninja_main

    sys.argv = ["enhanced_ninja_parser.py"] + args
    ninja_main()


def run_selective_test_filter(args):
    from src.selective_test_filter import main as filter_main

    sys.argv = ["selective_test_filter.py"] + args
    filter_main()


def main():
    parser = argparse.ArgumentParser(
        description="Unified Ninja Dependency & Selective Testing Tool"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # Dependency parsing
    parser_parse = subparsers.add_parser(
        "parse", help="Parse build.ninja and generate dependency mapping"
    )
    parser_parse.add_argument("build_ninja", help="Path to build.ninja")
    parser_parse.add_argument(
        "--ninja", help="Path to ninja executable", default="ninja"
    )
    parser_parse.add_argument(
        "--workspace-root", help="Path to workspace root", default=None
    )

    # Selective testing
    parser_test = subparsers.add_parser(
        "select", help="Selective test filtering between git refs"
    )
    parser_test.add_argument("depmap_json", help="Path to dependency mapping JSON")
    parser_test.add_argument("ref1", help="Source git ref")
    parser_test.add_argument("ref2", help="Target git ref")
    parser_test.add_argument(
        "--all", action="store_true", help="Include all executables"
    )
    parser_test.add_argument(
        "--test-prefix",
        action="store_true",
        help="Only include executables starting with 'test_'",
    )
    parser_test.add_argument(
        "--output", help="Output JSON file", default="bin/miopen_gtest_tests_to_run.json"
    )
    parser_test.add_argument(
        "--fixturemap", help="Optional path to file containing the test <-> gtest fixture mapping", default=""
    )

    # Code auditing
    parser_audit = subparsers.add_parser(
        "audit", help="List all files and their dependent executables"
    )
    parser_audit.add_argument("depmap_json", help="Path to dependency mapping JSON")

    # Build optimization
    parser_opt = subparsers.add_parser(
        "optimize", help="List affected executables for changed files"
    )
    parser_opt.add_argument("depmap_json", help="Path to dependency mapping JSON")
    parser_opt.add_argument("changed_files", nargs="+", help="List of changed files")

    args = parser.parse_args()

    if args.command == "parse":
        parse_args = [args.build_ninja, args.ninja]
        if args.workspace_root:
            parse_args.append(args.workspace_root)
        run_dependency_parser(parse_args)
    elif args.command == "select":
        if args.ref1 == "0":
            ref1 = subprocess.run(['git', 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
        if args.ref2 == "0":
            ref2 = subprocess.run(['git', 'merge-base', 'origin/develop', ref1], capture_output=True, text=True).stdout.strip()
        filter_args = [args.depmap_json, ref1, ref2]
        if args.test_prefix:
            filter_args.append("--test-prefix")
        if args.all:
            filter_args.append("--all")
        if args.output:
            filter_args += ["--output", args.output]
        if args.fixturemap:
            filter_args += ["--fixturemap", args.fixturemap]
        run_selective_test_filter(filter_args)
    elif args.command == "audit":
        run_selective_test_filter([args.depmap_json, "--audit"])
    elif args.command == "optimize":
        run_selective_test_filter(
            [args.depmap_json, "--optimize-build"] + args.changed_files
        )
    else:
        parser.print_help()


if __name__ == "__main__":
    main()

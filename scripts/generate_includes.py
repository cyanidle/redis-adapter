#!/usr/bin/env python3
import logging
import os
from pathlib import Path
from argparse import ArgumentParser
from typing import IO, Generator, List, Optional
from pri_directory import PriDirectory

def main():
    """
        Recursively .cpp and .h(pp) files to directories` .pri file.
        You can specify exclude dirs (default: build, lib)
    """
    parser = ArgumentParser(
                prog='./scripts/generate_includes.py',
                description=main.__doc__,
                epilog='Bereg foreva'
            )
    parser.add_argument(
        "-d",
        "--workdir",
        default="./src",
        dest="cwd",
        help="Directory to start recursion from"
        )
    parser.add_argument(
        "-e",
        "--excludes",
        default="build lib",
        metavar='E',
        nargs='*',
        help="Excluded directory names",
        dest="excluded"
        )
    args = parser.parse_args()
    logging.basicConfig(
        handlers=[logging.StreamHandler()],
        level=logging.NOTSET,
        format='%(levelname)-8s %(name)-12s %(lineno)-5s %(message)s'
        )
    cwd = args.cwd
    exc = args.excluded.split()
    workdir = Path(cwd)
    root = PriDirectory(workdir, exc, is_root=True)
    root.recurse()

if __name__ == "__main__":
    main()
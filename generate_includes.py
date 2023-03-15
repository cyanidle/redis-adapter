#!/usr/bin/env python3
import os
from pathlib import Path
from argparse import ArgumentParser
from typing import IO, Generator, List, Optional
from scripts.pri_directory import PriDirectory

def main():
    """
        Recursively add directories with .cpp and .h(pp) files to <output>.pri file. 
        You can specify exclude dirs (default: build, lib)
    """
    parser = ArgumentParser(
                prog='QMake helper script',
                description=__doc__,
                epilog='Bereg foreva'
            )
    parser.add_argument("-d", "--workdir", default="./src", dest="cwd")
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
    cwd = args.cwd
    exc = args.excluded.split()
    workdir = Path(cwd)
    root = PriDirectory(workdir, exc, is_root=True)
    root.recurse()

if __name__ == "__main__":
    main()
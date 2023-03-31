#!/usr/bin/env python3
from argparse import ArgumentParser
import logging

# TODO: create class which performs template mapping, checking mmissing params!

def start(template_path: str):
    pass

def main():
    """
        Create Empty project, which includes sources and header of Bereg directly
    """
    parser = ArgumentParser(
                prog='./scripts/generate_includes.py',
                description=main.__doc__,
                epilog='Bereg foreva'
            )
    parser.add_argument(
        "project",
        help="Project name",
        required=True
        )
    parser.add_argument(
        "-d",
        "--workdir",
        default=".",
        dest="cwd",
        help="Directory of project"
        )
    parser.add_argument(
        "-t",
        "--template",
        default="./src/lib/redis-adapter/project_templates/full_build.pro.template",
        dest="template",
        help="Template path"
        )
    args = parser.parse_args()
    cwd: str = args.cwd
    project: str = args.project
    template: str = args.template
    logging.basicConfig(
        handlers=[logging.StreamHandler()],
        level=logging.NOTSET,
        format='%(levelname)-8s %(name)-12s %(lineno)-5s %(message)s'
        )
    start(cwd)
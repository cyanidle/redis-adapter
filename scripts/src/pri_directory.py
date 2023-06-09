from logging import getLogger
from pathlib import Path
from typing import List

log = getLogger("sources-directory")

class PriDirectory:
    def __init__(self, dir: Path, exc: List[str], is_root = False) -> None:
        self.is_root = is_root
        self.dir = dir
        self.children: List[PriDirectory] = []
        self.headers: List[Path] = []
        self.sources: List[Path] = []
        for child in self.dir.iterdir():
            if not child.is_dir() and (child.suffix ==".cpp" or child.suffix == ".c"):
                self.sources.append(child)
            if not child.is_dir() and (child.suffix ==".hpp" or child.suffix == ".h"):
                self.headers.append(child)
            if child.is_dir() and not child.name in exc:
                self.children.append(PriDirectory(child, exc))
        def sort(src: List[str]):
            src = list(reversed(sorted(src)))
        sort(self.headers)
        sort(self.sources)
    def __repr__(self) -> str:
        return repr(self.dir)
    @property
    def pri_file(self) -> Path:
        return self.dir.joinpath(f"{self.dir.parts[-1]}.pri")
    def fill_pri_file(self):
        log.info(f"Filling {self.pri_file}...")
        with open(self.pri_file, "w") as file:
            file.truncate()
            file.write("SOURCES+=")
            file.writelines(map(format_file, self.sources))
            file.write("\n")
            file.write("HEADERS+=")
            file.writelines(map(format_file, self.headers))
            file.write("\n")
            file.writelines(map(format_child, self.children))
    def recurse(self):
        self.fill_pri_file()
        for child in self.children:
            child.recurse()
    
def format_file(file: Path):
    return f" \\\n   $$PWD/{file.parts[-1]}"

def format_child(child: PriDirectory):
    return f"\ninclude($$PWD/{child.dir.parts[-1]}/{child.pri_file.parts[-1]})"

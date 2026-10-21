"""Reject excluded content in the tracked file list before publication."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
result = subprocess.run(["git", "-C", str(root), "ls-files", "-z"],
                        check=True, stdout=subprocess.PIPE)
paths = [p.decode("utf-8") for p in result.stdout.split(b"\0") if p]
if not paths:
    raise SystemExit("No tracked files; stage the source tree before running this check.")
blocked_dirs = {"civ5-assets", "assets", "captures", "research", "packages", "mod-cache", "build", ".worktrees", ".idea"}
blocked_suffixes = {".exe", ".dll", ".pdb", ".obj", ".o", ".a", ".lib", ".dds", ".fpk", ".gr2", ".civ5save", ".dmp", ".bin"}
errors = []
for name in paths:
    path = Path(name)
    if path.parts[0].lower() in blocked_dirs or ".worktrees" in path.parts:
        errors.append(f"Excluded directory: {name}")
    if path.suffix.lower() in blocked_suffixes:
        errors.append(f"Excluded binary/game input: {name}")
    if path.suffix.lower() in {".zip", ".7z"} and not name.startswith("civ5/tests/data/mod-archive-"):
        errors.append(f"Archive outside synthetic test allowlist: {name}")
    data = (root / name).read_bytes()
    if data.startswith(b"version https://git-lfs.github.com/spec/v1"):
        errors.append(f"Unexpected Git LFS pointer: {name}")
if errors:
    print("\n".join(errors), file=sys.stderr)
    raise SystemExit(1)
print(f"Public tree policy passed: {len(paths)} tracked files.")

#!/usr/bin/env python3
"""Enforce the single-authority Nitori v2 header, Document and global skill layout."""

from __future__ import annotations

from pathlib import Path
import subprocess


ROOT = Path("/home/tnuzy/NitoriSTL")
HEADER = ROOT / "v2" / "Nitori.h"
DOCUMENT = ROOT / "NITORI_DOCUMENT.md"
SKILL = Path("/home/tnuzy/.codex/skills/nitori-competitive-programming-v2")
OLD_GLOBAL_SKILL = Path("/home/tnuzy/.codex/skills/nitori-competitive-programming")
OLD_REPO_SKILL = ROOT / "skills" / "use-nitoristl"
VALIDATOR = Path("/home/tnuzy/.codex/skills/.system/skill-creator/scripts/quick_validate.py")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def run(*command: str) -> None:
    print("+", " ".join(command), flush=True)
    result = subprocess.run(command, cwd=ROOT)
    if result.returncode:
        raise SystemExit(result.returncode)


def main() -> int:
    require(HEADER.is_file(), f"missing canonical header: {HEADER}")
    require(DOCUMENT.is_file(), f"missing canonical Document: {DOCUMENT}")
    require(not OLD_GLOBAL_SKILL.exists(), f"old global skill survived: {OLD_GLOBAL_SKILL}")
    require(not OLD_REPO_SKILL.exists(), f"old repository skill survived: {OLD_REPO_SKILL}")

    duplicate_documents = [
        ROOT / "v2" / "API.md",
        ROOT / "v2" / "README.md",
        ROOT / "v2_unsafe" / "README.md",
        ROOT / "v2_src" / "README.md",
        ROOT / "test_v2" / "README.md",
    ]
    for path in duplicate_documents:
        require(not path.exists(), f"duplicate Nitori authority survived: {path}")

    expected_skill_files = {SKILL / "SKILL.md", SKILL / "agents" / "openai.yaml"}
    actual_skill_files = {path for path in SKILL.rglob("*") if path.is_file()}
    require(actual_skill_files == expected_skill_files,
            f"global skill must contain only SKILL.md and agents/openai.yaml: {actual_skill_files}")

    skill_text = (SKILL / "SKILL.md").read_text()
    document_text = DOCUMENT.read_text()
    for path in (HEADER, DOCUMENT):
        require(str(path) in skill_text, f"skill does not directly reference authority: {path}")
    require(str(HEADER) in document_text, "Document does not name the canonical checked header")
    require("skills/use-nitoristl" not in skill_text, "skill references the removed repository skill")

    run("python3", str(VALIDATOR), str(SKILL))
    run("python3", "tools/amalgamate_v2.py", "--check")
    print("Nitori v2 single-authority audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

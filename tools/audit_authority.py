#!/usr/bin/env python3
"""Enforce the single-authority Nitori X header, Document and skill layout."""

from __future__ import annotations

from pathlib import Path
import subprocess


ROOT = Path("/home/tnuzy/NitoriSTL")
HEADER = ROOT / "Nitori.h"
UNSAFE_HEADER = ROOT / "Nitori_unsafe.h"
DOCUMENT = ROOT / "NITORI_DOCUMENT.md"
SKILL = Path("/home/tnuzy/.codex/skills/nitori-x")
OLD_GLOBAL_SKILL = Path("/home/tnuzy/.codex/skills/nitori-competitive-programming")
OLD_V2_SKILL = Path("/home/tnuzy/.codex/skills/nitori-competitive-programming-v2")
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
    require(UNSAFE_HEADER.is_file(), f"missing unsafe header: {UNSAFE_HEADER}")
    require(DOCUMENT.is_file(), f"missing canonical Document: {DOCUMENT}")
    require(not OLD_GLOBAL_SKILL.exists(), f"old global skill survived: {OLD_GLOBAL_SKILL}")
    require(not OLD_V2_SKILL.exists(), f"old v2 skill survived: {OLD_V2_SKILL}")

    legacy_paths = [
        ROOT / "REPORT.md",
        ROOT / "V2_PLAN.md",
        ROOT / "Nitori_naive.h",
        ROOT / "v2",
        ROOT / "v2_unsafe",
        ROOT / "v2_src",
        ROOT / "test_v2",
        ROOT / "build",
        ROOT / "include",
        ROOT / "skills",
        ROOT / "tests",
    ]
    for path in legacy_paths:
        require(not path.exists(), f"legacy repository artifact survived: {path}")

    duplicate_documents = [
        ROOT / "API.md",
        ROOT / "src" / "README.md",
        ROOT / "test" / "README.md",
    ]
    for path in duplicate_documents:
        require(not path.exists(), f"duplicate Nitori authority survived: {path}")

    expected_skill_files = {SKILL / "SKILL.md", SKILL / "agents" / "openai.yaml"}
    actual_skill_files = {path for path in SKILL.rglob("*") if path.is_file()}
    require(actual_skill_files == expected_skill_files,
            f"global skill must contain only SKILL.md and agents/openai.yaml: {actual_skill_files}")

    skill_text = (SKILL / "SKILL.md").read_text()
    document_text = DOCUMENT.read_text()
    for path in (HEADER, UNSAFE_HEADER, DOCUMENT):
        require(str(path) in skill_text, f"skill does not directly reference authority: {path}")
    require(str(HEADER) in document_text, "Document does not name the canonical checked header")
    require(str(UNSAFE_HEADER) in document_text, "Document does not name the unsafe header")
    require(document_text.startswith("# Nitori X\n"), "Document title is not Nitori X")
    require("skills/use-nitoristl" not in skill_text, "skill references the removed repository skill")

    run("python3", str(VALIDATOR), str(SKILL))
    run("python3", "tools/amalgamate.py", "--check")
    print("Nitori X single-authority audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

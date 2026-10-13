#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "[1/4] Required cloud build files"
test -s cloud/xenon_probe.c
test -s cloud/Makefile
test -s .github/workflows/xenon-cloud.yml

echo "[2/4] Host C syntax check (not a Xenon target build)"
gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only cloud/xenon_probe.c

echo "[3/4] Workflow and Makefile sanity"
grep -q 'build-xenon-toolchain toolchain' .github/workflows/xenon-cloud.yml
grep -q 'build-xenon-toolchain libxenon' .github/workflows/xenon-cloud.yml
grep -q 'xenon-gcc' cloud/Makefile
grep -q 'xenon-objcopy' cloud/Makefile
grep -q 'dreamkai360_cloud_probe.elf32' cloud/Makefile

echo "[4/4] Summary"
printf '%s\n' 'PASS: required inputs exist' 'PASS: host C syntax is valid' 'PASS: cloud workflow references SDK build stages' 'NOTE: these checks do not prove the Xenon toolchain or emulator builds.'

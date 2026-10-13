#!/usr/bin/env bash
set -euo pipefail
set -x

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "[1/5] Required cloud build files"
for f in cloud/xenon_probe.c cloud/Makefile cloud/audit_elf.sh .github/workflows/xenon-cloud.yml; do
  test -s "$f"
done

echo "[2/5] Shell syntax checks"
bash -n cloud/audit_elf.sh
bash -n cloud/validate_source.sh

echo "[3/5] Host C syntax check (not a Xenon target build)"
gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only cloud/xenon_probe.c

echo "[4/5] Workflow and Makefile sanity"
grep -q 'build-xenon-toolchain' .github/workflows/xenon-cloud.yml
grep -q 'libxenon' .github/workflows/xenon-cloud.yml
grep -q 'xenon-gcc' cloud/Makefile
grep -q 'xenon-objcopy' cloud/Makefile
grep -q 'dreamkai360_cloud_probe.elf32' cloud/Makefile
grep -q 'audit_elf.sh' .github/workflows/xenon-cloud.yml

echo "[5/5] Summary"
printf '%s\n' \
  'PASS: required inputs exist' \
  'PASS: shell scripts parse' \
  'PASS: host C syntax is valid' \
  'PASS: cloud workflow references SDK build and ELF audit stages' \
  'NOTE: these checks do not prove the Xenon toolchain or emulator builds.'

#!/usr/bin/env bash
set -euo pipefail

elf="${1:-cloud/dreamkai360_cloud_probe.elf}"
flat="${2:-cloud/dreamkai360_cloud_probe.elf32}"

for f in "$elf" "$flat"; do
  if [ ! -s "$f" ]; then
    echo "ERROR: missing or empty ELF: $f" >&2
    exit 2
  fi
  file "$f"
  readelf -h "$f"
done

header="$(readelf -h "$elf")"
grep -Eq 'Class:[[:space:]]+ELF32' <<<"$header" || {
  echo "ERROR: target ELF is not ELF32" >&2
  exit 3
}
grep -Eq 'Data:[[:space:]]+2.s complement, big endian' <<<"$header" || {
  echo "ERROR: target ELF is not big-endian" >&2
  exit 4
}
grep -Eq 'Machine:[[:space:]]+PowerPC' <<<"$header" || {
  echo "ERROR: target ELF machine is not PowerPC" >&2
  exit 5
}

flat_header="$(readelf -h "$flat")"
grep -Eq 'Class:[[:space:]]+ELF32' <<<"$flat_header" || {
  echo "ERROR: flat ELF is not ELF32" >&2
  exit 6
}
grep -Eq 'Machine:[[:space:]]+PowerPC' <<<"$flat_header" || {
  echo "ERROR: flat ELF machine is not PowerPC" >&2
  exit 7
}

sha256sum "$elf" "$flat"
echo "PASS: ELF format audit completed. This does not establish console execution or game compatibility."

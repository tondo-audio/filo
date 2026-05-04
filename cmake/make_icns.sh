#!/usr/bin/env bash
set -euo pipefail

SRC="$1"
DEST="$2"

TMP="$(mktemp -d)/Filo.iconset"
mkdir -p "$TMP"

for sz in 16 32 128 256 512; do
  sz2=$((sz*2))
  sips -z "$sz"  "$sz"  "$SRC" --out "$TMP/icon_${sz}x${sz}.png" >/dev/null
  sips -z "$sz2" "$sz2" "$SRC" --out "$TMP/icon_${sz}x${sz}@2x.png" >/dev/null
done

iconutil -c icns "$TMP" -o "$DEST"

#!/usr/bin/env bash
# Runs Apple's auval and pluginval (strictness 5) on the installed plugins.
# Downloads pluginval into tools/ on first use.
#
#   scripts/validate.sh
set -euo pipefail
cd "$(dirname "$0")/.."

PLUGINVAL_VERSION=v1.0.4
PV=tools/pluginval.app/Contents/MacOS/pluginval
if [[ ! -x "$PV" ]]; then
  mkdir -p tools
  curl -sSL -o tools/pluginval.zip \
    "https://github.com/Tracktion/pluginval/releases/download/${PLUGINVAL_VERSION}/pluginval_macOS.zip"
  unzip -q -o tools/pluginval.zip -d tools && rm tools/pluginval.zip
fi

VST3="$HOME/Library/Audio/Plug-Ins/VST3/scqwatcha.vst3"
AU="$HOME/Library/Audio/Plug-Ins/Components/scqwatcha.component"
mkdir -p build/validation

echo "== auval"
auval -strict -v aumu Scq1 Scqw | tee build/validation/auval.log | tail -3

echo "== pluginval VST3 (strictness 5)"
"$PV" --strictness-level 5 --validate-in-process --output-dir build/validation --validate "$VST3" \
  | tee build/validation/pluginval-vst3.log | tail -2

echo "== pluginval AU (strictness 5)"
"$PV" --strictness-level 5 --validate-in-process --output-dir build/validation --validate "$AU" \
  | tee build/validation/pluginval-au.log | tail -2

echo "All validators passed."

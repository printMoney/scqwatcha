#!/usr/bin/env bash
# Builds "scqwatcha.vst3", "scqwatcha.component" (AU) and the
# standalone app as universal binaries (Apple Silicon + Intel), runs the engine
# tests, and copies the results to dist/.
#
#   scripts/build.sh            Release build, installs into ~/Library/Audio/Plug-Ins
#   scripts/build.sh --no-install
set -euo pipefail
cd "$(dirname "$0")/.."

INSTALL=ON
[[ "${1:-}" == "--no-install" ]] && INSTALL=OFF

ARGS=(-DCMAKE_BUILD_TYPE=Release -DTS_COPY_PLUGINS="$INSTALL")
# Use a local JUCE checkout if present (faster, works offline).
if [[ -d external/JUCE ]]; then
  ARGS+=(-DFETCHCONTENT_SOURCE_DIR_JUCE="$PWD/external/JUCE")
fi

cmake -S . -B build "${ARGS[@]}"
cmake --build build --config Release -j "$(sysctl -n hw.ncpu)"

echo "== Engine tests"
./build/TurntableTests_artefacts/Release/TurntableTests

OUT=build/TurntableSampler_artefacts/Release
rm -rf dist && mkdir -p dist
cp -R "$OUT/VST3/scqwatcha.vst3" dist/
cp -R "$OUT/AU/scqwatcha.component" dist/
cp -R "$OUT/Standalone/scqwatcha.app" dist/

echo "== Architectures"
lipo -archs "dist/scqwatcha.vst3/Contents/MacOS/scqwatcha"
lipo -archs "dist/scqwatcha.component/Contents/MacOS/scqwatcha"

if [[ "$INSTALL" == "ON" ]]; then
  # Make macOS notice the new AU.
  killall -9 AudioComponentRegistrar 2>/dev/null || true
fi
echo "Done. Plugins are in dist/"

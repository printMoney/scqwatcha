#!/usr/bin/env bash
# Builds the release downloads from dist/ (run scripts/build.sh first):
#   scqwatcha-<version>-macOS.pkg   installer: VST3 + AU + standalone app
#   scqwatcha-<version>-macOS.zip   the same three bundles for manual install
#   SHA256SUMS.txt
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION=$(sed -nE 's/^project\(TurntableSampler VERSION ([0-9.]+).*/\1/p' CMakeLists.txt)
NAME="scqwatcha-${VERSION}-macOS"
OUT=release
WORK=build/package
rm -rf "$WORK" "$OUT" && mkdir -p "$WORK/pkgs" "$OUT"

for b in "scqwatcha.vst3" "scqwatcha.component" "scqwatcha.app"; do
  [[ -d "dist/$b" ]] || { echo "Missing dist/$b - run scripts/build.sh first"; exit 1; }
done

# One component package per format.
component () { # id  bundle  install-location  [scripts-dir]
  local id=$1 bundle=$2 location=$3 scripts=${4:-}
  local root="$WORK/root-$id"
  mkdir -p "$root" && cp -R "dist/$bundle" "$root/"
  pkgbuild --analyze --root "$root" "$WORK/$id.plist" >/dev/null
  # Always install to the standard folder, never "relocate" to an old copy elsewhere.
  plutil -replace 0.BundleIsRelocatable -bool NO "$WORK/$id.plist"
  local extra=()
  [[ -n "$scripts" ]] && extra=(--scripts "$scripts")
  pkgbuild --root "$root" --component-plist "$WORK/$id.plist" --identifier "org.scqwatcha.pkg.$id" \
    --version "$VERSION" --install-location "$location" ${extra[@]+"${extra[@]}"} "$WORK/pkgs/$id.pkg" >/dev/null
}
component vst3 "scqwatcha.vst3" "/Library/Audio/Plug-Ins/VST3"
component au "scqwatcha.component" "/Library/Audio/Plug-Ins/Components" packaging/scripts-au
component app "scqwatcha.app" "/Applications"

sed "s/@VERSION@/$VERSION/g" packaging/distribution.xml > "$WORK/distribution.xml"
cp -R packaging/resources "$WORK/resources"
cp LICENSE "$WORK/resources/LICENSE.txt"
sed -i '' "s/@VERSION@/$VERSION/g" "$WORK/resources/"*.html

productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/resources" --package-path "$WORK/pkgs" \
  "$OUT/$NAME.pkg" >/dev/null

# Zip for manual installs.
ZIPDIR="$WORK/$NAME"
mkdir -p "$ZIPDIR"
cp -R "dist/scqwatcha.vst3" "dist/scqwatcha.component" "dist/scqwatcha.app" "$ZIPDIR/"
cp packaging/INSTALL.txt LICENSE "$ZIPDIR/"
(cd "$WORK" && ditto -c -k --keepParent "$NAME" "../../$OUT/$NAME.zip")

(cd "$OUT" && shasum -a 256 *.pkg *.zip > SHA256SUMS.txt)
ls -la "$OUT"

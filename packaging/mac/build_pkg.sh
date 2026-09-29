#!/bin/bash
# build the unsigned installer pkg from a Release build
#
# usage: packaging/mac/build_pkg.sh [build_dir]
#   build_dir defaults to build/mac. build it with the Release config first
#
# the pkg itself is not signed, the user clears gatekeeper once in the GUI.
# files the installer writes get no quarantine xattr, so the plugins load
# without any xattr -dr in a terminal
set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
readonly ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
readonly PRODUCT="L-DAT_LP_Mode"
readonly PKG_ID_BASE="cc.mirrorball.ldatlpmode.pkg"

build_dir="$(cd "${1:-$ROOT/build/mac}" && pwd)"
art="$build_dir/LDatLpMode_artefacts/Release"
au="$art/AU/$PRODUCT.component"
vst3="$art/VST3/$PRODUCT.vst3"
app="$art/Standalone/$PRODUCT.app"

for bundle in "$au" "$vst3" "$app"; do
    if [ ! -d "$bundle" ]; then
        echo "missing $bundle, build Release first" >&2
        exit 1
    fi
done

plist_get() {
    /usr/libexec/PlistBuddy -c "Print :$2" "$1/Contents/Info.plist"
}

# project(VERSION) stays the single source, read it back from the build.
# the .app is sure to carry LSMinimumSystemVersion, plugin bundles may not
version="$(plist_get "$app" CFBundleShortVersionString)"
min_os="$(plist_get "$app" LSMinimumSystemVersion)"
echo "version $version, macOS $min_os or newer"

work="$build_dir/pkg"
rm -rf "$work"
mkdir -p "$work/resources"

# stage one bundle, check its signature, build a component pkg
make_component() {
    local name="$1" bundle="$2" location="$3"
    local root="$work/root-$name"
    local plist="$work/$name.plist"
    local staged="$root/$(basename "$bundle")"

    mkdir -p "$root"
    ditto "$bundle" "$staged"
    # a quarantine xattr in the payload would come out on the user side too
    xattr -cr "$staged"

    codesign --verify --deep --strict "$staged"
    local info signer
    info="$(codesign -dvv "$staged" 2>&1)"
    signer="$(printf '%s\n' "$info" | awk -F= '/^Authority=/ { print $2; exit }')"
    if [ -z "$signer" ]; then
        echo "warning: $name is ad-hoc signed, TCC forgets the mic permission on every update" >&2
    else
        echo "$name: signed by $signer"
    fi

    # installer moves a relocatable bundle to wherever it finds one with the
    # same id, e.g. an old copy in ~/Library or the build tree. pin it.
    # --analyze leaves the key out, which means relocatable, so add it
    pkgbuild --analyze --root "$root" "$plist"
    local i=0
    while plutil -extract "$i" xml1 -o /dev/null "$plist" 2>/dev/null; do
        plutil -replace "$i.BundleIsRelocatable" -bool NO "$plist"
        i=$((i + 1))
    done
    if [ "$i" -eq 0 ]; then
        echo "pkgbuild --analyze found no bundle in $root" >&2
        exit 1
    fi

    pkgbuild --root "$root" \
        --component-plist "$plist" \
        --identifier "$PKG_ID_BASE.$name" \
        --version "$version" \
        --install-location "$location" \
        "$work/$name.pkg"
}

make_component au "$au" /Library/Audio/Plug-Ins/Components
make_component vst3 "$vst3" /Library/Audio/Plug-Ins/VST3
make_component standalone "$app" /Applications

# license pane. the bundles carry the same files in Contents/Resources
{
    cat "$ROOT/LICENSE"
    printf '\n\n'
    cat "$ROOT/resources/fonts/Host_Grotesk/OFL.txt"
} > "$work/resources/license.txt"

sed -e "s/@VERSION@/$version/g" -e "s/@MIN_OS@/$min_os/g" \
    "$SCRIPT_DIR/distribution.xml" > "$work/distribution.xml"

out="$build_dir/$PRODUCT-$version-mac.pkg"
rm -f "$out"
productbuild --distribution "$work/distribution.xml" \
    --package-path "$work" \
    --resources "$work/resources" \
    "$out"
echo "wrote $out"

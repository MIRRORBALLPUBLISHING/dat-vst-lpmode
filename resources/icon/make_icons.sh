#!/usr/bin/env bash
# build ldat.icns / ldat.ico and the ICON_BIG / ICON_SMALL pngs from the two svgs.
# run by hand on a mac after touching the svgs, outputs are committed so windows builds need none of these tools
set -euo pipefail

cd "$(dirname "$0")"

for tool in rsvg-convert magick iconutil; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "error: $tool not found. install it and run again" >&2
        exit 1
    fi
done

readonly kMaster=ldat_icon.svg
readonly kSmall=ldat_icon_small.svg
# sizes up to this many px come from the small svg
readonly kSmallMaxPx=32

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# render <px> <out>, picks the svg by size
render() {
    local src=$kMaster
    if (( $1 <= kSmallMaxPx )); then
        src=$kSmall
    fi
    rsvg-convert -w "$1" -h "$1" "$src" -o "$2"
}

# mac, iconutil wants exactly these names
iconset="$work/ldat.iconset"
mkdir "$iconset"
for pt in 16 32 128 256 512; do
    render "$pt" "$iconset/icon_${pt}x${pt}.png"
    render $((pt * 2)) "$iconset/icon_${pt}x${pt}@2x.png"
done
iconutil -c icns "$iconset" -o ldat.icns

# windows. magick 7.1.2 stores every entry as 32bit bmp, 256 too. valid ico, just ~400KB
ico_pngs=()
for px in 16 20 24 32 40 48 64 96 128 256; do
    render "$px" "$work/ico_$px.png"
    ico_pngs+=("$work/ico_$px.png")
done
magick "${ico_pngs[@]}" ldat.ico

# ICON_BIG / ICON_SMALL for juce_add_plugin. juceaide output gets replaced by the files above,
# these only keep JUCE's plist / rc / desktop.ini wiring alive
rsvg-convert -w 1024 -h 1024 "$kMaster" -o ldat_icon_1024.png
rsvg-convert -w 32 -h 32 "$kSmall" -o ldat_icon_small_32.png

echo "wrote ldat.icns ldat.ico ldat_icon_1024.png ldat_icon_small_32.png"

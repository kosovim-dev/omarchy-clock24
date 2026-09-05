#!/usr/bin/env bash
# Build the Clock24 QML module and install it into the Omarchy plugin dir.
#
# Layout produced at ~/.config/omarchy/plugins/kosovim-dev.clock24/:
#   manifest.json      Omarchy plugin manifest (kind: bar-widget)
#   BarWidget.qml      Entry point: bar pill that toggles the clock popup
#   Panel.qml          Popup content: Clock24 item host
#   Clock24/           Self-contained QML module (qmldir + shared libs + types)
#
# The Clock24/ subdirectory makes `import Clock24` resolve; QML_IMPORT_PATH
# (set in ~/.config/uwsm/env) also points at the plugin dir.

set -euo pipefail

PROJ_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJ_DIR}/build"
DEST="${HOME}/.config/omarchy/plugins/kosovim-dev.clock24"

cmake -S "${PROJ_DIR}" -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}"

MODULE_DIR="${BUILD_DIR}/Clock24"
rm -rf "${MODULE_DIR}"
mkdir -p "${MODULE_DIR}"

# qmldir + qt type info live at the build root for single-module projects.
cp "${BUILD_DIR}/qmldir" "${MODULE_DIR}/qmldir"
cp "${BUILD_DIR}/clock24plugin.qmltypes" "${MODULE_DIR}/clock24plugin.qmltypes"
cp "${BUILD_DIR}/libclock24plugin.so" "${MODULE_DIR}/"
cp "${BUILD_DIR}/libclock24pluginplugin.so" "${MODULE_DIR}/"

mkdir -p "${DEST}"
cp "${PROJ_DIR}/plugin/manifest.json" "${DEST}/manifest.json"
rm -rf "${DEST}/Clock24"
cp -r "${MODULE_DIR}" "${DEST}/Clock24"
cp "${PROJ_DIR}/plugin/BarWidget.qml" "${DEST}/BarWidget.qml"
cp "${PROJ_DIR}/plugin/Panel.qml" "${DEST}/Panel.qml"

echo
echo "Installed to ${DEST}"
echo "QML_IMPORT_PATH is expected in ~/.config/uwsm/env and is read by"
echo "omarchy-shell only when the session starts. After a re-login the"
echo "widget mounts automatically once it is in a bar slot. To add it"
echo "to the current bar: add { \"id\": \"kosovim-dev.clock24\" } to a slot in"
echo "~/.config/omarchy/shell.json bar.layout (hot-reloads on save)."
echo "Ongoing reload while iterating: omarchy-shell shell rescanPlugins"
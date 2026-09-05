import QtQuick
import Quickshell
import qs.Commons
import qs.Ui
import Clock24

// Popup hosting the 24-hour analog clock face. Anchored to the bar pill
// (BarWidget.qml) via KeyboardPanel; outside clicks and the shell's own
// popout coordinator dismiss it.
Panel {
  id: root
  moduleName: "kosovim-dev.clock24"
  ipcTarget: "kosovim-dev.clock24"
  manageIpc: false

  property var anchorItem: null
  property var hostWidget: null

  // The bar tracks the widget mounted in its slot (BarWidget.qml), not this
  // nested panel, so identify by the host widget exactly like omarchy.clock.
  readonly property var barIdentity: hostWidget || root

  // Location feeding the solar arcs, sunrise/sunset hands and solar-noon
  // axis. Read from the widget's shell.json entry by default (see
  // applyLocationSettings); Melbourne, Australia is the fallback.
  property real faceLatitude: -37.8136
  property real faceLongitude: 144.9631
  property real faceTimeZoneOffset: 10.0

  function applyLocationSettings() {
    faceLatitude = parseFloat(root.setting("latitude", -37.8136))
    faceLongitude = parseFloat(root.setting("longitude", 144.9631))
    faceTimeZoneOffset = parseFloat(root.setting("timeZoneOffset", 10.0))
  }

  Component.onCompleted: applyLocationSettings()
  onSettingsChanged: applyLocationSettings()

  function open() {
    root.controller.show()
  }

  function close() {
    root.controller.hide()
  }

  function toggle() {
    if (root.opened) root.close()
    else root.open()
  }

  KeyboardPanel {
    id: panel
    anchorItem: root.anchorItem
    owner: root.barIdentity
    bar: root.bar
    open: root.opened
    centerOnBar: true
    contentWidth: panel.fittedContentWidth(Style.space(560))
    contentHeight: panel.fittedContentHeight(Style.space(560))

    Clock24 {
      id: clock
      anchors.fill: parent
      updateIntervalMs: 250
      latitude: root.faceLatitude
      longitude: root.faceLongitude
      timeZoneOffset: root.faceTimeZoneOffset
    }
  }
}
import QtQuick
import Quickshell
import Quickshell.Wayland
import qs.Commons

// Chrome-less popup surface for the 24-hour clock. A transparent overlay
// window holding only the Clock24 item, with no card, background fill or
// border, so the dial floats directly over the desktop like the original
// standalone Clock24 app. Left-click anywhere outside the dial dismisses it.
PanelWindow {
  id: rootPopup

  required property Item anchorItem
  required property QtObject bar
  property var owner: null
  property int margin: Style.gapsOut
  property int gap: Style.gapsOut
  property int contentWidth: Style.space(560)
  property int contentHeight: Style.space(560)
  property bool centerOnBar: true
  property bool open: false

  default property alias contentItem: clockHolder.children

  readonly property var coordinatorKey: owner || rootPopup
  readonly property var anchorWindow: anchorItem ? anchorItem.QsWindow.window : null
  readonly property string barPos: bar ? bar.position : "top"
  readonly property real screenW: screen ? screen.width : 0
  readonly property real screenH: screen ? screen.height : 0
  readonly property real barW: anchorWindow ? anchorWindow.width : screenW
  readonly property real barH: anchorWindow ? anchorWindow.height : 0

  screen: anchorWindow ? anchorWindow.screen : null
  visible: open || clockHolder.opacity > 0
  color: "transparent"
  exclusionMode: ExclusionMode.Ignore

  WlrLayershell.namespace: "omarchy-clock24-popup"
  WlrLayershell.layer: WlrLayer.Overlay
  WlrLayershell.keyboardFocus: open ? WlrKeyboardFocus.OnDemand : WlrKeyboardFocus.None

  anchors {
    top: true
    bottom: true
    left: true
    right: true
  }

  onOpenChanged: {
    if (!bar) return
    if (open) bar.requestPopout(coordinatorKey)
    else if (bar.activePopout === coordinatorKey) bar.releasePopout(coordinatorKey)
  }

  // Center the popup over the bar for the current bar position, clamped to
  // the screen (mirrors KeyboardPanel.cardOrigin).
  readonly property point cardOrigin: {
    if (!anchorItem || !bar) return Qt.point(margin, margin)
    var x = 0, y = 0
    if (barPos === "top" || barPos === "bottom") {
      x = screenW / 2 - contentWidth / 2
      y = barPos === "bottom" ? screenH - barH - contentHeight - gap : barH + gap
    } else if (barPos === "left") {
      x = barW + gap
      y = screenH / 2 - contentHeight / 2
    } else {
      x = screenW - barW - contentWidth - gap
      y = screenH / 2 - contentHeight / 2
    }
    x = Math.max(margin, Math.min(x, screenW - contentWidth - margin))
    y = Math.max(margin, Math.min(y, screenH - contentHeight - margin))
    return Qt.point(Math.round(x), Math.round(y))
  }

  function close() {
    if (owner && "close" in owner) owner.close()
    else rootPopup.open = false
  }

  // Clicks anywhere outside the clock dismiss the popup.
  MouseArea {
    anchors.fill: parent
    enabled: rootPopup.open
    acceptedButtons: Qt.AllButtons
    onPressed: rootPopup.close()
  }

  // Clicks on the clock itself are swallowed so they don't bubble to the
  // dismissal area above.
  MouseArea {
    x: rootPopup.cardOrigin.x
    y: rootPopup.cardOrigin.y
    width: rootPopup.contentWidth
    height: rootPopup.contentHeight
    enabled: rootPopup.open
    acceptedButtons: Qt.AllButtons
    onPressed: {}
  }

  Item {
    id: clockHolder
    x: rootPopup.cardOrigin.x
    y: rootPopup.cardOrigin.y
    width: rootPopup.contentWidth
    height: rootPopup.contentHeight
    opacity: rootPopup.open ? 1.0 : 0

    Behavior on opacity {
      NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
    }
  }
}
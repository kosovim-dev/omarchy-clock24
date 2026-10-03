# Clock24 — 24-hour solar clock for Omarchy

A 24-hour analog clock as an Omarchy **bar widget**. The bar shows a small
`24` pill; clicking it opens a popup clock face that plots the day as a ring:
midnight at the bottom, sunrise and sunset on the left and right, noon at the
top, the current time on a sweep hand.

![Clock24 popup](clock24.png)

## Features

- Analog 24-hour dial with hour/minutes/secs, sweep-hand and static hands
- Selectable hour/minute hand shapes (`thin`, `arrow`, `monument`) via the
  `handShape` setting
- Sunrise / sunset hands and the day's light-arc band, recalculated daily
  for your location
- Solar noon axis drawn from the actual solar-noon offset for your
  longitude/timezone
- Location fully configurable from the widget's `shell.json` entry
  (`latitude`, `longitude`, `timeZoneOffset`) so the solar geometry is correct
  anywhere on Earth; the UTC offset follows the system clock (DST included)
  unless explicitly pinned
- Self-contained `Clock24/` QML module (C++ core), no runtime scripts
- Outside-click dismissal and the shell's popout coordinator integration
- IPC target (`omarchy-shell kosovim-dev.clock24 toggle …`)

## Requirements

- Omarchy shell (Quickshell-based bar) on a Qt 6 environment
- CMake ≥ 3.16, Ninja, a C++17 compiler
- Qt 6 with `Quick` and `Qml` development files

## Build & install

```sh
git clone https://github.com/kosovim-dev/omarchy-clock24
cd omarchy-clock24
./scripts/build.sh
```

`build.sh` configures and builds the CMake project, then installs the plugin
into `~/.config/omarchy/plugins/kosovim-dev.clock24/`:

```
kosovim-dev.clock24/
├── manifest.json       Omarchy plugin manifest (kind: bar-widget)
├── BarWidget.qml       Entry point: the "24" bar pill
├── Panel.qml           Popup content hosting the Clock24 item
└── Clock24/            Self-contained QML module (qmldir + shared libs)
```

Only files under `plugin/` and `src/` are the plugin proper — everything else
is build scaffolding. There is no separate packaging step; the installed
directory is the plugin.

> **Note:** the shell does not watch plugin sources. After editing QML or C++,
> run `omarchy-restart-shell` (or re-login) for the changes to take effect —
> `omarchy-shell shell rescanPlugins` only notices newly added plugin
> directories, not edits to entry QML.

## Upgrading

Updates are manual. `omarchy plugin update` only pulls plugins that are
installed as a git checkout under `~/.config/omarchy/plugins/<id>`, and this
plugin ships a *compiled* Qt6 module (`Clock24/libclock24plugin.so`), so a
straight pull can't deliver it. Get the new sources, rebuild, and reload:

```sh
git -C ~/omarchy-clock24-plugin pull
~/omarchy-clock24-plugin/scripts/build.sh
~/omarchy-clock24-plugin/scripts/reload-shell.sh
```

`reload-shell.sh` restarts the shell and verifies exactly one instance comes
back (plain `omarchy-restart-shell` can leave a duplicate bar behind when the
old instance's runtime socket is stale).

Because builds are required, upgrade and build failures are possible:

- The same build prerequisites as the initial install must be present (Qt6
  `Quick`/`Qml` dev files, CMake, Ninja, a C++17 compiler). On Arch that is
  roughly `pacman -S qt6-devel cmake ninja`; elsewhere the equivalent
  distribution packages.
- If a rebuild fails, the plugin keeps running the previous installed files —
  your bar is not left broken, but the upgrade did not apply.

## Setup & configuration

The `Clock24` QML module is resolved through `QML_IMPORT_PATH`. Export it from
`~/.config/uwsm/env` so it is present when the shell starts:

```sh
# ~/.config/uwsm/env
export QML_IMPORT_PATH=$HOME/.config/omarchy/plugins/kosovim-dev.clock24
```

Add the widget to a bar slot in `~/.config/omarchy/shell.json` — the plugin id
and its configuration live in the same entry, e.g. `bar.layout.center`:

```json
{
  "bar": {
    "layout": {
      "center": [
        { "id": "omarchy.clock" },
        { "id": "kosovim-dev.clock24", "latitude": 51.5072, "longitude": -0.1276, "timeZoneOffset": 1.0, "background": false, "handShape": "monument" },
        { "id": "kosovim-dev.todo" }
      ]
    }
  }
}
```

All keys are optional; the location falls back to Melbourne, Australia
(−37.8136, 144.9631), and the UTC offset follows the system clock (DST
included) unless overridden:

| Key               | Type    | Default     | Meaning                                |
|-------------------|---------|-------------|----------------------------------------|
| `latitude`        | number  | `-37.8136`  | Geodetic latitude, decimal degrees     |
| `longitude`       | number  | `144.9631`  | Longitude, decimal degrees             |
| `timeZoneOffset`  | number  | `auto`      | Manual offset from UTC in hours; omit to follow the system clock (DST included) |
| `background`      | boolean | `false`     | Opaque square behind the dial; `false` floats the clock transparently |
| `handShape`       | string  | `"thin"`    | Hour/minute hand style: `thin` (plain lines), `arrow` (triangular arrowhead tips) or `monument` (tapered wedge hands). The second hand always stays a thin sweep line |

`timeZoneOffset` shifts the solar-noon axis; latitude/longitude drive the
sunrise/sunset calculations and light-arc geometry. Set `timeZoneOffset`
only to pin the geometry to a timezone other than the system's (e.g.
tracking a remote location); by default it tracks the system clock through
DST transitions automatically.

### Hand shapes

The `handShape` key selects the outline style of the hour and minute hands:

- `"thin"` — plain round-cap lines, the original look (default)
- `"arrow"` — a slim shaft ending in a triangular arrowhead at the tip
- `"monument"` — a tapered wedge, wide at the pivot and broad along its
  length, closing with a short blunt tip that mirrors its counter-tail

The second hand always remains a thin sweep line regardless of `handShape`.
The digital time, moon-phase and date panels are drawn above the hands at
50% transparency, so the readouts stay readable whichever shape (or size)
of hands sweeps beneath them.

Saving `shell.json` hot-reloads the bar; the widget appears once the plugin
directory is in place and `QML_IMPORT_PATH` is set (a re-login picks up the
new `uwsm/env`).

> **Note:** after the first install you will need to log out and back in (or
> reboot) before the plugin will function — `QML_IMPORT_PATH` from `uwsm/env`
> is read when the compositor session starts, so it only reaches the shell
> after a fresh session.

## Controlling the popup

The bar-widget root exposes `open()`, `close()`, and `opened`, so the bar's
summon/hide/toggle routing works, and it registers an `IpcHandler`:

```sh
omarchy-shell kosovim-dev.clock24 open     # show the popup
omarchy-shell kosovim-dev.clock24 close    # hide the popup
omarchy-shell kosovim-dev.clock24 toggle   # flip it
```

## License

MIT — see [LICENSE](LICENSE).

Solar calculations come from `src/SunRise.{h,cpp}` (by Cyrus Rahman, subject
to Stephen Schmitt's copyright). Redistribution of that code must retain its
copyright notice, which is preserved in the sources.

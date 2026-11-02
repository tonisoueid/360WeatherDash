# AGENTS.md — 360WeatherDash

Operational guide for AI coding agents working in this repository (Claude Code,
Codex, Cursor and other agent tools read this file natively). Update it when
build steps or architecture change.

## Project

Qt 6 / C++20 desktop weather dashboard styled as a NOC-style instrument console. One
executable, `360WeatherDash`. Live data from the keyless Open-Meteo API. Custom-painted
UI (no bitmap assets). Ships to **three targets** from this repo: Linux (native build or
`build-linux.sh` tarball), Windows 10/11 x64 and macOS arm64 (cross-compiled — see *Packaging*).

Not under version control (no git history to consult; no tests; no linter configured).

## Build & run (Linux)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/360WeatherDash
```

Requires Qt 6 (Widgets + Network dev packages), CMake ≥ 3.21.

- New source files **must** be added to the explicit `add_executable` list in
  `CMakeLists.txt` (no globbing). The packaging scripts pick up new files automatically
  once CMakeLists lists them — nothing else to update.
- `CMAKE_AUTOMOC` is on: `Q_OBJECT` classes need no extra wiring.
- `if(WIN32)` sets `WIN32_EXECUTABLE` (no console window on Windows) — do not remove.

## Verifying without a GUI

The app has built-in headless modes; use them for any visual/behavioral check:

- `--offline` — link-down drill, no network calls (also disables the `R` rescan key)
- `--grab frame.png` — render one frame after the first scan completes (30 s fallback
  exit — give `--offline --grab` runs an outer timeout ≥ 45 s), then exit
- `--grab-admin <prefix>` — scripted admin-dialog add + save, screenshots to `<prefix>-*.png`
- `--icon FILE` — render the 256 px app icon, then exit

Always `QT_QPA_PLATFORM=offscreen` for headless runs. For interaction testing, use an
ad-hoc CMake harness in `/tmp` that links the real widgets and drives synthetic mouse/key
events (`sendEvent` press+release at `rect().center()`; during splitter drags recompute
global positions per step — the handle moves mid-drag; `widget->grab()` alone shows
alpha-garbage at transparent edges, so verify icons in full-window grabs).

**Visual QA honesty rule:** AI judgments of "centered/aligned" are unreliable — measure
bright-pixel spans per region instead of eyeballing. Card background is `#121820` vs panel
`#11161d`; use tolerance < 6 and run-gap < 6 to segment cards.

## Packaging (all three OSes)

| Target | Command | Artifact |
|---|---|---|
| Windows 10/11 x64 | `packaging/windows/build-win64.sh` | `dist/360WeatherDash-1.0.0-win64.zip` (40 MB) |
| macOS arm64 | `packaging/macos/build-macos.sh` | `dist/360WeatherDash-1.0.0-macos-arm64.zip` (24 MB) |
| Linux x86_64 | `packaging/linux/build-linux.sh` | `dist/360WeatherDash-1.0.0-linux-x86_64.tar.gz` (44 MB) |
| Linux (dev) | the plain cmake build above | `build/360WeatherDash` |

- All three packaging scripts run in Docker (Fedora 43 base; images build on first use). Zero
  source changes needed for either — if a change breaks cross-compilation, the change is
  wrong, not the scripts.
- Windows is runtime-verified under Wine (live scan 12/12 LIVE). macOS is **statically**
  verified only (otool closure/rpath/signature) but confirmed working on a Mac mini M4.
- macOS EULA caveat: osxcross uses Apple's SDK on non-Apple hardware — an explicit
  owner decision (Nov 2026) for personal builds. Never distribute the macOS artifact
  (ad-hoc signed, not notarized). Do not re-add `install_name_tool` post-link edits to
  the macOS exe — that invalidates the ad-hoc code signature and macOS reports the app
  as corrupt (this shipped broken once; rpath is baked at link time for exactly this reason).
- The Linux tarball bundles the Qt libs + plugins (glibc/GPU stack excluded; needs
  glibc ≥ 2.42, built on Fedora 43) behind a `360WeatherDash` launcher that sets
  `LD_LIBRARY_PATH`; `bin/qt.conf` points Qt at `../plugins`. The script smoke-tests
  the bundle (`--offline --grab`, offscreen) before archiving.
- **App icon**: painted by `src/AppIcon.h` (no bitmap assets committed). `packaging/common/gen-icons.sh OUTDIR`
  renders the PNG set + multi-size `icon.ico` with a host Qt harness; Windows passes the .ico via
  `-DAPP_ICON_ICO` (CMake embeds it through an `.rc`), Linux ships hicolor PNGs + `360weatherdash.desktop`
  + `install-desktop.sh` in the tarball (and `cmake --install` renders the 256 px icon), macOS builds its
  own .icns. Packaging now needs host Qt 6 Gui + CMake + Ninja + python3.
- `dist/` is build output. Windows zip must stay self-contained; macOS zip must use
  `-y` (framework symlinks are sealed by signatures — flattening breaks them).

## Architecture

Data flows one way: `StationRegistry` (roster) → `WeatherService` (polling) →
`MainWindow` (alert diffing, wiring) → `StationCard` / `TrendChart` / `EventLog`
(painting).

- **`src/StationData.h`** — shared header: data model (`Station`, `CurrentObs`,
  `HourlySeries`, `DailyEntry`, `StationData`), WMO code table (`decodeWmo`), default
  12-site roster (`stationList()`), and the watch rules (`evaluateAlerts`). To add or
  tune an alert, edit `evaluateAlerts` here.
- **`src/StationRegistry`** — editable roster persisted as `stations.json` in
  `QStandardPaths::AppConfigLocation` (`~/.config/360WeatherDash/` on Linux,
  `%APPDATA%` on Windows, `~/Library/Application Support/` on macOS). `setStations()`
  normalizes (trim + upper-case), validates, persists via `QSaveFile`, then emits
  `stationsChanged`. Roster is hot-swappable: samples survive when code+coords are
  unchanged; a `m_generation` counter retires in-flight replies from a superseded
  roster. **Station indexes change after a swap (`stationsReconfigured`) — never cache
  indexes across it.**
- **`src/WeatherService`** — round-robin polling, one in-flight request per site,
  `kStaggerMs = 450` apart, full sweep every `kRefreshSecs = 900` (15 min). Failures
  downgrade a site to Stale/Error but keep the last good sample. To change the sweep
  interval, edit **only** `kRefreshSecs` (both countdown arm sites read it).
- **`src/MainWindow`** — owns timers; diffs `evaluateAlerts` results between samples so
  the event log records raise **and** clear ("…NOMINAL") events. Hotkeys in
  `keyPressEvent`: `R` rescan (disabled offline), `1`–`9` select site, `Ctrl+E` site
  admin, `Ctrl+Q` quit.
- **`src/AdminDialog`** — edits a copy of the roster (field caps 18/6/16 chars for
  name/code/region); only `StationRegistry` persists. The only stylesheet user.
- **Rendering** — every instrument surface is custom `QPainter` (`StationCard`,
  `widgets/TrendChart`, `widgets/Panel`, `widgets/WeatherGlyph.h`); icon artwork is
  verbatim Bootstrap Icons v1.11.3 SVG path data (MIT) converted by a hand-written
  parser — never freehand icon geometry; pull the pinned-version `d` attribute when
  changing artwork. Colors, fonts, per-domain accents live in `Theme.h`.

### Invariants — preserve these in every change

1. **Missing-data convention**: every measurement starts NaN (doubles) or -1 (ints);
   the UI renders explicit gaps (`---`, dashed baselines), never zero. Check
   `std::isnan` / `< 0` before using any value. "Measured, never invented" is the
   product's core promise — never fake a reading.
2. **UI claims must be wired**: any countdown/status promise (e.g. "Next scan") must
   have a backing code path. This class of gap has shipped before.
3. **Trend strip is date-anchored**: the API uses `past_days=1`, so `daily[0]` is
   *yesterday*. Anchor to the station-local date (`current.timeIso.left(10)`), never
   to array indexes. Hour labels are `NNH` (midnight `00H`).
4. **Open-Meteo pressure**: use `pressure_msl` (QNH), never `surface_pressure` —
   station-altitude pressure reads misleadingly low at elevated sites.
5. **Calm UI**: cards repaint on a minute gate; anything blinking belongs in the
   window header, not per-card. New card elements must be static between minute ticks
   and data updates.

## Design system (owner-set rules — do not regress)

- **Multi-accent palette**: exactly one saturated accent per data domain — temperature
  orange, wind/precip blue, humidity violet, pressure/status green, storms purple,
  critical red (`Theme.h`). Never return to monochrome green or monospace-everything.
- **Legibility floor**: all text flows through `Theme::sans()/mono()` which applies
  ×1.2 scaling with a 10 px minimum (`f.setPixelSize(qMax(10, int(px*1.2+0.5)))`).
  Never hardcode pixel font sizes outside `Theme.h`.
- **Contrast tiers** stay bright on the charcoal chassis (`#0b0e13` cards-blue base);
  dim roles were deliberately brightened after owner complaints. Every scale/gauge
  carries labeled, meaningful numbers (units, reference values), not bare extremes.
- Card layout collision zones (default 384×288 card): chip band y≈56–91, temp gauge
  y≈96–196, centered metrics y≈123–186, compass lower-left. Re-check overlaps after
  any font/layout change with an offscreen `--grab`.

## Safety around running processes

- `timeout`'s SIGTERM does **not** kill 360WeatherDash. Kill leftover processes by pid
  found by walking `/proc/*/cmdline` for the app's flags — never `pkill -f` with a
  pattern that appears in your own shell's command line.
- A 360WeatherDash process you didn't launch may be the owner manually testing the
  binary — check cmdline/cwd before killing anything.
- OOM repro harness: `QT_QPA_PLATFORM=offscreen ./build/360WeatherDash --offline`
  under `ulimit -v`.

## Documents

- **`spec.md`** — canonical functional/UI requirements (FR-01…40, UI-01…72, exact
  error strings, threshold tables). When behavior and spec conflict, the spec states
  intent; consult it before planning features. `spec_opus.md` is the long-form variant.
- `README.md` — visual/feature description (omits some features; spec.md wins).
- `docs/servicenow-implementation-report.md`, `docs/outsystems-11-implementation-report.md`
  — research porting workstreams (reference only).

**These local-only documents (`spec.md`, `spec_opus.md`, `docs/`) are deliberately
unpublished — gitignored by owner decision (Nov 2026). They exist in the working
tree only; never `git add -f` them.**

## Working with the owner

The owner is expert-level: they reproduce bugs themselves, hand over diagnosis +
proposed fixes, and expect **verify-against-source, then apply** — not re-derivation.
They communicate design taste by attaching reference screenshots; extract the full
design system from them rather than cherry-picking colors. Probing questions about
runtime behavior ("is this hitting the API every second?") expect honest audits that
fix any gaps found.

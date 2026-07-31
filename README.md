# Serial Debug Console

A native C++/Qt6 desktop serial terminal built for embedded development. It is a
cleaner alternative to a bare serial monitor: timestamps, RX/TX tagging,
automatic log-level detection with colour and filtering, inter-message time
deltas, event markers, and session save/replay — the things you actually reach
for while debugging firmware over UART.

It is deliberately a *native* engineering tool: Qt Widgets, menus, toolbars, a
dense monospaced table, no web stack.

## Features

- **Serial connection** — pick port, baud, data bits, parity, stop bits; connect/disconnect.
- **Terminal table** — one row per line with `Time | Δt | Dir | Level | Message`, auto-scroll that follows the tail but yields when you scroll up to inspect history.
- **Log levels** — each RX line is classified (INFO / WARNING / ERROR / DEBUG) by scanning for common firmware tokens; colour-coded for fast scanning.
- **Filtering** — toggle any level on/off from the View menu; live text search box.
- **Time differences** — the Δt column shows `+250ms` / `+3.25s` since the previous message, handy for spotting timing bugs.
- **Markers** — insert a labelled divider into the stream (Ctrl+M) to note "reset pressed", "test 2", etc.
- **Save logs** — export the session to `.csv` (round-trips exactly) or a human-readable `.txt`.
- **Session replay** — load a saved `.csv` and play it back honouring the recorded inter-message timing.

## Requirements

- CMake ≥ 3.16
- A C++17 compiler
- Qt 6 with the **Widgets** and **SerialPort** modules (the optional tests also need **Test**)

## Building

### Linux

```bash
# Debian/Ubuntu: sudo apt install cmake g++ qt6-base-dev qt6-serialport-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/SerialDebugger
```

### Windows

Install Qt 6 via the official online installer (MSVC or MinGW), then from a
"Qt 6 … Command Prompt":

```bat
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2019_64
cmake --build build --config Release
build\SerialDebugger.exe
```

If CMake can't find Qt, point it at your install with
`-DCMAKE_PREFIX_PATH=<path-to>/Qt/6.x.x/<compiler>`.

### Running the tests

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Architecture

A simple layered design keeps serial I/O, data, and presentation separate:

```
        QSerialPort  (raw bytes)
             |
       SerialManager        emits complete RX/TX lines as signals; no GUI code
             |
   LogModel  +  Logger      LogModel holds LogMessages; Logger saves/loads
             |
      LogFilterProxy        level + text filtering (QSortFilterProxyModel)
             |
   TerminalView / MainWindow   Qt Widgets: table, toolbars, menus, status bar
```

### Files

```
src/
  main.cpp                      app entry point
  gui/
    MainWindow.{h,cpp}          window, toolbars, menus, wiring
    TerminalView.{h,cpp}        QTableView with monospace + tail-follow scroll
  serial/
    SerialManager.{h,cpp}       QSerialPort wrapper + line buffering
  logging/
    Logger.{h,cpp}              TXT/CSV save, CSV load (with proper escaping)
  models/
    LogMessage.h                LogMessage struct, enums, level classifier
    LogModel.{h,cpp}            QAbstractTableModel
    LogFilterProxy.{h,cpp}      QSortFilterProxyModel
tests/
  test_logger.cpp               CSV round-trip + level classification (Qt Test)
```

### Design notes

- **Model/view, not a text box.** Storing `LogMessage` objects in a
  `QAbstractTableModel` (rather than appending formatted strings to a text
  widget) is what makes filtering, colouring, and the Δt column clean. A
  professional tool like Wireshark is fundamentally a table, and this mirrors
  that.

- **Threading.** `QSerialPort` is asynchronous: it emits `readyRead()` on the
  event loop and never blocks, so the UI stays responsive without a worker
  thread. `SerialManager` contains no GUI code and communicates only through
  signals, so if you *want* to demonstrate the worker-thread pattern you can
  construct it without a parent and `moveToThread(new QThread)`, then drive
  `open`/`close`/`sendLine` via queued signal connections — no code changes to
  the class itself. For a debug console the single-threaded event-loop approach
  is the correct, simpler default.

- **Δt under filtering.** The delta is the real elapsed time since the previous
  *actual* message, computed in the source model. When a filter hides rows, the
  visible Δt therefore still reflects true elapsed time rather than the gap to
  the previous *visible* row — usually what you want when debugging timing.

## Possible extensions

Multiple simultaneous ports, live plotting of numeric fields, binary/hex view,
device command profiles, or a firmware-upload panel — each slots in behind the
existing signal/model boundaries without disturbing the core.

# SfizzTUI

A simple terminal interface (TUI) to browse, filter, and load SFZ files into the **Sfizz** engine using **JACK**.

## Features

* **SFZ Browser:** Navigate your instrument library directly from the terminal.
* **Tag Filtering:** Filter files using a bitmask-based system (Favorites, Piano, Bass, etc.).
* **Metadata Scraping:** Reads tags directly from comments inside `.sfz` files.
* **JACK Integration:** Connects to the JACK Audio Connection Kit for real-time loading.
* **Keyboard Focused:** Optimized for navigation via arrows, Enter, and (soon) MIDI.

## Metadata & Tags

The application scans the first few lines of your `.sfz` files for a specific tag comment. This allows for easy organization without external database files.

Example of a tagged `.sfz` file:
```sfz
// tags: piano, felt, ambient
<region> sample=piano_c4.wav
...
```

Existing tags list is hardcoded ATM since it uses bitmasking for efficiency;
*
## Requirements
* `sfizz` (library and headers)
* `jackd2`
* `ftxui`
* meson & ninja

## Build

```bash
meson setup build
meson compile -C build
./build/sfizz-tui
```
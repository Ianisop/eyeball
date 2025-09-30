# eyeball
Eyeball uses **inotify** to keep your downloads folder clean by automatically sorting files.
It runs as a lightweight CLI tool that forks into a background daemon. Eyeball moves files into their configured folders based on file extensions, keeping your `Downloads` directory tidy.

Configuration is handled via a file called `paths.config`.
Place the provided example config in `~/.config/eyeball/` and you’re ready to run.

![CI](https://github.com/Ianisop/eyeball/actions/workflows/ci.yml/badge.svg)

---
> Releases are availible as single binaries, but its recommended you clone and build to ensure a symlink is created, if you know what you're doing, go ahead
## Building

Eyeball requires **CMake** and **C++17**.
To build, use the provided script:

```bash
./build.sh
```

> [!WARNING]
> Make sure the build script has execute permissions before running it.

---

## Running

Start Eyeball with:

```bash
eyeball on
```

Stop it with:

```bash
eyeball off
```

Check its current status anytime with:

```bash
eyeball status
```

---

## Notes

* Currently **Linux only**.


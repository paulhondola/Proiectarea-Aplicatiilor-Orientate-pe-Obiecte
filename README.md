# Programarea Aplicatiilor Orientate pe Obiecte

## Curs UPT AC Calculatoare Anul 4

A command-line image filter tool that also demonstrates the C++ concepts.

## Prerequisites

On macOS, with Homebrew:

```bash
brew install cmake meson ninja llvm lefthook
```

The project uses C++26, or C++23 if the compiler doesn't offer it. The first build needs internet access to download CLI11 and Catch2. stb is included in `third_party/stb`.

## Build and test

Both build systems produce the same `paoo` executable and run the same tests.

### CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

```bash
cmake --build build --parallel
```

```bash
ctest --test-dir build --output-on-failure
```

The executable is `build/src/paoo`.

### Meson

```bash
meson setup target --buildtype=debug
```

```bash
meson compile -C target
```

```bash
meson test -C target --suite PAOO --print-errorlogs
```

The executable is `target/src/paoo`.

## Usage

Show the demos for every lab, or for just one:

```bash
paoo demo
```

```bash
paoo demo --lab 5
```

Apply filters to an image, in order:

```bash
paoo filter -i input.png -o output.png -f grayscale -f blur:2
```

| Filter | Effect |
| :--- | :--- |
| `grayscale` | Sets colour channels to luma and keeps alpha. |
| `invert` | Replaces each colour channel `v` with `255 - v` and keeps alpha. |
| `blur:<radius>` | Box blur with clamped edges. The radius is from 1 to 64. |

The input can be any format `stb_image` reads. The output format comes from its extension: `.png`, `.jpg`, `.jpeg` or `.bmp`.

## Where each lab is

| Lab | Run | Code |
| :--- | :--- | :--- |
| Lucrarea 3 | `paoo demo --lab 3` | [lab3.cpp](src/demos/lab3.cpp), [Image.cpp](src/Image.cpp) |
| Lucrarea 4 | `paoo demo --lab 4` | [lab4.cpp](src/demos/lab4.cpp), [Logger.cpp](src/Logger.cpp) |
| Lucrarea 5 | `paoo demo --lab 5` | [lab5.cpp](src/demos/lab5.cpp), [LabeledImage.cpp](src/LabeledImage.cpp) |

## Checks

`scripts/verify.sh` defines every check. With no arguments it runs them all and stops at the first failure.

```bash
scripts/verify.sh
```

To run one step, name it:

```bash
scripts/verify.sh sanitize
```

| Step | What it checks |
| :--- | :--- |
| `format` | `clang-format` on project files |
| `lint` | `clang-tidy` on project sources |
| `cmake-debug`, `cmake-release` | CMake build and tests |
| `meson-debug`, `meson-release` | Meson build and tests |
| `parity` | Every build prints the same demo checks (run the build steps first) |
| `sanitize` | Tests under ASan and UBSan, in both build systems |
| `leaks` | Memory leaks, using macOS `leaks --atExit --` |

Install the git hooks once:

```bash
lefthook install
```

The `pre-commit` hook formats staged files. The `pre-push` hook runs `format`, `lint`, `cmake-debug` and `meson-debug`. CI in [ci.yml](.github/workflows/ci.yml) runs on macOS, and covers every step except `parity`.

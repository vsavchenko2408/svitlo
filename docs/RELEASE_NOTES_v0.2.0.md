## Svitlo 0.2.0 — now on Windows

Svitlo (Ukrainian for *"light"*) is a free, open-source photo library and non-destructive RAW editor. This release brings it to **Windows** for the first time.

### What's new
- **Windows version.** A portable build for 64-bit Windows 10 and 11: unzip and run, no installation needed.
- **RAW files in folders with non-Latin names** (for example Cyrillic) open correctly on Windows.
- **Svitlo.exe has its own icon** and version information.
- **Automatic builds and tests.** Every change is now built and tested on Linux, macOS and Windows with GitHub Actions, and release files are created automatically.

Everything from 0.1.0 is included: photo library with ratings and flags, RAW support via LibRaw, white balance, exposure and tone controls, tone curve, crop and straighten, sharpening, vignette, before / after view, batch export, and an interface in English, Ukrainian, Danish, German and Polish.

### Downloads
| File | For |
|---|---|
| `Svitlo-0.2.0-x86_64.dmg` | macOS 12 Monterey or later (Intel Macs; Apple Silicon via Rosetta 2, not tested yet) |
| `Svitlo-0.2.0-windows-x64.zip` | Windows 10 / 11, 64-bit |

### Installing on macOS
1. Open the `.dmg` and drag **Svitlo** to **Applications**.
2. Svitlo is not signed with an Apple Developer certificate yet, so macOS blocks the first launch. Try to open it once, then go to **System Settings → Privacy & Security** and click **Open Anyway**. Alternatively, run:
   ```bash
   xattr -dr com.apple.quarantine /Applications/Svitlo.app
   ```

### Installing on Windows
1. Unzip `Svitlo-0.2.0-windows-x64.zip` anywhere, for example to `Documents\Svitlo`.
2. Run **Svitlo.exe**.
3. Svitlo is not code-signed yet, so Windows SmartScreen may show a warning on the first start: click **More info → Run anyway**.

Your original photos are never modified.

This is the first Windows release, so some things may not work perfectly yet. If you find a bug or have an idea, please open an [issue](https://github.com/vsavchenko2408/svitlo/issues). If you like Svitlo, you can support it on [Ko-fi](https://ko-fi.com/dermeister). Thank you!

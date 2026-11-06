## Svitlo 0.4.0 — colour mixer (HSL)

### What's new
- **Colour mixer (HSL).** A new **Colour Mixer** section in Develop changes colours one range at a time: red, orange, yellow, green, aqua, blue, purple and magenta. Switch between **Hue**, **Saturation** and **Luminance** with the buttons above the sliders. Make a sky deeper blue without touching skin tones, calm down a too-bright green, or turn autumn leaves more golden.
  - Colours between two ranges follow both sliders smoothly, so there are no hard edges.
  - Greys and whites stay neutral.
  - In **Black & white**, the Luminance sliders work like colour filters: darken blue to get a dramatic sky.
- Colour mixer settings are saved with your edits, copied with **Copy / Paste settings** and included in exports.

The core of the colour mixer was written by me, Vi, as my first feature in Svitlo, step by step with an AI tutor.

### Downloads
| File | For |
|---|---|
| `Svitlo-0.4.0-x86_64.dmg` | macOS 12 Monterey or later (Intel Macs; Apple Silicon via Rosetta 2, not tested yet) |
| `Svitlo-0.4.0-windows-x64.zip` | Windows 10 / 11, 64-bit |

### Installing
- **macOS:** open the `.dmg`, drag **Svitlo** to **Applications** and replace the old version. Svitlo is not signed with an Apple Developer certificate yet: if macOS blocks it, go to **System Settings → Privacy & Security** and click **Open Anyway**, or run `xattr -dr com.apple.quarantine /Applications/Svitlo.app`.
- **Windows:** unzip and run **Svitlo.exe**; you can replace the old folder. If SmartScreen warns you, click **More info → Run anyway**.

If you already use Svitlo 0.3.0, it will offer this update itself (**Help → Check for Updates…**).

Your catalog, edits and settings are kept when you update. Your original photos are never modified.

Found a bug or have an idea? Please open an [issue](https://github.com/vsavchenko2408/svitlo/issues). If you like Svitlo, you can support it on [Ko-fi](https://ko-fi.com/dermeister). Thank you!

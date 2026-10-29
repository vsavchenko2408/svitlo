## Svitlo 0.1.0 — first release

The first public version of **Svitlo** (Ukrainian for *"light"*): a free, open-source photo library and non-destructive RAW editor for macOS.

### Highlights
- Photo library with folders, star ratings, pick / reject flags, filters and search
- RAW support via LibRaw (Nikon, Canon, Sony, Fujifilm, Olympus, Panasonic, DNG and more), plus JPEG, PNG and TIFF
- White balance, exposure, contrast, highlights, shadows, whites, blacks, clarity, vibrance, saturation, black & white
- Tone curve, crop and straighten, sharpening, vignette, auto tone
- Before / after view, RGB histogram, undo / redo
- Batch export to JPEG, PNG or TIFF
- Interface in English, Ukrainian, Danish, German and Polish

### Requirements
- macOS 12 Monterey or later
- Intel Mac (Apple Silicon should work through Rosetta 2, not tested yet)

### Installing
1. Download `Svitlo-0.1.0-x86_64.dmg` below, open it and drag **Svitlo** to **Applications**.
2. Svitlo is not signed with an Apple Developer certificate yet, so macOS will block the first launch. Try to open it once, then go to **System Settings → Privacy & Security** and click **Open Anyway**. Alternatively, run:
   ```bash
   xattr -dr com.apple.quarantine /Applications/Svitlo.app
   ```

Your original photos are never modified.

Found a bug or have an idea? Please open an [issue](https://github.com/vsavchenko2408/svitlo/issues). If you like Svitlo, you can support it on [Ko-fi](https://ko-fi.com/dermeister). Thank you!

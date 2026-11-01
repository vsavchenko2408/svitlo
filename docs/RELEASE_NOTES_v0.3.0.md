## Svitlo 0.3.0 — light theme, faster export, update check

### What's new
- **Light theme.** Choose **View → Appearance → Dark, Light or Match System**. With *Match System*, Svitlo switches together with macOS or Windows, without a restart. This also fixes parts of the window staying dark (or light) when the system used the other appearance.
- **Remove a folder from the catalog.** Right-click a folder in the Library's folder list, or use **File → Remove Folder from Catalog…**. Your photo files are never deleted, and your edits stay in the `.svitlo.json` files next to the photos, so importing the folder again brings them back.
- **Faster batch export.** Several photos are now exported at the same time, which is noticeably faster for large batches of RAW files. Photos with the same file name from different folders get separate files (`DSC_0001.jpg`, `DSC_0001_2.jpg`).
- **Right-click menu in the photo grid:** Open in Develop, Export…, Remove from Catalog….
- **Update check.** Svitlo checks GitHub for a new version at startup (at most once a day) and can download the right file for your system. Turn it off in **Help → Check for Updates at Startup**, or check manually with **Help → Check for Updates…**. Only one anonymous request to `api.github.com` is sent.

**Tip:** to export many photos, select them in the Library (`⌘A` / `Ctrl+A` for all, `Shift`-click for a range, `⌘`/`Ctrl`-click for single photos) and press `⇧⌘E` / `Ctrl+Shift+E`.

### Downloads
| File | For |
|---|---|
| `Svitlo-0.3.0-x86_64.dmg` | macOS 12 Monterey or later (Intel Macs; Apple Silicon via Rosetta 2, not tested yet) |
| `Svitlo-0.3.0-windows-x64.zip` | Windows 10 / 11, 64-bit |

### Installing
- **macOS:** open the `.dmg`, drag **Svitlo** to **Applications** and replace the old version. Svitlo is not signed with an Apple Developer certificate yet: if macOS blocks it, go to **System Settings → Privacy & Security** and click **Open Anyway**, or run `xattr -dr com.apple.quarantine /Applications/Svitlo.app`.
- **Windows:** unzip and run **Svitlo.exe**; you can replace the old folder. If SmartScreen warns you, click **More info → Run anyway**.

Your catalog, edits and settings are kept when you update. Your original photos are never modified.

Found a bug or have an idea? Please open an [issue](https://github.com/vsavchenko2408/svitlo/issues). If you like Svitlo, you can support it on [Ko-fi](https://ko-fi.com/dermeister). Thank you!

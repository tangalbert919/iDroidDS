# nds4droid

>**WARNING! This is not the official branch, and lots of things are going on here. Use this at your own risk.**

nds4droid is a free (including ad-free) open source Nintendo DS emulator for Android smartphones. You can find it on the [Google Play Store](https://play.google.com/store/apps/details?id=com.opendoorstudios.ds4droid&hl=en).

All releases from [jquesnelle](https://github.com/jquesnelle) will come from him. All releases from this fork is not uploaded to the play store, but can be found in the releases section of this fork. APKs will be uploaded to the release section of this fork if I believe I need to.

This is a development branch of version 48 of nds4droid. Use at your own risk.

This emulator uses [DeSmuME](http://desmume.org) 0.9.11 as the core, as opposed to the older 0.9.8-0.9.10 (somewhere around there) core found in most emulators. A custom threaded interpreter and the Lightning JIT engine are also included.

### Issues

If you found an issue, please [make an issue ticket here](https://github.com/tangalbert919/nds4droid/issues/new). Your issue will be closed if it either:
* Has nothing to do with this emulator,
* Contains copyrighted stuff,
* or has something to do with adding support for unsupported devices.

### To-do list

* Increase performance overall.
* Improve the OpenGL ES 2.0 Renderer.
* Work on the OpenGL ES 3.0 Renderer.
* Fix the DeSmuME default renderer (the rasterizer).
* Update libraries to the latest version (if applicable).

### Known issues

* The rasterizer can fail during emulation (in fact, it fails very quickly).
* The OpenGL ES 2.0 Renderer does not support FBOs, resulting in incorrect rendering of some textures.
* No file is created when saving the game.

**DISCLAIMER:** nds4droid is licensed under the GNU General Public License, which means the software is provided as-is. The Nintendo DS, and the Nintendo DS logo are trademarks of Nintendo, Inc. Games that support the Nintendo DS are trademarks of Nintendo, Inc., and their respective companies and affiliates.

**We do not support piracy in any way. All ROMs that are found online have copyrights that last 75 years. Only use the games you physically own. Don't be a pirate. Have a nice day.**

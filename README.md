# nds4droid

>**WARNING! This is not the official branch, and lots of things are going on here. Use this at your own risk.**

nds4droid is a free (including ad-free) open source Nintendo DS emulator for Android smartphones. You can find it on the [Google Play Store](https://play.google.com/store/apps/details?id=com.opendoorstudios.ds4droid&hl=en).

All releases from [jquesnelle](https://github.com/jquesnelle) will come from him. All releases from this fork is not uploaded to the play store, but can be found in the releases section of this fork.

This is a development branch of version 48 of nds4droid. It is not recommended for normal use, as it may crash or do something incorrectly.

This emulator uses [DeSmuME](http://desmume.org) 0.9.11 as the core, as opposed to the older 0.9.8-0.9.10 (somewhere around there) core found in most emulators. This means significantly better performance than the other emulators out there.

### Issues

If you found an issue, please [make an issue ticket here](https://github.com/tangalbert919/nds4droid/issues/new). Your issue will be closed if it either:
* Has nothing to do with this emulator,
* Contains copyrighted stuff,
* or has something to do with adding support for unsupported devices.

### To-do list

* Increase performance overall.
* Improve the OpenGL ES 2.0 Renderer.
* Get the OpenGL ES 3.0 Renderer to start operating.
* Update libraries to the latest version (if applicable).

### Known issues

* The rasterizer can fail during emulation, particularly when used with a filter on Pokemon games. Without a filter, some textures do not render.
* The OpenGL ES 2.0 Renderer does not support FBOs, resulting in incorrect rendering of some textures.
* The OpenGL ES 3.0 Renderer does not enable on supported devices for some reason.
* No file is created when saving the game. This is a major issue.

**DISCLAIMER:** nds4droid is licensed under the GNU General Public License, which means the software is provided as-is. The Nintendo DS, and the Nintendo DS logo are trademarks of Nintendo, Inc. Games that support the Nintendo DS are trademarks of Nintendo, Inc., and their respective companies and affiliates.

**We do not support piracy in any way. All ROMs that are found online have copyrights that last 75 years. Only use the games you physically own. Don't be a pirate. Have a nice day.**

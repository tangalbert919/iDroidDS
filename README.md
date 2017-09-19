# nds4droid

>**WARNING! This is not the official branch, and lots of things are going on here. Use this at your own risk.**

nds4droid is a free (including ad-free) open source Nintendo DS emulator for Android smartphones. You can find it on the [Google Play Store](https://play.google.com/store/apps/details?id=com.opendoorstudios.ds4droid&hl=en).

All releases from [jquesnelle](https://github.com/jquesnelle) will come from him. All releases from this fork is not uploaded to the play store, but can be found in the releases section of this fork.

This is a development branch of version 48 of nds4droid. It is not recommended for normal use, as it may crash or do something incorrectly.

### Advantages

* This project is open-sourced. Anyone can contribute to it and make it better.
* This emulator is free, unlike a few other emulators (that might not even have better performance).
* No advertisements! You can play this game ad-free!
* The sound quality has been improved (and Android is notorious for having noticeable audio latency).
* Other emulators use DeSmuME 0.9.8 as the core. We use 0.9.11. Less bugs for you to run into.
* There's an OpenGL ES 3.0 Renderer for people to use (but your device has to support it).

### Disadvantages

* You need to be on Android 5.0 or newer to use this version of nds4droid (since it's a development version, and we're using the Material theme).
* Lightning JIT is gone. However, the normal DeSmuME interpreter has become a lot faster, and every device I've used reported JIT as an illegal instruction, so why bring it back?
* If you're reading this, you're aware that you're on the development branch of nds4droid. Something will fail, and I am hoping that you will report an issue about it.

### Issues

If you found an issue, please [make an issue ticket here](https://github.com/tangalbert919/nds4droid/issues/new). Your issue will be closed if it either:
* Has nothing to do with this emulator,
* Contains copyrighted stuff,
* or has something to do with adding support for unsupported devices.

### To-do list

* Make the OpenGL ES 2.0 Renderer to render textures correctly.
* Start using Qualcomm's Ne10 library instead of math-neon from the Google Code Archive.

### Known issues

* The app crashes when loading a ROM. Just try loading it again and it will work.
* The OpenGL ES 2.0 Renderer does not support FBOs, resulting in incorrect rendering of some textures.
* The OpenGL ES 3.0 Renderer does not enable on supported devices for some reason.
* No file is created when saving the game. This is a major issue.

**DISCLAIMER:** nds4droid is licensed under the GNU General Public License, which means the software is provided as-is. The Nintendo DS, and the Nintendo DS logo are trademarks of Nintendo, Inc. Games that support the Nintendo DS are trademarks of Nintendo, Inc., and their respective companies and affiliates.

**We do not support piracy in any way. All ROMs that are found online have copyrights that last 75 years. Only use the games you physically own. Don't be a pirate. Have a nice day.**

# iDroidDS (nds4droid) ![Travis-CI](https://travis-ci.org/tangalbert919/iDroidDS.svg?branch=master)

>**WARNING! This is not the official repository of nds4droid, and nothing here is stable! Use this at your own risk.**

nds4droid is a free (including ad-free) open-source Nintendo DS emulator for Android smartphones.
You can find it on the [Google Play Store](https://play.google.com/store/apps/details?id=com.opendoorstudios.ds4droid&hl=en).

The official repository is found [here](https://github.com/jquesnelle/nds4droid).

This repository is merely a new location to the forked repo found [here](https://github.com/tangalbert919/nds4droid).

There are many advantages and disadvantages to this project, and against other DS emulators in general.


### Advantages

* This project is free and open-source. Anyone can contribute to this project and make it better.
* There are no advertisements in nds4droid. You should not be interrupted by ads for any reason.
* Other emulators use DeSmuME 0.9.9 as the core. We use 0.9.11 instead, since it irons out a LOT of bugs that people have complained about.
* This emulator supports armeabi-v7a, arm64-v8a, x86, and x86_64, which means better performance on those last three architectures.


### Disadvantages

* You need to be on Android 5.0 (Lollipop) or newer to use this version of nds4droid.
* There will be bugs present in this repository that might cause nds4droid to not operate properly. This is why I need bug reports.


### Issues

If you found an issue, please [make an issue ticket here](https://github.com/tangalbert919/iDroidDS/issues/new).
Your issue will be closed if any of the following reasons are included:
* The issue has something to do with supporting devices that are below Android 5.0 (Lollipop).
* The issue involves a ROM not operating correctly.


### Supported devices

Any device with over 512 MB of RAM and uses Android 5.0 or above should be able to run this emulator.
Performance varies by the hardware in your device and the game you're attempting to emulate.


### To-do list

- [ ] Handle problems other users have reported.
- [ ] Add Dropbox support.
- [ ] Improve the OpenGL ES 2.0 Renderer.
- [ ] Get the OpenGL ES 3.0 Renderer working.
- [ ] Replace Google's math-neon with Qualcomm's Ne10.
- [ ] Switch to libc++ (NDK requirements).
- [x] Fix the bug that causes the emulator to not write save files.
- [ ] Improve the UI.
- [ ] Add Vulkan renderer.
- [ ] Build JIT for ARM (x86 already has it).

### License
**DISCLAIMER:** nds4droid is licensed under the GNU General Public License, which means the software is provided as-is. Absolutely no warranty is provided with this software.

The Nintendo DS, and the Nintendo DS logo are trademarks of Nintendo, Inc. Games that support the Nintendo DS are trademarks of Nintendo, Inc., and their respective companies and affiliates.

**We do not support piracy in any way. Only use the games you physically own. Don't be a pirate. Have a nice day.**

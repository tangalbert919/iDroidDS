# iDroidDS (nds4droid)

>**WARNING! This is not the official repository, and lots of things are going on here. Use this at your own risk.**

nds4droid is a free (including ad-free) open source Nintendo DS emulator for Android smartphones. You can find it on the [Google Play Store](https://play.google.com/store/apps/details?id=com.opendoorstudios.ds4droid&hl=en).

This is NOT the official repository for nds4droid, as I have previously stated in the warning at the top of this README file. The official repository is found [here](https://github.com/jquesnelle/nds4droid). Where you are right now is the development repository, where new versions of nds4droid are in production.

There are many advantages and disadvantages to this project, and against other DS emulators in general.

### Advantages

* This project is free and open-source. Anyone can contribute to this project and make it better.
* There are no advertisements in nds4droid. You should not be interrupted by ads for any reason.
* Other emulators use DeSmuME 0.9.9 as the core. We use 0.9.11 (and some stuff from 0.9.12, which is in active development [here](https://github.com/TASVideos/desmume)).
* Some emulators do not have binaries built for the x86, ARM64, or x86_64 architectures. This emulator does, and will always have it.

### Disadvantages

* You need to be on Android 5.0 (Lollipop) or newer to use this version of nds4droid. As much as we like to keep the emulator running on as many devices as possible, we also need to keep moving forward as the industry does.
* There will be bugs present in this repository that might cause nds4droid to not operate properly. This is why I need bug reports.
* Only the x86 and x86_64 architectures are able to use JIT. ARM and ARM64 devices can't use this or the emulator crashes in an instant (but the normal DeSmuME interpreter is much faster now).

### Issues

If you found an issue, please [make an issue ticket here](https://github.com/tangalbert919/iDroidDS/issues/new). Your issue will be closed if any of these reasons are fulfilled:
* The issue has something to do with supporting devices that are not on Android 5.0 or above.
* The issue involves a ROM not operating correctly.
* The issue has something to do with making this emulator run properly on Amazon devices.

### Supported devices

Any device with over 512 MB of RAM and uses Android 5.0 or above should be able to run this emulator. Performance varies by the hardware in your device and the game you're attempting to emulate.

### To-do list

* Make the OpenGL ES 2.0 Renderer to render textures correctly.
* Start using Qualcomm's Ne10 library instead of math-neon from the Google Code Archive.
* Fix the bug that causes the application to crash when loading a ROM.
* Fix the bug that causes the emulator to not write save files (Android sets the file path correctly, but the emulator writes files using a path that starts with "./", which is problematic).

**DISCLAIMER:** nds4droid is licensed under the GNU General Public License, which means the software is provided as-is. The Nintendo DS, and the Nintendo DS logo are trademarks of Nintendo, Inc. Games that support the Nintendo DS are trademarks of Nintendo, Inc., and their respective companies and affiliates.

**We do not support piracy in any way. All ROMs that are found online have copyrights that last 75 years. Only use the games you physically own. Don't be a pirate. Have a nice day.**

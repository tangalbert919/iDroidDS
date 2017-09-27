/*	sndsuperpowered.cpp
	Copyright (C) 2017 The nds4droid team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

// This entire class file is supposed to use Superpowered SDK, which is supposed to bring zero audio latency compared to OpenSL ES.
#include "SPU.h"
#include "sndsuperpowered.h"
#include "main.h"

#include <superpowered/AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

int SNDSuperpowerInit(int buffersize);
void SNDSuperpowerDeInit();
void SNDSuperpowerUpdateAudio(s16 *buffer, u32 num_samples);
u32 SNDSuperpowerGetAudioSpace();
void SNDSuperpowerMuteAudio();
void SNDSuperpowerUnMuteAudio();
void SNDSuperpowerSetVolume(int volume);
void SNDSuperpowerClearAudioBuffer();

SoundInterface_struct SNDSuperpowered = {
        SNDCORE_SUPERPOWERED,
        "Superpowered Audio Engine",
        SNDSuperpowerInit,
        SNDSuperpowerDeInit,
        SNDSuperpowerUpdateAudio,
        SNDSuperpowerGetAudioSpace,
        SNDSuperpowerMuteAudio,
        SNDSuperpowerUnMuteAudio,
        SNDSuperpowerSetVolume,
        SNDSuperpowerClearAudioBuffer,
};

SuperpoweredAndroidAudioIO audioIO;

int SNDSuperpowerInit(int buffersize) {
    audioIO.start();

    int frequency = DESMUME_SAMPLE_RATE;
    audioProcessingCallback callback;
    buffersize = 512;
}

void SNDSuperpowerMuteAudio() {
    audioIO.onBackground();
}

void SNDSuperpowerUnMuteAudio() {
    audioIO.onForeground();
}
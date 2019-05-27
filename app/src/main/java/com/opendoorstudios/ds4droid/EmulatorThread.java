package com.opendoorstudios.ds4droid;

/*
Copyright (C) 2012 Jeffrey Quesnelle

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

import android.os.Environment;
import android.preference.PreferenceManager;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

class EmulatorThread extends Thread {

    final AtomicBoolean paused = new AtomicBoolean(false);
    private final AtomicBoolean finished = new AtomicBoolean(false);
    private final Object dormant = new Object();
    boolean frameFinished = false;
    long lastDraw = 0;
    Lock inFrameLock = new ReentrantLock();
    int fps = 1;
    long frameCounter = 0;
    private boolean soundPaused = true;
    private String pendingRomLoad = null;
    private Integer pending3DChange = null;
    private Integer pendingSoundChange = null;
    private Integer pendingCPUChange = null;
    private Integer pendingSoundSyncModeChange = null;
    private MainActivity activity = null;

    EmulatorThread(MainActivity activity) {
        super("DeSmuME Thread");
        this.activity = activity;
    }

    void setCurrentActivity(MainActivity activity) {
        this.activity = activity;
    }

    void loadRom(String path) {
        pendingRomLoad = path;
        synchronized (dormant) {
            dormant.notifyAll();
        }
    }

    void change3D(int set) {
        pending3DChange = set;
    }

    void changeSound(int set) {
        pendingSoundChange = set;
    }

    void changeCPUMode(int set) {
        pendingCPUChange = set;
    }

    void changeSoundSyncMode(int set) {
        pendingSoundSyncModeChange = set;
    }

    public void setCancel(boolean set) {
        finished.set(set);
        synchronized (dormant) {
            dormant.notifyAll();
        }
    }

    public void setPause(boolean set) {
        paused.set(set);
        if (DeSmuME.inited) {
            DeSmuME.setSoundPaused(set ? 1 : 0);
            DeSmuME.setMicPaused(set ? 1 : 0);
            soundPaused = set;
        }
        synchronized (dormant) {
            dormant.notifyAll();
        }
    }

    @Override
    public void run() {
        if (!DeSmuME.inited) {
            DeSmuME.context = activity;
            DeSmuME.load();

            final String defaultWorkingDir = Environment.getExternalStorageDirectory().getAbsolutePath() + "/nds4droid";
            final String path = PreferenceManager.getDefaultSharedPreferences(activity).getString(Settings.DESMUME_PATH, defaultWorkingDir);
            final File workingDir = new File(path);
            final File tempDir = new File(path + "/Temp");
            tempDir.mkdir();
            DeSmuME.setWorkingDir(workingDir.getAbsolutePath(), tempDir.getAbsolutePath() + "/");
            workingDir.mkdir();
            new File(path + "/States").mkdir();
            new File(path + "/Battery").mkdir();
            new File(path + "/Cheats").mkdir();

            //clear any previously extracted ROMs

            final File[] cacheFiles = tempDir.listFiles();
            if (cacheFiles != null) {
                for (File cacheFile : cacheFiles) {
                    if (cacheFile.getAbsolutePath().toLowerCase().endsWith(".nds"))
                        cacheFile.delete();
                }
            }

            DeSmuME.init();
            DeSmuME.inited = true;
        }

        while (!finished.get()) {
            if (pendingRomLoad != null) {
                activity.msgHandler.sendEmptyMessage(MainActivity.LOADING_START);
                if (DeSmuME.romLoaded)
                    DeSmuME.closeRom();
                if (!DeSmuME.loadRom(pendingRomLoad)) {
                    activity.msgHandler.sendEmptyMessage(MainActivity.LOADING_END);
                    activity.msgHandler.sendEmptyMessage(MainActivity.ROM_ERROR);
                    DeSmuME.romLoaded = false;
                    DeSmuME.loadedRom = null;
                } else {
                    activity.msgHandler.sendEmptyMessage(MainActivity.LOADING_END);
                    DeSmuME.romLoaded = true;
                    DeSmuME.loadedRom = pendingRomLoad;
                    setPause(false);
                }
                pendingRomLoad = null;
            }
            if (pending3DChange != null) {
                DeSmuME.change3D(pending3DChange);
                pending3DChange = null;
            }
            if (pendingSoundChange != null) {
                DeSmuME.changeSound(pendingSoundChange);
                pendingSoundChange = null;
            }
            if (pendingCPUChange != null) {
                DeSmuME.changeCpuMode(pendingCPUChange);
                pendingCPUChange = null;
            }
            if (pendingSoundSyncModeChange != null) {
                DeSmuME.changeSoundSynchMode(pendingSoundSyncModeChange);
                pendingSoundSyncModeChange = null;
            }

            if (!paused.get()) {

                if (soundPaused) {
                    DeSmuME.setSoundPaused(0);
                    DeSmuME.setMicPaused(0);
                    soundPaused = false;
                }

                inFrameLock.lock();
                do {
                    DeSmuME.runCore();
                } while (DeSmuME.fastForwardMode);
                inFrameLock.unlock();
                frameFinished = true;

            } else {
                //hacky, but keeps thread alive so we don't lose contexts
                try {
                    synchronized (dormant) {
                        dormant.wait();
                    }
                } catch (InterruptedException e) {
                }
            }
        }
    }
}

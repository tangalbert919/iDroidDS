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

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import android.os.Environment;
import android.preference.PreferenceManager;

class EmulatorThread extends Thread {
	
	EmulatorThread(MainActivity activity) {
		super("EmulatorThread");
		this.activity = activity;
	}
	
	void setCurrentActivity(MainActivity activity) {
		this.activity = activity;
	}
	
	private boolean soundPaused = true;
	boolean frameFinished = false;
	long lastDraw = 0;
	private final AtomicBoolean finished = new AtomicBoolean(false);
	final AtomicBoolean paused = new AtomicBoolean(false);
	private String pendingRomLoad = null;
	private Integer pending3DChange = null;
	private Integer pendingSoundChange = null;
	private boolean pendingCPUChange = false;
	private Integer pendingSoundSyncModeChange = null;
	
	void loadRom(String path) {
		pendingRomLoad = path;
		synchronized(dormant) {
			dormant.notifyAll();
		}
	}
	
	void change3D(int set) {
		pending3DChange = set;
	}
	
	void changeSound(int set) {
		pendingSoundChange = set;
	}
	
	void changeCPUMode(boolean set) {
		pendingCPUChange = set;
	}
	
	void changeSoundSyncMode(int set) {
		pendingSoundSyncModeChange = set;
	}
	
	public void setCancel(boolean set) {
		finished.set(set);
		synchronized(dormant) {
			dormant.notifyAll();
		}
	}
	
	public void setPause(boolean set) {
		paused.set(set);
		if(ds.inited) {
			ds.setSoundPaused(set ? 1 : 0);
			ds.setMicPaused(set ? 1 : 0);
			soundPaused = set;
		}
		synchronized(dormant) {
			dormant.notifyAll();
		}
	}
	
	private final Object dormant = new Object();
	
	Lock inFrameLock = new ReentrantLock();
	int fps = 1;
	private MainActivity activity = null;
	long frameCounter = 0;
	private DeSmuME ds = new DeSmuME();
	
	@Override
	public void run() {
		if(!ds.inited) {
			ds.context = activity;
			ds.load();
			
			final String defaultWorkingDir = Environment.getExternalStorageDirectory().getAbsolutePath() + "/nds4droid";
			final String path = PreferenceManager.getDefaultSharedPreferences(activity).getString(Settings.DESMUME_PATH, defaultWorkingDir);
			final File workingDir = new File(path);
			final File tempDir = new File(path + "/Temp");
			tempDir.mkdir();
			ds.setWorkingDir(workingDir.getAbsolutePath(), tempDir.getAbsolutePath() + "/");
			workingDir.mkdir();
			new File(path + "/States").mkdir();
			new File(path + "/Battery").mkdir();
			new File(path + "/Cheats").mkdir();
			
			//clear any previously extracted ROMs
			
			final File[] cacheFiles = tempDir.listFiles();
			if(cacheFiles != null) {
				for(File cacheFile : cacheFiles) {
					if(cacheFile.getAbsolutePath().toLowerCase().endsWith(".nds"))
						cacheFile.delete();
				}
			}
			
			ds.init();
			ds.inited = true;
		}
		
		while(!finished.get()) {
			if(pendingRomLoad != null) {
				activity.msgHandler.sendEmptyMessage(MainActivity.LOADING_START);
				if(ds.romLoaded)
					ds.closeRom();
				if(!ds.loadRom(pendingRomLoad)) {
					activity.msgHandler.sendEmptyMessage(MainActivity.LOADING_END);
					activity.msgHandler.sendEmptyMessage(MainActivity.ROM_ERROR);
					ds.romLoaded = false;
					ds.loadedRom = null;
				}
				else {
					activity.msgHandler.sendEmptyMessage(MainActivity.LOADING_END);
					ds.romLoaded = true;
					ds.loadedRom = pendingRomLoad;
					setPause(false);
				}
				pendingRomLoad = null;
			}
			if(pending3DChange != null) {
				ds.change3D(pending3DChange);
				pending3DChange = null;
			}
			if(pendingSoundChange != null) {
				ds.changeSound(pendingSoundChange);
				pendingSoundChange = null;
			}
			if(pendingCPUChange) {
				ds.changeCpuMode(pendingCPUChange);
				pendingCPUChange = false;
			}
			if(pendingSoundSyncModeChange != null) {
				ds.changeSoundSynchMode(pendingSoundSyncModeChange);
				pendingSoundSyncModeChange = null;
			}
			
			if(!paused.get()) {
				
				if(soundPaused) {
					DeSmuME.setSoundPaused(0);
					DeSmuME.setMicPaused(0);
					soundPaused = false;
				}
				
				inFrameLock.lock();
				do {
					DeSmuME.runCore();
				} while(DeSmuME.fastForwardMode);
				inFrameLock.unlock();
				frameFinished = true;

			} 
			else {
				//hacky, but keeps thread alive so we don't lose contexts
				try {
					synchronized(dormant) {
						dormant.wait();
					}
				} 
				catch (InterruptedException e) {
				} 
			}
		}
	}
}
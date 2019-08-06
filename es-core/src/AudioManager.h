#pragma once
#ifndef ES_CORE_AUDIO_MANAGER_H
#define ES_CORE_AUDIO_MANAGER_H

#include <SDL_audio.h>
#include <memory>
#include <vector>
#include "SDL_mixer.h"
#include "ThemeData.h"
#include <string>

class Sound;

class AudioManager
{	

public:
	static std::shared_ptr<AudioManager> & getInstance();

	void init();
	void deinit();

	void registerSound(std::shared_ptr<Sound> & sound);
	void unregisterSound(std::shared_ptr<Sound> & sound);

	void play();
	void stop();

	void playRandomMusic(bool continueIfPlaying = true);
	void stopMusic();
	void themeChanged(const std::shared_ptr<ThemeData>& theme);

	void setSystemName(std::string name) {
		mSystemName = name;
	}

	virtual ~AudioManager();

private:
	AudioManager();

	static std::vector<std::shared_ptr<Sound>> sSoundVector;
	static std::shared_ptr<AudioManager> sInstance;
	static Mix_Music* mCurrentMusic;
	
	static void onMusicFinished();

	void	findMusic(const std::string &path, std::vector<std::string>& all_matching_files);
	void	playMusic(std::string path);
		
	std::string mSystemName;
	std::string mCurrentThemeMusicDirectory;	
	bool		mRunningFromPlaylist;	

};

#endif // ES_CORE_AUDIO_MANAGER_H

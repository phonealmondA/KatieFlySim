// AudioManager.h
#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <iostream>
#include <map>
#include <memory>

class AudioManager {
private:
    sf::Music backgroundMusic;
    float musicVolume;
    float soundVolume;
    bool musicEnabled;
    bool soundEnabled;
    std::map<std::string, std::unique_ptr<sf::SoundBuffer>> soundBuffers;
    std::map<std::string, std::unique_ptr<sf::Sound>> sounds;

public:
    AudioManager();
    ~AudioManager();

    bool loadBackgroundMusic(const std::string& filename);
    void playBackgroundMusic(bool loop = true);
    void stopBackgroundMusic();
    void pauseBackgroundMusic();
    void resumeBackgroundMusic();
    void setMusicVolume(float volume);
    float getMusicVolume() const;
    bool isMusicPlaying() const;
    void toggleMusic();

    bool loadSound(const std::string& name, const std::string& filename);
    void playSound(const std::string& name);
    void setSoundVolume(float volume);
    float getSoundVolume() const;
    void toggleSound();
};
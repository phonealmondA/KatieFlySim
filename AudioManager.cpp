// AudioManager.cpp
#include "AudioManager.h"
#include <iostream>

AudioManager::AudioManager()
    : musicVolume(50.0f), soundVolume(50.0f),
    musicEnabled(true), soundEnabled(true) {
}

AudioManager::~AudioManager() {
    // Stop any playing music
    backgroundMusic.stop();
}

bool AudioManager::loadBackgroundMusic(const std::string& filename) {
    if (!backgroundMusic.openFromFile(filename)) {
        std::cerr << "Failed to load background music: " << filename << std::endl;
        return false;
    }
    backgroundMusic.setVolume(musicVolume);
    return true;
}

void AudioManager::playBackgroundMusic(bool loop) {
    if (musicEnabled) {
        backgroundMusic.setLooping(loop);  // Correct method in SFML 3.0
        backgroundMusic.play();
    }
}

void AudioManager::stopBackgroundMusic() {
    backgroundMusic.stop();
}

void AudioManager::pauseBackgroundMusic() {
    backgroundMusic.pause();
}

void AudioManager::resumeBackgroundMusic() {
    if (musicEnabled) {
        backgroundMusic.play();
    }
}

void AudioManager::setMusicVolume(float volume) {
    musicVolume = std::min(100.0f, std::max(0.0f, volume));
    backgroundMusic.setVolume(musicVolume);
}

float AudioManager::getMusicVolume() const {
    return musicVolume;
}

bool AudioManager::isMusicPlaying() const {
    return backgroundMusic.getStatus() == sf::SoundSource::Status::Playing;  // Correct enum in SFML 3.0
}

void AudioManager::toggleMusic() {
    musicEnabled = !musicEnabled;
    if (musicEnabled) {
        if (backgroundMusic.getStatus() == sf::SoundSource::Status::Paused) {  // Correct enum in SFML 3.0
            backgroundMusic.play();
        }
    }
    else {
        if (backgroundMusic.getStatus() == sf::SoundSource::Status::Playing) {  // Correct enum in SFML 3.0
            backgroundMusic.pause();
        }
    }
}

bool AudioManager::loadSound(const std::string& name, const std::string& filename) {
    auto buffer = std::make_unique<sf::SoundBuffer>();
    if (!buffer->loadFromFile(filename)) {
        std::cerr << "Failed to load sound effect: " << filename << std::endl;
        return false;
    }
    soundBuffers[name] = std::move(buffer);
    sounds[name] = std::make_unique<sf::Sound>(*soundBuffers[name]);
    sounds[name]->setVolume(soundVolume);
    return true;
}

void AudioManager::playSound(const std::string& name) {
    if (!soundEnabled) return;
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        it->second->play();
    }
    else {
        std::cerr << "Sound not found: " << name << std::endl;
    }
}

void AudioManager::setSoundVolume(float volume) {
    soundVolume = std::min(100.0f, std::max(0.0f, volume));
    for (auto& sound : sounds) {
        sound.second->setVolume(soundVolume);
    }
}

float AudioManager::getSoundVolume() const {
    return soundVolume;
}

void AudioManager::toggleSound() {
    soundEnabled = !soundEnabled;
}
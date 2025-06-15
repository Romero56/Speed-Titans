#include "Sounds.h"
#include <fstream>
#include <iostream>

static ALCdevice* device = nullptr;
static ALCcontext* context = nullptr;
static ALuint source;

bool LoadWavFile(const std::string& filename, ALuint& buffer) {
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open OpenAL device.\n";
        return false;
    }

    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open WAV file: " << filename << "\n";
        return false;
    }

    char riff[4];
    file.read(riff, 4);
    file.ignore(4); // file size
    char wave[4];
    file.read(wave, 4);

    char fmt[4];
    file.read(fmt, 4);
    file.ignore(4); // fmt chunk size

    short audioFormat;
    short numChannels;
    int sampleRate;
    file.read(reinterpret_cast<char*>(&audioFormat), 2);
    file.read(reinterpret_cast<char*>(&numChannels), 2);
    file.read(reinterpret_cast<char*>(&sampleRate), 4);
    file.ignore(6); // Byte rate and block align
    short bitsPerSample;
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    char data[4];
    file.read(data, 4);
    int dataSize;
    file.read(reinterpret_cast<char*>(&dataSize), 4);

    std::vector<char> bufferData(dataSize);
    file.read(bufferData.data(), dataSize);

    ALenum format = 0;
    if (bitsPerSample == 8) format = (numChannels == 1) ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
    if (bitsPerSample == 16) format = (numChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, bufferData.data(), dataSize, sampleRate);

    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);


    return true;
}

void PlaySound(ALuint buffer) {
    alSourcePlay(source);
}

void CleanupOpenAL() {
    alDeleteSources(1, &source);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

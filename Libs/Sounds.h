#ifndef SOUNDS_H
#define SOUNDS_H

#include <AL/al.h>
#include <AL/alc.h>
#include <vector>
#include <string>

bool LoadWavFile(const std::string& filename, ALuint& buffer);
void PlaySound(ALuint buffer);
void CleanupOpenAL();

#endif
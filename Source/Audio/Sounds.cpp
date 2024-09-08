/*
Copyright (C) 2010-2017 - Lugaru contributors (see AUTHORS file)

This file is part of Lugaru.

Lugaru is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Lugaru is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Audio/Sounds.hpp"

#include "Audio/openal_wrapper.hpp"
#include "Utils/Folders.hpp"
#include <filesystem>

struct OPENAL_SAMPLE* samp[sounds_count];

extern XYZ envsound[30];
extern float envsoundvol[30];
extern int numenvsounds;
extern float envsoundlife[30];

int footstepsound, footstepsound2, footstepsound3, footstepsound4;

int channels[100];

static const char* sound_data[sounds_count] = {
#define DECLARE_SOUND(id, filename) filename,
#include "Sounds.def"
#undef DECLARE_SOUND
};

// FIXME: dimensionality is not a property of the sound sample.
// This should be decided at the time of playback
static int snd_mode(int snd)
{
    switch (snd) {
        case alarmsound:
        case consolefailsound:
        case consolesuccesssound:
        case firestartsound:
        case fireendsound:
            return OPENAL_2D;
        default:
            return OPENAL_HW3D;
    }
}

void loadAllSounds() {
    for (int i = 0; i < sounds_count; i++) {
        std::string soundFilename = sound_data[i];

        // Use getResourcePath to find the sound
        std::string soundPath = Folders::getResourcePath("Sounds/" + soundFilename);

        if (soundPath.empty()) {
            std::cerr << "Sound not found in resource paths: " << soundFilename << std::endl;
            continue; // Skip to the next sound if the file is not found
        }

        //std::cout << "Loading Sound: " << soundPath << std::endl;

        // Load the sound directly from its path
        samp[i] = OPENAL_Sample_Load(OPENAL_FREE, soundPath.c_str(), snd_mode(1), 0, 0);
        
        // Error checking for sound loading
        if (samp[i] == nullptr) {
            std::cerr << "Failed to load sound: " << soundPath << std::endl;
            continue; // Skip to the next sound if loading fails
        }

        // Debug the OpenAL error state
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cerr << "OpenAL error after loading sound: " << alGetString(error) << std::endl;
        }
    }

    // Set stream mode for looping sounds after loading all
    for (int i = stream_firesound; i <= stream_menutheme; i++) {
        if (samp[i] != nullptr) {
            OPENAL_Stream_SetMode(samp[i], OPENAL_LOOP_NORMAL);

            // Debug the OpenAL error state after setting mode
            ALenum error = alGetError();
            if (error != AL_NO_ERROR) {
                std::cerr << "OpenAL error after setting stream mode: " << alGetString(error) << std::endl;
            }
        } else {
            std::cerr << "Stream sound " << i << " is not loaded, cannot set mode." << std::endl;
        }
    }
}

void addEnvSound(XYZ coords, float vol, float life)
{
    envsound[numenvsounds] = coords;
    envsoundvol[numenvsounds] = vol;
    envsoundlife[numenvsounds] = life;
    numenvsounds++;
}

void emit_sound_at(int soundid, const XYZ& pos, float vol)
{
    PlaySoundEx(soundid, samp[soundid], NULL, true);
    OPENAL_3D_SetAttributes_(channels[soundid], pos);
    OPENAL_SetVolume(channels[soundid], vol);
    OPENAL_SetPaused(channels[soundid], false);
}

void emit_sound_np(int soundid, float vol)
{
    PlaySoundEx(soundid, samp[soundid], NULL, true);
    OPENAL_SetVolume(channels[soundid], vol);
    OPENAL_SetPaused(channels[soundid], false);
}

void emit_stream_at(int soundid, const XYZ& pos, float vol)
{
    PlayStreamEx(soundid, samp[soundid], NULL, true);
    OPENAL_3D_SetAttributes_(channels[soundid], pos);
    OPENAL_SetVolume(channels[soundid], vol);
    OPENAL_SetPaused(channels[soundid], false);
}

void emit_stream_np(int soundid, float vol)
{
    PlayStreamEx(soundid, samp[soundid], NULL, true);
    OPENAL_SetVolume(channels[soundid], vol);
    OPENAL_SetPaused(channels[soundid], false);
}

void resume_stream(int soundid)
{
    OPENAL_SetPaused(channels[soundid], false);
}

void pause_sound(int soundid)
{
    OPENAL_SetPaused(channels[soundid], true);
}
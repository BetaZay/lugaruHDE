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
    std::vector<std::string> enabledMods = Folders::getEnabledMods(); // Get enabled mods
    std::string tempFolderPath = "Data/Temp";  // Specify the temp folder path

    // Create the temporary folder if it doesn't exist
    if (!std::filesystem::exists(tempFolderPath)) {
        std::filesystem::create_directory(tempFolderPath);
    }
    
    for (int i = 0; i < sounds_count; i++) {
        std::string soundFilename = sound_data[i];
        std::string modSoundPath;
        bool soundFound = false;

        // Look through enabled mods first
        for (const std::string& modName : enabledMods) {
            modSoundPath = Folders::getSoundResourcePath(modName, soundFilename);

            // Check for exact match
            if (Folders::file_exists(modSoundPath)) {
                soundFound = true;
                break;  // Sound found in a mod, stop searching
            }

            // If not found, perform a case-insensitive search
            std::string lowerModSoundPath = Folders::findFileCaseInsensitive(modSoundPath);
            if (!lowerModSoundPath.empty()) {
                soundFound = true;
                modSoundPath = lowerModSoundPath;
                break;  // Case-insensitive match found, stop searching
            }
        }

        // If not found in mods, look in the main sound folder
        if (!soundFound) {
            modSoundPath = Folders::getResourcePath("Sounds/" + soundFilename);

            // Check for exact match first
            if (!Folders::file_exists(modSoundPath)) {
                // If not found, perform a case-insensitive search
                modSoundPath = Folders::findFileCaseInsensitive(modSoundPath);
            }
        }

        size_t modStartPos = modSoundPath.find("/Mods/");
        if (modStartPos != std::string::npos) {
            std::string trimmedFilename = modSoundPath.substr(modStartPos + 6); // Skip over "/Mods/"
            std::cout << "Loading Sound: " << trimmedFilename << std::endl;
        } else {
            std::cout << "Loading Sound: " << modSoundPath << std::endl;
        }

        // Copy the sound to the temporary folder
        std::string tempSoundPath = tempFolderPath + "/" + soundFilename;
        try {
            std::filesystem::copy(modSoundPath, tempSoundPath, std::filesystem::copy_options::overwrite_existing);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to copy sound to temp folder: " << e.what() << std::endl;
            continue; // Skip to the next sound if copying fails
        }

        // Load the sound from the temp folder
        if (Folders::file_exists(tempSoundPath)) {
            samp[i] = OPENAL_Sample_Load(OPENAL_FREE, tempSoundPath.c_str(), snd_mode(1), 0, 0);

            // Delete the sound file after loading
            try {
                std::filesystem::remove(tempSoundPath);
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Failed to delete temp sound file: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Failed to load sound from temp folder: " << tempSoundPath << std::endl;
        }
    }

    // Loop to set the stream mode, outside of sound-loading loop
    for (int i = stream_firesound; i <= stream_menutheme; i++) {
        OPENAL_Stream_SetMode(samp[i], OPENAL_LOOP_NORMAL);
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

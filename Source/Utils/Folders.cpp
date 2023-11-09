/*
Copyright (C) 2003, 2010 - Wolfire Games
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
su
You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Folders.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <filesystem>
#include <unordered_set>

#if PLATFORM_UNIX
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if _WIN32
#include <shlobj.h> // to get paths related functions
#include <windows.h>
#endif

const std::string Folders::dataDir = DATA_DIR;

std::string Folders::getScreenshotDir()
{
    std::string screenshotDir = getUserDataPath() + "/Screenshots";
    makeDirectory(screenshotDir);
    return screenshotDir;
}

std::string Folders::getUserDataPath()
{
    std::string userDataPath;
#ifdef _WIN32
    char path[MAX_PATH];
    // %APPDATA% (%USERPROFILE%\Application Data)
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        userDataPath = std::string(path) + "/Lugaru/";
    } else {
        return dataDir;
    }
#elif (defined(__APPLE__) && defined(__MACH__))
    const char* homePath = getHomeDirectory();
    if (homePath == NULL) {
        userDataPath = ".";
    } else {
        userDataPath = std::string(homePath) + "/Library/Application Support/Lugaru";
    }
#else // Linux
    userDataPath = getGenericDirectory("XDG_DATA_HOME", ".local/share");
#endif
    makeDirectory(userDataPath);
    return userDataPath;
}

std::string Folders::getConfigFilePath()
{
    std::string configFolder;
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
    configFolder = getUserDataPath();
#else // Linux
    configFolder = getGenericDirectory("XDG_CONFIG_HOME", ".config");
    makeDirectory(configFolder);
#endif
    return configFolder + "/config.txt";
}

int Folders::getNumMods() {
    int numMods = 0;
    std::string modListFilePath = getUserDataPath() + "/modlist.txt";
    std::ifstream modListFile(modListFilePath);
    if (modListFile.is_open()) {
        std::string line;
        while (std::getline(modListFile, line)) {
            size_t modStatusPos = line.find(":");
            if (modStatusPos != std::string::npos) {
                int modStatus = std::stoi(line.substr(modStatusPos + 2));
                if (modStatus == 1) {
                    numMods++;
                }
            }
        }
        modListFile.close();
    } else {
        std::cerr << "Unable to open modlist.txt." << std::endl;
    }
    return numMods;
}

std::vector<std::string> Folders::getEnabledMods() {
    std::vector<std::string> enabledMods;
    std::ifstream modListFile(getUserDataPath() + "/modlist.txt");
    if (modListFile.is_open()) {
        std::string line;
        while (std::getline(modListFile, line)) {
            size_t modStatusPos = line.find(":");
            if (modStatusPos != std::string::npos) {
                std::string modName = line.substr(0, modStatusPos);
                int modStatus = std::stoi(line.substr(modStatusPos + 1));
                if (modStatus == 1) {
                    enabledMods.push_back(modName);
                }
            }
        }
        modListFile.close();
    } else {
        std::cerr << "Unable to open modlist.txt." << std::endl;
    }
    return enabledMods;
}

std::string Folders::getModResourcePath(const std::string& modName, const std::string& resourceType) {
    std::string modsFolderPath = getUserDataPath() + "/Mods/";
    std::string modPath = modsFolderPath + modName + "/" + resourceType;
    return modPath;
}

std::string Folders::createModListFile() {
    std::string modsFolderPath = getUserDataPath() + "/Mods";
    if (!std::filesystem::exists(modsFolderPath)) {
        if (!makeDirectory(modsFolderPath)) {
            std::cerr << "Unable to create Mods folder." << std::endl;
            return "";
        }
        std::cout << "Mods folder created successfully." << std::endl;
    }

    std::string modListFilePath = getUserDataPath() + "/modlist.txt";

    if (!std::filesystem::exists(modListFilePath)) {
        std::ofstream modListFile(modListFilePath);
        if (!modListFile.is_open()) {
            std::cerr << "Unable to create modlist.txt." << std::endl;
            return "";
        }
        modListFile.close();
        if (std::filesystem::exists(modListFilePath)) {
            std::cout << "modlist.txt created successfully." << std::endl;
        } else {
            std::cerr << "Unable to create modlist.txt." << std::endl;
            return "";
        }
    } else {
        // File already exists, update it
        return updateModListFile();
    }

    return "modlist.txt created successfully";
}

std::string Folders::updateModListFile() {
    std::string modsFolderPath = getUserDataPath() + "/Mods";
    std::string modListFilePath = getUserDataPath() + "/modlist.txt";

    std::unordered_set<std::string> installedMods;
    std::unordered_set<std::string> currentMods;

    // Read existing modlist data
    if (std::filesystem::exists(modListFilePath)) {
        std::ifstream inputFile(modListFilePath);
        if (inputFile.is_open()) {
            std::string line;
            while (std::getline(inputFile, line)) {
                std::size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    std::string modName = line.substr(0, pos);
                    installedMods.insert(modName);
                }
            }
            inputFile.close();
        } else {
            std::cerr << "Unable to read modlist.txt." << std::endl;
            return "";
        }
    }

    // Update the modlist data
    std::ofstream modListFile(modListFilePath, std::ios::trunc);
    if (!modListFile.is_open()) {
        std::cerr << "Unable to update modlist.txt." << std::endl;
        return "";
    }

    for (const auto &entry : std::filesystem::directory_iterator(modsFolderPath)) {
        if (entry.is_directory()) {
            std::string modName = entry.path().filename().string();
            currentMods.insert(modName);
            if (installedMods.find(modName) == installedMods.end()) {
                // Mod not found in the existing modlist, so write it with the default value
                modListFile << modName << ": 0" << std::endl;
            } else {
                // Mod found in the existing modlist, so write it as is
                modListFile << modName << ": 1" << std::endl;
                installedMods.erase(modName); // Remove the mod from the installed set
            }
        }
    }

    // Write any remaining installed mods that are no longer present in the directory
    for (const auto &mod : installedMods) {
        if (currentMods.find(mod) == currentMods.end()) {
            // Mod no longer exists in the Mods folder, so don't write it
            continue;
        }
        modListFile << mod << ": 0" << std::endl;
    }

    modListFile.close();

    if (std::filesystem::exists(modListFilePath)) {
        std::cout << "modlist.txt updated successfully." << std::endl;
        return "modlist.txt updated successfully";
    } else {
        std::cerr << "Unable to update modlist.txt." << std::endl;
        return "";
    }
}

#if PLATFORM_LINUX
/* Generic code for XDG ENVVAR test and fallback */
std::string Folders::getGenericDirectory(const char* ENVVAR, const std::string& fallback)
{
    const char* path = getenv(ENVVAR);
    std::string ret;
    if ((path != NULL) && (strlen(path) != 0)) {
        ret = std::string(path) + "/lugaru";
    } else {
        const char* homedir = getHomeDirectory();
        if ((homedir != NULL) && (strlen(homedir) != 0)) {
            ret = std::string(homedir) + '/' + fallback + "/lugaru";
        } else {
            ret = ".";
        }
    }
    return ret;
}
#endif

#if PLATFORM_UNIX
const char* Folders::getHomeDirectory()
{
    const char* homedir = getenv("HOME");
    if (homedir != NULL) {
        return homedir;
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw != NULL) {
        return pw->pw_dir;
    }
    return NULL;
}
#endif

bool Folders::makeDirectory(const std::string& path)
{
#ifdef _WIN32
    int status = CreateDirectory(path.c_str(), NULL);
    return ((status != 0) || (GetLastError() == ERROR_ALREADY_EXISTS));
#else
    errno = 0;
    int status = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return ((status == 0) || (errno == EEXIST));
#endif
}

FILE* Folders::openMandatoryFile(const std::string& filename, const char* mode)
{
    FILE* tfile = fopen(filename.c_str(), mode);
    if (tfile == NULL) {
        throw FileNotFoundException(filename);
    }
    return tfile;
}

bool Folders::file_exists(const std::string& filepath)
{
    FILE* file;
    file = fopen(filepath.c_str(), "rb");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

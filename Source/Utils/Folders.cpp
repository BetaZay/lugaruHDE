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
#include <algorithm> // Needed for std::sort
#include <nlohmann/json.hpp> // Include JSON library

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

// Function to read ModInfo from JSON
ModInfo ModInfo::fromJson(const std::string& jsonFilePath) {
    ModInfo modInfo;
    std::ifstream jsonFile(jsonFilePath);
    if (!jsonFile.is_open()) {
        std::cerr << "Unable to open " << jsonFilePath << std::endl;
        return modInfo;
    }

    nlohmann::json jsonData;
    jsonFile >> jsonData;
    modInfo.infoName = jsonData.value("name", "Unknown Mod");
    modInfo.description = jsonData.value("description", "No description available.");
    modInfo.version = jsonData.value("version", "1.0");

    return modInfo;
}


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

std::vector<ModInfo> Folders::getEnabledModsWithInfo() {
    std::vector<ModInfo> enabledMods;
    std::ifstream modListFile(getUserDataPath() + "/modlist.txt");
    if (modListFile.is_open()) {
        std::string line;
        while (std::getline(modListFile, line)) {
            size_t modStatusPos = line.find(":");
            if (modStatusPos != std::string::npos) {
                std::string modName = line.substr(0, modStatusPos);
                int modStatus = std::stoi(line.substr(modStatusPos + 1));
                if (modStatus == 1) {
                    // Load mod info from the mod folder
                    std::string modInfoPath = getModResourcePath(modName, "modinfo.json");
                    if (std::filesystem::exists(modInfoPath)) {
                        ModInfo modInfo = ModInfo::fromJson(modInfoPath);
                        enabledMods.push_back(modInfo);
                    } else {
                        // If no modinfo.json, use modName as fallback
                        ModInfo fallbackMod;
                        fallbackMod.folderName = modName;
                        enabledMods.push_back(fallbackMod);
                    }
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

    std::unordered_map<std::string, int> installedMods;  // Track mod names and their statuses
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
                    int modStatus = std::stoi(line.substr(pos + 2));
                    installedMods[modName] = modStatus;
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

    // Iterate through current mods in Mods folder and keep their original status
    for (const auto &entry : std::filesystem::directory_iterator(modsFolderPath)) {
        if (entry.is_directory()) {
            std::string modName = entry.path().filename().string();
            currentMods.insert(modName);

            // Keep original status if available, otherwise default to 0
            int modStatus = (installedMods.find(modName) != installedMods.end()) ? installedMods[modName] : 0;
            modListFile << modName << ": " << modStatus << std::endl;
            
            // Remove from installedMods to track remaining unused mods
            installedMods.erase(modName);
        }
    }

    // Write remaining mods that are no longer present in the directory (mark as available)
    for (const auto &mod : installedMods) {
        modListFile << mod.first << ": 0" << std::endl;  // Default to 0 (available) for missing mods
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

std::string Folders::toLower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

std::string Folders::findFileCaseInsensitive(const std::string& path)
{
    std::filesystem::path p(path);
    std::string directory = p.parent_path().string();
    std::string requestedFilename = p.filename().string();

    // Ensure the directory exists
    if (!std::filesystem::exists(directory)) {
        return "";
    }

    // First, check if the exact file exists
    std::filesystem::path fullPath = std::filesystem::path(directory) / requestedFilename;
    if (std::filesystem::exists(fullPath)) {
        return fullPath.string();
    }

    // Perform a case-insensitive search only if the exact match was not found
    std::string requestedFilenameLower = toLower(requestedFilename);
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        std::string currentFilename = entry.path().filename().string();

        if (toLower(currentFilename) == requestedFilenameLower) {
            return entry.path().string();  // Return the correct path
        }
    }

    // Return an empty string if the file wasn't found
    return "";
}

std::string Folders::getSoundResourcePath(const std::string& modName, const std::string& soundFileName) {
    std::string modsFolderPath = getUserDataPath() + "/Mods/";
    std::string modSoundPath = modsFolderPath + modName + "/Sounds/" + soundFileName;
    return modSoundPath;
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

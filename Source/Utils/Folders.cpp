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
#include "../../../../../../usr/include/sys/stat.h"

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

// Function to read ModInfo from JSON (not needed anymore but kept as reference)
ModInfo ModInfo::fromJson(const std::string& jsonFilePath) {
    ModInfo modInfo;
    std::ifstream jsonFile(jsonFilePath);
    if (!jsonFile.is_open()) {
        std::cerr << "Unable to open " << jsonFilePath << std::endl;
        return modInfo;
    }

    nlohmann::json jsonData;
    jsonFile >> jsonData;
    modInfo.infoName = jsonData.value("Name", "Unknown Mod");
    modInfo.description = jsonData.value("Description", "No description available.");
    modInfo.version = jsonData.value("Version", "1.0");

    return modInfo;
}

std::string Folders::getScreenshotDir() {
    std::string screenshotDir = getUserDataPath() + "/Screenshots";
    makeDirectory(screenshotDir);
    return screenshotDir;
}

std::string Folders::getUserDataPath() {
    std::string userDataPath;
#ifdef _WIN32
    char path[MAX_PATH];
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

std::string Folders::getConfigFilePath() {
    std::string configFolder;
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
    configFolder = getUserDataPath();
#else // Linux
    configFolder = getGenericDirectory("XDG_CONFIG_HOME", ".config");
    makeDirectory(configFolder);
#endif
    return configFolder + "/config.txt";
}

// Generate PackList.json if it doesn't exist, add mods enabled by default
void Folders::createPackListFile() {
    std::string packListPath = dataDir + "/PackList.json";
    
    if (std::filesystem::exists(packListPath)) {
        updatePackList();  // Update existing list
        return;
    }

    nlohmann::json packListJson;
    packListJson["Packs"]["Mods"] = nlohmann::json::array();
    packListJson["Packs"]["TexturePacks"] = nlohmann::json::array();  // Add array for texture packs

    // Iterate through each folder in the Data directory
    for (const auto& entry : std::filesystem::directory_iterator(dataDir)) {
        if (entry.is_directory()) {
            std::string packInfoPath = entry.path().string() + "/PackInfo.json";
            
            if (std::filesystem::exists(packInfoPath)) {
                nlohmann::json packInfoJson;
                std::ifstream packInfoFile(packInfoPath);
                packInfoFile >> packInfoJson;

                nlohmann::json newPack;
                newPack["ModName"] = packInfoJson.value("Name", "Unknown Pack");
                newPack["Description"] = packInfoJson.value("Description", "No description available.");
                newPack["Version"] = packInfoJson.value("Version", "1.0");
                newPack["Author"] = packInfoJson.value("Author", "Unknown");
                newPack["PackType"] = packInfoJson.value("PackType", "Mod");
                newPack["Status"] = "Enabled";  // Enabled by default

                // Add to correct section (Mods or TexturePacks)
                if (newPack["PackType"] == "Texture") {
                    packListJson["Packs"]["TexturePacks"].push_back(newPack);
                } else {
                    packListJson["Packs"]["Mods"].push_back(newPack);
                }
            }
        }
    }

    // Write the updated PackList.json
    std::ofstream packListFile(packListPath);
    packListFile << packListJson.dump(4);  // Pretty print
}

void Folders::updatePackList() {
    std::string packListPath = dataDir + "/PackList.json";

    // Read the existing PackList.json
    nlohmann::json packListJson;
    std::ifstream packListFile(packListPath);
    if (packListFile.is_open()) {
        packListFile >> packListJson;
        packListFile.close();
    } else {
        std::cerr << "Unable to open PackList.json." << std::endl;
        return;
    }

    // Create sets to track existing mods and texture packs
    std::unordered_set<std::string> existingMods;
    std::unordered_set<std::string> existingTexturePacks;

    for (const auto& mod : packListJson["Packs"]["Mods"]) {
        existingMods.insert(mod["ModName"]);
    }

    for (const auto& texturePack : packListJson["Packs"]["TexturePacks"]) {
        existingTexturePacks.insert(texturePack["ModName"]);
    }

    bool packListUpdated = false;

    // Iterate through each folder in the Data directory
    for (const auto& entry : std::filesystem::directory_iterator(dataDir)) {
        if (entry.is_directory()) {
            std::string packInfoPath = entry.path().string() + "/PackInfo.json";
            std::string modName = entry.path().filename().string();

            if (std::filesystem::exists(packInfoPath)) {
                nlohmann::json packInfoJson;
                std::ifstream packInfoFile(packInfoPath);
                packInfoFile >> packInfoJson;
                
                std::string packType = packInfoJson.value("PackType", "Mod"); // Default to "Mod"
                
                if (packType == "Mod" && existingMods.find(modName) == existingMods.end()) {
                    // New mod found, add it to the Mods list
                    nlohmann::json newMod;
                    newMod["ModName"] = packInfoJson.value("Name", modName); // Fallback to folder name
                    newMod["Description"] = packInfoJson.value("Description", "No description available.");
                    newMod["Version"] = packInfoJson.value("Version", "1.0");
                    newMod["Author"] = packInfoJson.value("Author", "Unknown");
                    newMod["PackType"] = "Mod";
                    newMod["Status"] = "Enabled"; // Enable by default
                    
                    packListJson["Packs"]["Mods"].push_back(newMod);
                    packListUpdated = true;
                } else if (packType == "Texture" && existingTexturePacks.find(modName) == existingTexturePacks.end()) {
                    // New texture pack found, add it to the TexturePacks list
                    nlohmann::json newTexturePack;
                    newTexturePack["ModName"] = packInfoJson.value("Name", modName); // Fallback to folder name
                    newTexturePack["Description"] = packInfoJson.value("Description", "No description available.");
                    newTexturePack["Version"] = packInfoJson.value("Version", "1.0");
                    newTexturePack["Author"] = packInfoJson.value("Author", "Unknown");
                    newTexturePack["PackType"] = "Texture";
                    newTexturePack["Status"] = "Enabled"; // Enable by default
                    
                    packListJson["Packs"]["TexturePacks"].push_back(newTexturePack);
                    packListUpdated = true;
                }
            }
        }
    }

    // Remove mods from PackList.json that no longer exist in Data
    for (auto it = packListJson["Packs"]["Mods"].begin(); it != packListJson["Packs"]["Mods"].end(); ) {
        std::string modName = (*it)["ModName"];
        if (!std::filesystem::exists(dataDir + "/" + modName)) {
            it = packListJson["Packs"]["Mods"].erase(it); // Remove the missing mod
            packListUpdated = true;
        } else {
            ++it;
        }
    }

    // Remove texture packs from PackList.json that no longer exist in Data
    for (auto it = packListJson["Packs"]["TexturePacks"].begin(); it != packListJson["Packs"]["TexturePacks"].end(); ) {
        std::string texturePackName = (*it)["ModName"];
        if (!std::filesystem::exists(dataDir + "/" + texturePackName)) {
            it = packListJson["Packs"]["TexturePacks"].erase(it); // Remove the missing texture pack
            packListUpdated = true;
        } else {
            ++it;
        }
    }

    // Write the updated PackList.json if any changes were made
    if (packListUpdated) {
        std::ofstream outFile(packListPath);
        outFile << packListJson.dump(4); // Pretty print with 4 spaces
        std::cout << "PackList.json updated successfully." << std::endl;
    }
}

std::string Folders::getResourcePath(const std::string& relativePath) {
    std::string packListPath = dataDir + "/PackList.json";

    // Read PackList.json to get the list of mods and texture packs
    nlohmann::json packListJson;
    std::ifstream packListFile(packListPath);
    if (!packListFile.is_open()) {
        std::cerr << "Unable to open PackList.json." << std::endl;
        return "";
    }
    packListFile >> packListJson;
    packListFile.close();

    // Check texture packs first
    const auto& texturePacks = packListJson["Packs"]["TexturePacks"];
    for (const auto& texturePack : texturePacks) {
        if (texturePack["Status"] == "Enabled") {
            std::string packName = texturePack["ModName"];
            std::string texturePackPath = dataDir + "/" + packName + "/" + relativePath;

            std::string foundFile = findFileCaseInsensitive(texturePackPath);
            if (!foundFile.empty()) {
                return foundFile;  // Return the path if a texture pack contains the file
            }
        }
    }

    // Check mods if not found in texture packs
    const auto& mods = packListJson["Packs"]["Mods"];

     // Check if relativePath starts with a pack name
    for (const auto& mod : mods) {
        std::string modName = mod["ModName"];

        // If relativePath starts with modName, search directly in that mod
        if (relativePath.find(modName + "/") == 0) {
            std::string specificModPath = dataDir + "/" + relativePath;

            // Check if the file exists using case-insensitive search
            std::string foundFile = findFileCaseInsensitive(specificModPath);
            if (!foundFile.empty()) {
                return foundFile; // Return if found directly in the specific mod path
            } else {
                std::cerr << "File not found in specified mod: " << modName << std::endl;
                return ""; // Return empty if the file is not found in the specified mod
            }
        }
    }

    for (const auto& mod : mods) {
        if (mod["Status"] == "Enabled") {
            std::string modName = mod["ModName"];
            std::string modFolderPath = dataDir + "/" + modName + "/" + relativePath;

            std::string foundFile = findFileCaseInsensitive(modFolderPath);
            if (!foundFile.empty()) {
                return foundFile;
            }
        }
    }

    // Check base game folders in Data
    std::string baseGamePath = dataDir + "/" + relativePath;
    if (baseGamePath.find("Data/Data/") != std::string::npos) {
        baseGamePath.replace(baseGamePath.find("Data/Data/"), 10, "Data/");
    }

    std::string foundFile = findFileCaseInsensitive(baseGamePath);
    if (!foundFile.empty()) {
        return foundFile;
    }

    std::cerr << "Resource not found in any path." << std::endl;
    return "";
}


bool Folders::makeDirectory(const std::string& path) {
#ifdef _WIN32
    int status = CreateDirectory(path.c_str(), NULL);
    return ((status != 0) || (GetLastError() == ERROR_ALREADY_EXISTS));
#else
    errno = 0;
    int status = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return ((status == 0) || (errno == EEXIST));
#endif
}

FILE* Folders::openMandatoryFile(const std::string& filename, const char* mode) {
    FILE* tfile = fopen(filename.c_str(), mode);
    if (tfile == NULL) {
        throw FileNotFoundException(filename);
    }
    return tfile;
}

bool Folders::file_exists(const std::string& filepath) {
    FILE* file = fopen(filepath.c_str(), "rb");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

std::string Folders::toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

std::string Folders::findFileCaseInsensitive(const std::string& path) {
    std::filesystem::path p(path);
    std::string directory = p.parent_path().string();
    std::string requestedFilename = p.filename().string();

    if (!std::filesystem::exists(directory)) {
        return "";
    }

    std::filesystem::path fullPath = std::filesystem::path(directory) / requestedFilename;
    if (std::filesystem::exists(fullPath)) {
        return fullPath.string();
    }

    std::string requestedFilenameLower = toLower(requestedFilename);
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        std::string currentFilename = entry.path().filename().string();
        if (toLower(currentFilename) == requestedFilenameLower) {
            return entry.path().string();
        }
    }
    return "";
}

#if PLATFORM_LINUX
std::string Folders::getGenericDirectory(const char* ENVVAR, const std::string& fallback) {
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

const char* Folders::getHomeDirectory() {
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

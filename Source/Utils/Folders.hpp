#ifndef _FOLDERS_HPP_
#define _FOLDERS_HPP_

#include <string>
#include <vector>
#include <algorithm> // Needed for std::transform
#include <cctype>    // Needed for ::tolower
#include <exception>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <unordered_set>

#ifndef DATA_DIR
#define DATA_DIR "Data"
#endif

struct ModInfo {
    std::string folderName;   // Folder name to identify the mod
    std::string infoName;     // Display name of the mod
    std::string description;  // Description of the mod
    std::string version;      // Version of the mod

    static ModInfo fromJson(const std::string& jsonFilePath); // Helper to load from JSON
};

struct FileNotFoundException : public std::exception {
    std::string errorText;
    FileNotFoundException(const std::string &filename)
        : errorText(filename + " could not be found") {}
    const char *what() const throw() { return errorText.c_str(); }
};

class Folders {
    static const std::string dataDir;

public:
    static std::string getScreenshotDir();
    static std::string getUserDataPath();
    static std::string getConfigFilePath();
    static std::string getResourcePath(const std::string& relativePath);

    static nlohmann::json getCampaignData(const std::string &campaignName);

    static std::vector<std::string> getEnabledModCampaigns();

    // Generate PackList.json if not found and update it if needed
    static void createPackListFile();

    // Update existing PackList.json by adding new packs and optionally removing old ones
    static void updatePackList();

    static FILE* openMandatoryFile(const std::string& filename, const char* mode);
    static bool file_exists(const std::string& filepath);

        static inline std::string getUserSavePath()
    {
        return getUserDataPath() + "/users";
    }

    static bool makeDirectory(const std::string& path);
    static std::string findFileCaseInsensitive(const std::string& path);

private:
    static std::string toLower(const std::string& str);
    static std::string getGenericDirectory(const char* ENVVAR, const std::string& fallback);
#if PLATFORM_UNIX
    static const char* getHomeDirectory();
#endif
};

#endif /* _FOLDERS_HPP_ */
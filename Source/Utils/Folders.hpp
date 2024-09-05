#ifndef _FOLDERS_HPP_
#define _FOLDERS_HPP_

#include <string>
#include <vector>
#include <algorithm> // Needed for std::transform
#include <cctype>    // Needed for ::tolower
#include <exception>

#ifndef DATA_DIR
#define DATA_DIR "Data"
#endif

// Define a structure to hold mod information
struct ModInfo {
    std::string folderName;   // Folder name to identify the mod
    std::string infoName;     // Display name of the mod
    std::string description;  // Description of the mod
    std::string version;      // Version of the mod

    // Helper function to load from JSON file
    static ModInfo fromJson(const std::string& jsonFilePath);
};

struct FileNotFoundException : public std::exception
{
    std::string errorText;

    FileNotFoundException(const std::string &filename)
        : errorText(filename + " could not be found")
    {
    }

    const char *what() const throw()
    {
        return errorText.c_str();
    }
};

class Folders
{
    static const std::string dataDir;

public:
    /** Returns path to the screenshot directory. Creates it if needed. */
    static std::string getScreenshotDir();

    static void createAddonsJsonFile();

    /** Returns full path for user data */
    static std::string getUserDataPath();

    /** Returns full path for config file */
    static std::string getConfigFilePath();
    static std::string getSoundResourcePath(const std::string& modName, const std::string& soundFileName);
    /** Returns path for mod resource files */
    static std::string getModResourcePath(const std::string& modName, const std::string& resourceType);

    /** Returns the number of installed mods */
    static int getNumMods();

    /** Returns detailed information of enabled mods */
    static std::vector<ModInfo> getEnabledModsWithInfo();

    static std::vector<std::string> getEnabledMods();

    /** Creates the modlist.txt file in the Mods folder */
    static std::string updateModListFile();
    static std::string createModListFile();

    static FILE* openMandatoryFile(const std::string& filename, const char* mode);

    static bool file_exists(const std::string& filepath);

    /* Returns full path for a game resource */
    static inline std::string getResourcePath(const std::string& filepath)
    {
        return dataDir + '/' + filepath;
    }

    /** Returns full path for user progress save */
    static inline std::string getUserSavePath()
    {
        return getUserDataPath() + "/users";
    }

    static bool makeDirectory(const std::string& path);

    /** Finds a file case-insensitively within a directory */
    static std::string findFileCaseInsensitive(const std::string& path);

    static std::vector<ModInfo> availableMods;
    static std::vector<ModInfo> enabledMods;
    static std::vector<ModInfo> pendingAvailableMods;
    static std::vector<ModInfo> pendingEnabledMods;


private:
    static const char* getHomeDirectory();
    static std::string getGenericDirectory(const char* ENVVAR, const std::string& fallback);

    /** Converts a string to lowercase */
    static std::string toLower(const std::string& str);
};

#endif /* _FOLDERS_H_ */

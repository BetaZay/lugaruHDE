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

You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Level/Campaign.hpp"
#include "Game.hpp"
#include "Utils/Folders.hpp"
#include <dirent.h>

using namespace Game;

std::vector<CampaignLevel> campaignlevels;

bool campaign = false;

int actuallevel = 0;
std::string campaignEndText[3];

std::vector<std::string> ListCampaigns() {
    std::vector<std::string> campaignNames;

    // Check for base game campaigns (Lugaru)
    std::string lugaruCampaignPath = Folders::getResourcePath("Lugaru/Campaign.json");
    if (Folders::file_exists(lugaruCampaignPath)) {
        campaignNames.push_back("Lugaru");
        std::cerr << "Found base game campaign: Lugaru" << std::endl;
    }

    // Now check for campaigns in mod packs by searching for PackName/Campaign.json
    std::string packListPath = Folders::getResourcePath("PackList.json");
    if (!Folders::file_exists(packListPath)) {
        std::cerr << "PackList.json not found!" << std::endl;
        return campaignNames;
    }

    // Open and read PackList.json to get enabled mods
    std::ifstream packListFile(packListPath);
    nlohmann::json packListJson;
    packListFile >> packListJson;
    packListFile.close();

    const auto& mods = packListJson["Packs"]["Mods"];

    // Iterate through each enabled mod and check if a Campaign.json file exists
    for (const auto& mod : mods) {
        if (mod["Status"] == "Enabled") {
            std::string modName = mod["ModName"];
            std::string modCampaignPath = Folders::getResourcePath(modName + "/Campaign.json");

            if (Folders::file_exists(modCampaignPath)) {
                // Found a campaign in this mod, add its name
                campaignNames.push_back(modName);
                std::cerr << "Found campaign in mod: " << modName << std::endl;
            }
        }
    }

    return campaignNames;
}

void LoadCampaign() {
    if (!Account::hasActive()) {
        std::cerr << "No active account found!" << std::endl;
        return;
    }

    std::string campaignPath = Folders::getResourcePath(Account::active().getCurrentCampaign() + "/Campaign.json");
    bool found = false;

    // Try loading the campaign from the current pack
    std::ifstream campaignFile(campaignPath);
    if (campaignFile.good()) {
        found = true;
        std::cerr << "Loaded campaign from: " << campaignPath << std::endl;
    }

    if (!found) {
        std::cerr << "Could not find campaign \"" << Account::active().getCurrentCampaign() << "\", falling back to base game." << std::endl;
        Account::active().setCurrentCampaign("Lugaru");
        LoadCampaign();  // Recursive call to load the base campaign
        return;
    }

    // Parse campaign data if the file is found
    nlohmann::json campaignData;
    try {
        campaignFile >> campaignData;
        campaignFile.close();
    } catch (std::exception& e) {
        std::cerr << "Error parsing campaign JSON: " << e.what() << std::endl;
        return;
    }

    if (!campaignData.contains("CampaignLevels") || !campaignData.contains("Levels")) {
        std::cerr << "Invalid campaign JSON structure!" << std::endl;
        return;
    }

    campaignlevels.clear();

    // Loop through each level in the campaign
    for (const auto& levelData : campaignData["Levels"]) {
        CampaignLevel cl;

        if (!levelData.contains("Name") || !levelData.contains("Description")) {
            std::cerr << "Level data missing mandatory fields!" << std::endl;
            continue;
        }

        cl.mapname = levelData["Name"];
        cl.description = levelData["Description"];
        std::replace(cl.description.begin(), cl.description.end(), '_', ' ');

        // Handle optional fields
        cl.choosenext = levelData.value("ChooseNext", 0);  // Default to 0 if not present
        cl.location.x = levelData.value("LocationX", 0);  // Default to (0,0) if not present
        cl.location.y = levelData.value("LocationY", 0);

        // Ensure next levels are read properly
        int numNext = levelData.value("NumNext", 0);
        if (numNext > 0 && levelData.contains("NextLevel")) {
            if (levelData["NextLevel"].is_array()) {
                for (int i = 0; i < numNext; i++) {
                    cl.nextlevel.push_back(levelData["NextLevel"][i].get<int>() - 1);  // Convert to int and subtract 1
                }
            } else {
                cl.nextlevel.push_back(levelData["NextLevel"].get<int>() - 1);  // Convert to int and subtract 1
            }
        }

        campaignlevels.push_back(cl);
    }

    // Set end text if available
    if (campaignData.contains("EndText")) {
        campaignEndText[0] = "Congratulations!";
        campaignEndText[1] = campaignData["EndText"];
        campaignEndText[2] = "";
    }

    // Load World.png texture with fallback logic
    std::string worldTexturePath = Folders::getResourcePath(Account::active().getCurrentCampaign() + "/Textures/World.png");

    if (worldTexturePath.empty()) {
        std::cerr << "World.png not found in the current campaign, trying fallback." << std::endl;
        worldTexturePath = Folders::getResourcePath("Lugaru/Textures/World.png");

        if (worldTexturePath.empty()) {
            std::cerr << "Fallback to base game World.png failed!" << std::endl;
        } else {
            std::cerr << "Loaded fallback World.png from base game." << std::endl;
        }
    }

    if (!worldTexturePath.empty()) {
        Mainmenuitems[7].load(worldTexturePath, 0);
    }

    // Reset campaign progress if no choices were made yet
    if (Account::active().getCampaignChoicesMade() == 0) {
        Account::active().setCampaignScore(0);
        Account::active().resetFasttime();
    }
}

CampaignLevel::CampaignLevel()
    : width(10)
    , choosenext(1)
{
    location.x = 0;
    location.y = 0;
}

int CampaignLevel::getStartX()
{
    return 30 + 120 + location.x * 400 / 512;
}

int CampaignLevel::getStartY()
{
    return 30 + 30 + (512 - location.y) * 400 / 512;
}

int CampaignLevel::getEndX()
{
    return getStartX() + width;
}

int CampaignLevel::getEndY()
{
    return getStartY() + width;
}

XYZ CampaignLevel::getCenter()
{
    XYZ center;
    center.x = getStartX() + width / 2;
    center.y = getStartY() + width / 2;
    return center;
}

int CampaignLevel::getWidth()
{
    return width;
}

istream& CampaignLevel::operator<<(istream& is)
{
    is.ignore(256, ':');
    is.ignore(256, ':');
    is.ignore(256, ' ');
    is >> mapname;
    is.ignore(256, ':');
    is >> description;
    for (size_t pos = description.find('_'); pos != string::npos; pos = description.find('_', pos)) {
        description.replace(pos, 1, 1, ' ');
    }
    is.ignore(256, ':');
    is >> choosenext;
    is.ignore(256, ':');
    int numnext, next;
    is >> numnext;
    for (int j = 0; j < numnext; j++) {
        is.ignore(256, ':');
        is >> next;
        nextlevel.push_back(next - 1);
    }
    is.ignore(256, ':');
    is >> location.x;
    is.ignore(256, ':');
    is >> location.y;
    return is;
}

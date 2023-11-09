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

std::vector<std::string> ListCampaigns()
{
    std::vector<std::string> campaignNames;

    // Load campaigns from the Data folder and enabled mods
    std::string dataPath = Folders::getResourcePath("/Campaigns");
    if (!dataPath.empty()) {
        DIR* campaignsDir = opendir(dataPath.c_str());
        struct dirent* campaign = NULL;
        if (campaignsDir) {
            while ((campaign = readdir(campaignsDir)) != NULL) {
                std::string name(campaign->d_name);
                if (name.length() < 5) {
                    continue;
                }
                if (!name.compare(name.length() - 4, 4, ".txt")) {
                    campaignNames.push_back(name.substr(0, name.length() - 4));
                }
            }
            closedir(campaignsDir);
        }
    }

    // Load campaigns from enabled mods
    std::vector<std::string> enabledMods = Folders::getEnabledMods();
    for (const auto& mod : enabledMods) {
        std::string modPath = Folders::getModResourcePath(mod, "/Campaigns");
        if (!modPath.empty()) {
            DIR* modCampaignsDir = opendir(modPath.c_str());
            struct dirent* modCampaign = NULL;
            if (modCampaignsDir) {
                while ((modCampaign = readdir(modCampaignsDir)) != NULL) {
                    std::string name(modCampaign->d_name);
                    if (name.length() < 5) {
                        continue;
                    }
                    if (!name.compare(name.length() - 4, 4, ".txt")) {
                        campaignNames.push_back(name.substr(0, name.length() - 4));
                    }
                }
                closedir(modCampaignsDir);
            }
        }
    }

    return campaignNames;
}

void LoadCampaign()
{
    if (!Account::hasActive()) {
        return;
    }
    std::string campaignPath = Folders::getResourcePath("/Campaigns/" + Account::active().getCurrentCampaign() + ".txt");
    bool found = false;

    // Check the main Data folder first
    std::ifstream ipstream(campaignPath);
    if (ipstream.good()) {
        found = true;
        ipstream.close();
    } else {
        // Look for the campaign in enabled mods
        for (const auto &mod : Folders::getEnabledMods()) {
            std::string modCampaignPath = Folders::getModResourcePath(mod, "/Campaigns/" + Account::active().getCurrentCampaign() + ".txt");
            std::ifstream modStream(modCampaignPath);
            if (modStream.good()) {
                campaignPath = modCampaignPath;
                found = true;
                modStream.close();
                break;
            }
            modStream.close();
        }
    }

    if (!found) {
        if (Account::active().getCurrentCampaign() == "lugaru") {
            std::cerr << "Could not find main campaign!" << std::endl;
            return;
        }
        std::cerr << "Could not find campaign \"" << Account::active().getCurrentCampaign() << "\", falling back to main." << std::endl;
        Account::active().setCurrentCampaign("lugaru");
        LoadCampaign();
        return;
    }

    ipstream.open(campaignPath);
    ipstream.ignore(256, ':');
    int numlevels;
    ipstream >> numlevels;
    campaignlevels.clear();
    for (int i = 0; i < numlevels; i++) {
        CampaignLevel cl;
        ipstream >> cl;
        campaignlevels.push_back(cl);
    }
    campaignEndText[0] = "Congratulations!";
    campaignEndText[1] = "You have completed " + Account::active().getCurrentCampaign() + " campaign";
    campaignEndText[2] = "and restored peace to the island of Lugaru.";
    if (ipstream.good()) {
        ipstream.ignore(256, ':');
        getline(ipstream, campaignEndText[0]);
        getline(ipstream, campaignEndText[1]);
        getline(ipstream, campaignEndText[2]);
    }
    ipstream.close();

    std::ifstream test(Folders::getResourcePath("/Textures/" + Account::active().getCurrentCampaign() + "/World.png"));
    if (test.good()) {
        Mainmenuitems[7].load("Textures/" + Account::active().getCurrentCampaign() + "/World.png", 0);
    } else {
        Mainmenuitems[7].load("Textures/World.png", 0);
    }

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

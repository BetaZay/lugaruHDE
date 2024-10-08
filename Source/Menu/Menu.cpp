// Menu/Menu.cpp

#include "Menu/Menu.hpp"
#include "Audio/openal_wrapper.hpp"
#include "Graphic/gamegl.hpp"
#include "Level/Campaign.hpp"
#include "User/Settings.hpp"
#include "Utils/Input.hpp"
#include "Version.hpp"
#include "Utils/Folders.hpp"
#include "Game.hpp"
#include "Level/Awards.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm> 
#include "Menu.hpp"

using namespace Game;

extern float multiplier;
extern std::set<std::pair<int, int>> resolutions;
extern int mainmenu;
extern std::vector<CampaignLevel> campaignlevels;
extern float musicvolume[4];
extern float oldmusicvolume[4];
extern bool stillloading;
extern bool visibleloading;
extern int whichchoice;
extern int leveltheme;

extern void toggleFullscreen();

int entername = 0;
int selectedModIndex = -1;
std::string newusername = "";
unsigned newuserselected = 0;
float newuserblinkdelay = 0;
bool newuserblink = false;

std::vector<MenuItem> Menu::items;

std::vector<ModInfo> availableMods;
std::vector<ModInfo> enabledMods;

std::vector<ModInfo> pendingAvailableMods;
std::vector<ModInfo> pendingEnabledMods;

std::vector<std::string> Menu::wrapText(const std::string& text, int maxWidth) {
    std::vector<std::string> wrappedLines;
    std::string line;
    std::istringstream stream(text);
    std::string word;

    while (stream >> word) {
        if (getTextWidth(line.empty() ? word : line + " " + word) > maxWidth) {
            // Push the current line and start a new one with the current word
            wrappedLines.push_back(line);
            line = word;
        } else {
            // Add word to the line
            if (!line.empty()) {
                line += " "; // Add space before word if line is not empty
            }
            line += word;
        }
    }

    // Add the last line if it's not empty
    if (!line.empty()) {
        wrappedLines.push_back(line);
    }

    return wrappedLines;
}

int Menu::getTextWidth(const std::string& text) {
    // Assuming a fixed-width or average character size for simplicity
    int averageCharWidth = 8;  // Adjust this value based on the actual font used
    return text.length() * averageCharWidth;
}

MenuItem::MenuItem(MenuItemType _type, int _id, const std::string& _text, Texture _texture,
                int _x, int _y, int _w, int _h, float _r, float _g, float _b,
                float _rotation, float _linestartsize, float _lineendsize)
    : type(_type)
    , id(_id)
    , text(_text)
    , texture(_texture)
    , x(_x)
    , y(_y)
    , w(_w)
    , h(_h)
    , r(_r)
    , g(_g)
    , b(_b)
    , effectfade(0)
    , rotation(_rotation)  // Initialize the rotation here
    , linestartsize(_linestartsize)
    , lineendsize(_lineendsize)
{
    // Additional logic for BUTTON type to auto-calculate width and height if not provided
    if (type == MenuItem::BUTTON) {
        if (w == -1) {
            w = text.length() * 10;  // Auto-calculate width based on text length
        }
        if (h == -1) {
            h = 20;  // Set a default height
        }
    }
}


/**
 * @brief Clears all menu items.
 */
void Menu::clearMenu()
{
    items.clear();
}

/**
 * @brief Adds a label to the menu.
 * 
 * @param id The identifier for the label.
 * @param text The text to display on the label.
 * @param x The x-coordinate of the label.
 * @param y The y-coordinate of the label.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 */
void Menu::addLabel(int id, const std::string& text, int x, int y, float r, float g, float b)
{
    items.emplace_back(MenuItem::LABEL, id, text, Texture(), x, y, -1, -1, r, g, b);
}

/**
 * @brief Adds a button to the menu.
 * 
 * @param id The identifier for the button.
 * @param text The text to display on the button.
 * @param x The x-coordinate of the button.
 * @param y The y-coordinate of the button.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 */
void Menu::addButton(int id, const std::string& text, int x, int y, float r, float g, float b)
{
    items.emplace_back(MenuItem::BUTTON, id, text, Texture(), x, y, -1, -1, r, g, b);
}

/**
 * @brief Adds an image to the menu.
 * 
 * @param id The identifier for the image.
 * @param texture The texture to use for the image.
 * @param x The x-coordinate of the image.
 * @param y The y-coordinate of the image.
 * @param w The width of the image.
 * @param h The height of the image.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 */
void Menu::addImage(int id, Texture texture, int x, int y, int w, int h, float r, float g, float b)
{
    items.emplace_back(MenuItem::IMAGE, id, "", texture, x, y, w, h, r, g, b);
}

/**
 * @brief Adds a button with an image to the menu.
 * 
 * @param id The identifier for the image button.
 * @param texture The texture to use for the image button.
 * @param x The x-coordinate of the image button.
 * @param y The y-coordinate of the image button.
 * @param w The width of the image button.
 * @param h The height of the image button.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 * @param rotation The rotation angle (in degrees) to apply to the image button.
 */
void Menu::addButtonImage(int id, Texture texture, int x, int y, int w, int h, float r, float g, float b, float rotation) {
    items.emplace_back(MenuItem(MenuItem::IMAGEBUTTON, id, "", texture, x, y, w, h, r, g, b, rotation));
}

/**
 * @brief Adds a line to the map.
 * 
 * @param x The starting x-coordinate of the line.
 * @param y The starting y-coordinate of the line.
 * @param w The width of the line.
 * @param h The height of the line.
 * @param startsize The starting size of the line.
 * @param endsize The ending size of the line.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 */
void Menu::addMapLine(int x, int y, int w, int h, float startsize, float endsize, float r, float g, float b)
{
    items.emplace_back(MenuItem::MAPLINE, -1, "", Texture(), x, y, w, h, r, g, b, 0.0f, startsize, endsize);
}

/**
 * @brief Adds a marker to the map.
 * 
 * @param id The identifier for the map marker.
 * @param texture The texture to use for the map marker.
 * @param x The x-coordinate of the map marker.
 * @param y The y-coordinate of the map marker.
 * @param w The width of the map marker.
 * @param h The height of the map marker.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 */
void Menu::addMapMarker(int id, Texture texture, int x, int y, int w, int h, float r, float g, float b)
{
    items.emplace_back(MenuItem::MAPMARKER, id, "", texture, x, y, w, h, r, g, b);
}

/**
 * @brief Adds a label to the map.
 * 
 * @param id The identifier for the map label.
 * @param text The text to display on the label.
 * @param x The x-coordinate of the map label.
 * @param y The y-coordinate of the map label.
 * @param r The red color component (0.0 to 1.0).
 * @param g The green color component (0.0 to 1.0).
 * @param b The blue color component (0.0 to 1.0).
 */
void Menu::addMapLabel(int id, const std::string& text, int x, int y, float r, float g, float b)
{
    items.emplace_back(MenuItem::MAPLABEL, id, text, Texture(), x, y, -1, -1, r, g, b);
}

void Menu::addLineRect(int id, int x, int y, int w, int h, float r, float g, float b) {
    items.emplace_back(MenuItem::LINERECT, id, "", Texture(), x, y, w, h, r, g, b);
}

void Menu::addRectButton(int id, int x, int y, int w, int h, float r = 1, float g = 1, float b = 1) {
    items.emplace_back(MenuItem::RECTBUTTON, id, "", Texture(), x, y, w, h, r, g, b);
}

void Menu::setText(int id, const std::string& text)
{
    for (auto& item : items) {
        if (item.id == id) {
            item.text = text;
            item.w = item.text.length() * 10;
            break;
        }
    }
}

void Menu::setText(int id, const std::string& text, int x, int y, int w, int h)
{
    for (auto& item : items) {
        if (item.id == id) {
            item.text = text;
            item.x = x;
            item.y = y;
            if (w == -1) {
                item.w = item.text.length() * 10;
            }
            if (h == -1) {
                item.h = 20;
            }
            break;
        }
    }
}

int Menu::getSelected(int mousex, int mousey) {
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (it->type == MenuItem::BUTTON || it->type == MenuItem::IMAGEBUTTON || it->type == MenuItem::MAPMARKER || it->type == MenuItem::RECTBUTTON) {
            int mx = mousex;
            int my = mousey;

            if (mx >= it->x && mx < it->x + it->w && my >= it->y && my < it->y + it->h) {
                return it->id;
            }
        }
    }
    return -1;
}

void Menu::handleFadeEffect()
{
    for (auto& item : items) {
        if (item.id == Game::selected) {
            item.effectfade += multiplier * 5;
            if (item.effectfade > 1) {
                item.effectfade = 1;
            }
        } else {
            item.effectfade -= multiplier * 5;
            if (item.effectfade < 0) {
                item.effectfade = 0;
            }
        }
    }
}

void Menu::drawItems() {
    handleFadeEffect();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);

    for (auto it = items.begin(); it != items.end(); ++it) {
        glPushMatrix();  // Save the current transformation state

        switch (it->type) {
            case MenuItem::IMAGE:
            case MenuItem::IMAGEBUTTON:
            case MenuItem::MAPMARKER:
                if (it->id >= 1000 && it->id < 2000 && it->type == MenuItem::IMAGE) {  // Assuming 1000-2000 are for mod pack images
                    // Set the color with full opacity (the alpha will come from the texture itself)
                    glAlphaFunc(GL_GREATER, 0.2f);
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Opaque RGB, respect alpha for transparency

                    // Set the color with full opacity (the alpha will come from the texture itself)
                    glColor4f(it->r, it->g, it->b, 1);

                    it->texture.bind();
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    // Render the textured quad
                    glBegin(GL_QUADS);
                    glTexCoord2f(0, 0); glVertex3f(it->x, it->y, 0);
                    glTexCoord2f(1, 0); glVertex3f(it->x + it->w, it->y, 0);
                    glTexCoord2f(1, 1); glVertex3f(it->x + it->w, it->y + it->h, 0);
                    glTexCoord2f(0, 1); glVertex3f(it->x, it->y + it->h, 0);
                    glEnd();
                } else {
                    glEnable(GL_BLEND);
                    glColor4f(it->r, it->g, it->b, 1);
                    // Set the blend function for MAPMARKER and others
                    if (it->type == MenuItem::MAPMARKER) {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glTranslatef(2.5, -4.5, 0); // Old marker-specific translation
                    } else {
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    }

                    // Apply rotation only for IMAGEBUTTON type
                    if (it->type == MenuItem::IMAGEBUTTON) {
                        if (it->rotation != 0.0f) {
                            // Calculate the center of the item
                            float centerX = it->x + it->w / 2;
                            float centerY = it->y + it->h / 2;

                            // Apply the transformation
                            glTranslatef(centerX, centerY, 0);
                            glRotatef(it->rotation, 0.0f, 0.0f, 1.0f);
                            glTranslatef(-centerX, -centerY, 0);
                        }
                    }

                    // Bind the texture
                    it->texture.bind();
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    // Render the textured quad
                    glBegin(GL_QUADS);
                    glTexCoord2f(0, 0); glVertex3f(it->x, it->y, 0);
                    glTexCoord2f(1, 0); glVertex3f(it->x + it->w, it->y, 0);
                    glTexCoord2f(1, 1); glVertex3f(it->x + it->w, it->y + it->h, 0);
                    glTexCoord2f(0, 1); glVertex3f(it->x, it->y + it->h, 0);
                    glEnd();

                    if (it->type != MenuItem::IMAGE) {
                        // Mouseover highlight effect
                        for (int i = 0; i < 10; i++) {
                            if (1 - ((float)i) / 10 - (1 - it->effectfade) > 0) {
                                glColor4f(it->r, it->g, it->b, (1 - ((float)i) / 10 - (1 - it->effectfade)) * .25);
                                glBegin(GL_QUADS);
                                glTexCoord2f(0, 0); glVertex3f(it->x - ((float)i) * 1 / 2, it->y - ((float)i) * 1 / 2, 0);
                                glTexCoord2f(1, 0); glVertex3f(it->x + it->w + ((float)i) * 1 / 2, it->y - ((float)i) * 1 / 2, 0);
                                glTexCoord2f(1, 1); glVertex3f(it->x + it->w + ((float)i) * 1 / 2, it->y + it->h + ((float)i) * 1 / 2, 0);
                                glTexCoord2f(0, 1); glVertex3f(it->x - ((float)i) * 1 / 2, it->y + it->h + ((float)i) * 1 / 2, 0);
                                glEnd();
                            }
                        }
                    }
                }
                glPopMatrix(); // Reset transformations
                break;

            case MenuItem::LABEL:
            case MenuItem::BUTTON:
                glColor4f(it->r, it->g, it->b, 1);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                if (it->id >= 1000) {
                    Game::text->glPrint(it->x, it->y, it->text.c_str(), 0, 0.7, 640, 480);
                } else {
                    Game::text->glPrint(it->x, it->y, it->text.c_str(), 0, 1, 640, 480);
                }

                if (it->type != MenuItem::LABEL) {
                    // Mouseover highlight effect for buttons
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    for (int i = 0; i < 15; i++) {
                        if (1 - ((float)i) / 15 - (1 - it->effectfade) > 0) {
                            glColor4f(it->r, it->g, it->b, (1 - ((float)i) / 10 - (1 - it->effectfade)) * .25);
                            Game::text->glPrint(it->x - ((float)i), it->y, it->text.c_str(), 0, 1 + ((float)i) / 70, 640, 480);
                        }
                    }
                }
                break;
            case MenuItem::MAPLABEL:
                Game::text->glPrintOutlined(0.9, 0, 0, 1, it->x, it->y, it->text.c_str(), 0, 0.6, 640, 480);
                break;

            case MenuItem::MAPLINE: {
                XYZ linestart;
                linestart.x = it->x;
                linestart.y = it->y;
                linestart.z = 0;
                XYZ lineend;
                lineend.x = it->x + it->w;
                lineend.y = it->y + it->h;
                lineend.z = 0;
                XYZ offset = lineend - linestart;
                XYZ fac = offset;
                Normalise(&fac);
                offset = DoRotation(offset, 0, 0, 90);
                Normalise(&offset);

                linestart += fac * 4 * it->linestartsize;
                lineend -= fac * 4 * it->lineendsize;

                glDisable(GL_TEXTURE_2D);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(it->r, it->g, it->b, 1);
                glPushMatrix();
                glTranslatef(2, -5, 0); //from old code
                glBegin(GL_QUADS);
                glVertex3f(linestart.x - offset.x * it->linestartsize, linestart.y - offset.y * it->linestartsize, 0.0f);
                glVertex3f(linestart.x + offset.x * it->linestartsize, linestart.y + offset.y * it->linestartsize, 0.0f);
                glVertex3f(lineend.x + offset.x * it->lineendsize, lineend.y + offset.y * it->lineendsize, 0.0f);
                glVertex3f(lineend.x - offset.x * it->lineendsize, lineend.y - offset.y * it->lineendsize, 0.0f);
                glEnd();
                glPopMatrix();
                glEnable(GL_TEXTURE_2D);
            } break;

            default:
            case MenuItem::NONE:
                break;
        }

        glPopMatrix();  // Restore the previous transformation state

        for (auto it = items.begin(); it != items.end(); ++it) {
                glPushMatrix();  // Save the current transformation state

                if (it->type == MenuItem::LINERECT) {

                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow effect

                    // Loop to create multiple layers for the glow effect
                    for (int i = 0; i < 3; i++) {
                        float alpha = (1.0f - ((float)i) / 10) * 0.5f;  // Adjust alpha for fading glow effect
                        float lineWidth = 5.0f + i * 0.5f;  // Increase line width for outer glow layers
                        glLineWidth(lineWidth);
                        
                        // Set the color with adjusted alpha
                        glColor4f(it->r, it->g, it->b, alpha);
                        
                        // Draw the glowing rectangle
                        glBegin(GL_LINE_LOOP);
                        glVertex2f(it->x, it->y);             // Top-left
                        glVertex2f(it->x + it->w, it->y);     // Top-right
                        glVertex2f(it->x + it->w, it->y + it->h);  // Bottom-right
                        glVertex2f(it->x, it->y + it->h);     // Bottom-left
                        glEnd();
                    }

                    // Draw the main rectangle line with full opacity
                    glLineWidth(5.0f);  // Reset to the desired line width
                    glColor4f(it->r, it->g, it->b, 1.0f);  // Full opacity for the main line
                    glBegin(GL_LINE_LOOP);
                    glVertex2f(it->x, it->y);             // Top-left
                    glVertex2f(it->x + it->w, it->y);     // Top-right
                    glVertex2f(it->x + it->w, it->y + it->h);  // Bottom-right
                    glVertex2f(it->x, it->y + it->h);     // Bottom-left
                    glEnd();

                    // Reset line width (optional but safe)
                    glLineWidth(1.0f);
                }

                glPopMatrix();
        }
    }
}

void Menu::updateSettingsMenu()
{
    std::string sbuf = std::string("Resolution: ") + to_string(newscreenwidth) + "*" + to_string(newscreenheight);
    if (((float)newscreenwidth <= (float)newscreenheight * 1.61) && ((float)newscreenwidth >= (float)newscreenheight * 1.59)) {
        sbuf += " (widescreen)";
    }
    setText(0, sbuf);
    setText(14, fullscreen ? "Fullscreen: On" : "Fullscreen: Off");
    if (newdetail == 0) {
        setText(1, "Detail: Low");
    }
    if (newdetail == 1) {
        setText(1, "Detail: Medium");
    }
    if (newdetail == 2) {
        setText(1, "Detail: High");
    }
    if (bloodtoggle == 0) {
        setText(2, "Blood: Off");
    }
    if (bloodtoggle == 1) {
        setText(2, "Blood: On, low detail");
    }
    if (bloodtoggle == 2) {
        setText(2, "Blood: On, high detail (slower)");
    }
    setText(4, ismotionblur ? "Blur Effects: Enabled (less compatible)" : "Blur Effects: Disabled (more compatible)");
    setText(5, decalstoggle ? "Decals: Enabled (slower)" : "Decals: Disabled");
    setText(6, musictoggle ? "Music: Enabled" : "Music: Disabled");
    setText(9, invertmouse ? "Invert mouse: Yes" : "Invert mouse: No");
    setText(10, std::string("Mouse Speed: ") + to_string(int(usermousesensitivity * 5)));
    setText(11, std::string("Volume: ") + to_string(int(volume * 100)) + "%");
    setText(13, showdamagebar ? "Damage Bar: On" : "Damage Bar: Off");
    if ((newdetail == detail) && (newscreenheight == (int)screenheight) && (newscreenwidth == (int)screenwidth)) {
        setText(8, "Back");
    } else {
        setText(8, "Back (some changes take effect next time Lugaru is opened)");
    }
    setText(15, "Mods");
}

void Menu::updateStereoConfigMenu()
{
    setText(0, std::string("Stereo mode: ") + StereoModeName(newstereomode));
    setText(1, std::string("Stereo separation: ") + to_string(stereoseparation));
    setText(2, std::string("Reverse stereo: ") + (stereoreverse ? "Yes" : "No"));
}

void Menu::updateControlsMenu()
{
    setText(0, (std::string) "Forwards: " + (keyselect == 0 ? "_" : Input::keyToChar(forwardkey)));
    setText(1, (std::string) "Back: " + (keyselect == 1 ? "_" : Input::keyToChar(backkey)));
    setText(2, (std::string) "Left: " + (keyselect == 2 ? "_" : Input::keyToChar(leftkey)));
    setText(3, (std::string) "Right: " + (keyselect == 3 ? "_" : Input::keyToChar(rightkey)));
    setText(4, (std::string) "Crouch: " + (keyselect == 4 ? "_" : Input::keyToChar(crouchkey)));
    setText(5, (std::string) "Jump: " + (keyselect == 5 ? "_" : Input::keyToChar(jumpkey)));
    setText(6, (std::string) "Draw: " + (keyselect == 6 ? "_" : Input::keyToChar(drawkey)));
    setText(7, (std::string) "Throw: " + (keyselect == 7 ? "_" : Input::keyToChar(throwkey)));
    setText(8, (std::string) "Attack: " + (keyselect == 8 ? "_" : Input::keyToChar(attackkey)));
    if (devtools) {
        setText(9, (std::string) "Console: " + (keyselect == 9 ? "_" : Input::keyToChar(consolekey)));
    }
}

void Menu::updateModsMenu() {
    try {
        // Read PackList.json to get the list of packs
        std::string packListPath = Folders::getResourcePath("/PackList.json");
        if (!Folders::file_exists(packListPath)) {
            Folders::createPackListFile(); // This creates the PackList.json
        }

        std::ifstream packListFile(packListPath);
        nlohmann::json packListJson;
        availableMods.clear();  // Use availableMods to store all mods

        if (packListFile.is_open()) {
            packListFile >> packListJson;

            const auto& mods = packListJson["Packs"]["Mods"];
            for (const auto& mod : mods) {
                ModInfo modInfo;
                modInfo.folderName = mod["ModName"];
                modInfo.infoName = mod["ModName"];  // Default to folder name for display
                modInfo.description = mod["Description"];
                modInfo.version = mod["Version"];
                
                availableMods.push_back(modInfo);  // Push all mods into availableMods
            }

            packListFile.close();
        } else {
            std::cerr << "Unable to open PackList.json." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }

    drawModMenu();
}

void Menu::drawModMenu() {
    clearMenu();

    // Define UI layout dimensions
    int menuPadding = 30;
    int leftBoxX = menuPadding;
    int leftBoxY = 50;
    int leftBoxW = 200;
    int leftBoxH = 400;
    int rightBoxX = leftBoxX + leftBoxW + menuPadding;
    //int rightBoxY = leftBoxY;
    //int rightBoxW = 300;  // Wider box for mod details
    int rightBoxH = leftBoxH;

    int cardHeight = 70;
    int cardPadding = 5;

    // Draw the bounding boxes for mods and mod details
    //addLineRect(8000, leftBoxX, leftBoxY, leftBoxW, leftBoxH, 1.0f, 0.0f, 0.0f);  // Red bounding box for mods
    //addLineRect(8001, rightBoxX, rightBoxY, rightBoxW, rightBoxH, 1.0f, 0.0f, 0.0f); // Red bounding box for mod details

    // Render mods with `pack.png` and mod name, starting from the top
    int yPos = leftBoxY + leftBoxH - cardHeight;  // Start from the top of the box
    int packIconSize = 45;

    int ydPos = leftBoxY + rightBoxH; 
    // If a mod is selected, display its details on the right side
    if (selectedModIndex >= 0 && selectedModIndex < availableMods.size()) {
        ModInfo modInfo = availableMods[selectedModIndex];

        // Display mod details (name, description, version)
        addLabel(4000, "Mod Name: " + modInfo.infoName, rightBoxX + 10, ydPos - 30, 1, 1, 1);
        addLabel(4001, "Version: " + modInfo.version, rightBoxX + 10, ydPos - 60, 1, 1, 1);
        addLabel(4002, "Description: " + modInfo.description, rightBoxX + 10, ydPos - 90, 1, 1, 1);
    }

    for (size_t i = 0; i < availableMods.size(); ++i) {
        ModInfo modInfo = availableMods[i];
        std::string modInfoName = modInfo.infoName;

        if (i == selectedModIndex) {
            addLineRect(9000 + i, leftBoxX, yPos, leftBoxW, cardHeight, 1.0f, 0.0f, 0.0f);  // White rectangle for the selected mod
        }

        // Draw the mod's `pack.png` on the left
        Texture modTexture;
        modTexture.load(modInfo.folderName + "/pack.png", 0);
        addImage(1000 + i, modTexture, leftBoxX + cardPadding, yPos + (packIconSize / 2) - (cardPadding * 2), packIconSize, packIconSize, 1.0f, 1.0f, 1.0f);

        // Draw the mod name next to the image
        addLabel(2000 + i, modInfoName, leftBoxX + cardPadding + packIconSize, yPos + packIconSize, 1, 1, 1);

        // Add an invisible rectangular button over the mod's area
        addRectButton(3000 + i, leftBoxX, yPos, leftBoxW, cardHeight);  // Invisible rect button

        // Move to the next position for the next card, going downward
        yPos -= cardHeight + cardPadding;
    }

    // Back button
    addButton(1, "Back", 10, 10, 1.0f, 0.0f, 0.0f);
}


void Menu::handleArrowButtonPress(int buttonId)
{
    if (buttonId >= 1000 && buttonId < 1000 + static_cast<int>(availableMods.size())) {
        size_t packIndex = static_cast<size_t>(buttonId - 1000);
        if (packIndex < availableMods.size()) {
            // Move pack from available to enabled
            enabledMods.push_back(availableMods[packIndex]);
            availableMods.erase(availableMods.begin() + packIndex);
            drawModMenu();
        }
    }
}

void Menu::applyModChanges() {
    std::cout << "Applying mod changes..." << std::endl;

    // Update PackList.json with changes
    std::string packListPath = Folders::getResourcePath("/PackList.json");
    std::ofstream packListFile(packListPath);

    if (packListFile.is_open()) {
        nlohmann::json packListJson;

        // Save available mods
        for (const auto& mod : availableMods) {
            nlohmann::json modEntry;
            modEntry["ModName"] = mod.folderName;
            modEntry["Status"] = "Available";
            modEntry["Description"] = mod.description;
            modEntry["Version"] = mod.version;
            packListJson["Packs"]["Mods"].push_back(modEntry);
        }

        // Save enabled mods
        for (const auto& mod : enabledMods) {
            nlohmann::json modEntry;
            modEntry["ModName"] = mod.folderName;
            modEntry["Status"] = "Enabled";
            modEntry["Description"] = mod.description;
            modEntry["Version"] = mod.version;
            packListJson["Packs"]["Mods"].push_back(modEntry);
        }

        packListFile << packListJson.dump(4);  // Pretty-print with 4 spaces
        packListFile.close();
        std::cout << "Mod changes saved to PackList.json." << std::endl;
    } else {
        std::cerr << "Unable to open PackList.json for writing." << std::endl;
    }

    // Reload game assets with the newly applied packs
    Game::reloadGameAssets();
    fireSound();
    flash();
    std::cout << "Mod changes applied successfully." << std::endl;
}

/*
Values of mainmenu :
1 Main menu
2 Menu pause (resume/end game)
3 Option menu
4 Controls configuration menu
5 Main game menu (choose level or challenge) / The Map Screen
6 Deleting user menu
7 User managment menu (select/add)
8 Choose difficulty menu
9 Challenge level selection menu
10 End of the campaign congratulation (is that really a menu?)
11 Same that 9 ??? => unused
18 stereo configuration
19 Mod menu
*/

void Menu::Load()
{
    clearMenu();
    switch (mainmenu) {
        case 1:
        case 2:
            addImage(0, Mainmenuitems[0], 150, 480 - 128, 256, 128); //Lugaru logo
            addButtonImage(1, Mainmenuitems[mainmenu == 1 ? 1 : 5], 18, 320, 128, 32); // Start if main menu, Resume if pause menu
            addButtonImage(3, Mainmenuitems[mainmenu == 1 ? 8 : 9], 18, 240, 128, 32); // Mods if main menu, Restart if pause menu
            addButtonImage(2, Mainmenuitems[2], 18, 160, 112, 32); // Options
            addButtonImage(4, Mainmenuitems[mainmenu == 1 ? 3 : 6], 18, 80, mainmenu == 1 ? 68 : 132, 32); // Quit if main menu, EndGame if pause menu
            addLabel(-1, VERSION_NUMBER + VERSION_SUFFIX, 640 - 100, 10);
            break;
        case 3:
            addButton(0, "", 10 + 20, 440);
            addButton(14, "", 10 + 400, 440);
            addButton(1, "", 10 + 60, 405);
            if (!gameon) {
                addButton(15, "", 10 + 400, 405);
            }
            addButton(2, "", 10 + 70, 370);
            addButton(4, "", 10, 335);
            addButton(5, "", 10 + 60, 300);
            addButton(6, "", 10 + 70, 265);
            addButton(9, "", 10, 230);
            addButton(10, "", 20, 195);
            addButton(11, "", 10 + 60, 160);
            addButton(13, "", 30, 125);
            addButton(7, "-Configure Controls-", 10 + 15, 90);
            addButton(12, "-Configure Stereo -", 10 + 15, 55);
            addButton(8, "Back", 10, 10);
            updateSettingsMenu();
            break;
        case 4:
            addButton(0, "", 10, 400);
            addButton(1, "", 10 + 40, 360);
            addButton(2, "", 10 + 40, 320);
            addButton(3, "", 10 + 30, 280);
            addButton(4, "", 10 + 20, 240);
            addButton(5, "", 10 + 40, 200);
            addButton(6, "", 10 + 40, 160);
            addButton(7, "", 10 + 30, 120);
            addButton(8, "", 10 + 20, 80);
            if (devtools) {
                addButton(9, "", 10 + 10, 40);
            }
            addButton(devtools ? 10 : 9, "Back", 10, 10);
            updateControlsMenu();
            break;
    case 5: {
        // Load the selected campaign
        LoadCampaign();

        // Add the user’s name and main buttons for interaction
        addLabel(-1, Account::active().getName(), 5, 400);
        addButton(1, "Tutorial", 5, 300);
        addButton(2, "Challenge", 5, 240);
        addButton(3, "Delete User", 400, 10);
        addButton(4, "Main Menu", 5, 10);
        addButton(5, "Change User", 5, 180);
        addButton(6, "Campaign: " + Account::active().getCurrentCampaign(), 200, 420);

        // Display the campaign map image
        addImage(-1, Mainmenuitems[7], 150 + 2, 60 - 5, 400, 400);

        // Get the number of levels that should be displayed
        int numLevelsCompleted = Account::active().getCampaignChoicesMade();
        int numLevelsToShow = numLevelsCompleted;

        // Show the next available level, accounting for branching
        numLevelsToShow += (numLevelsCompleted > 0) ? campaignlevels[numLevelsCompleted].nextlevel.size() : 1;

        // Loop through the campaign levels and display each one on the map
        for (int i = 0; i < numLevelsToShow; i++) {
            XYZ midpoint = campaignlevels[i].getCenter();
            float itemsize = campaignlevels[i].getWidth();
            bool isLevelActive = (i >= numLevelsCompleted);

            // Make inactive levels appear smaller
            if (!isLevelActive) {
                itemsize /= 2;
            }

            // Draw connecting lines between levels
            if (i >= 1) {
                XYZ start = campaignlevels[i - 1].getCenter();
                addMapLine(start.x, start.y, midpoint.x - start.x, midpoint.y - start.y, 0.5, 
                        isLevelActive ? 1 : 0.5, isLevelActive ? 1 : 0.5, 0, 0);
            }

            // Add the campaign level marker (active or inactive)
            addMapMarker(NB_CAMPAIGN_MENU_ITEM + i, Mapcircletexture,
                        midpoint.x - itemsize / 2, midpoint.y - itemsize / 2, itemsize, itemsize, 
                        isLevelActive ? 1 : 0.5, 0, 0);

            // Log the next level marker
            if (isLevelActive) {
                std::cerr << "Marking active level: " << campaignlevels[i].mapname << std::endl;
            }

            // Add labels for active levels
            if (isLevelActive) {
                addMapLabel(-2, campaignlevels[i].description,
                            campaignlevels[i].getStartX() + 10,
                            campaignlevels[i].getStartY() - 4);
            }
        }
    } 
    break;
        case 6:
            addLabel(-1, "Are you sure you want to delete this user?", 10, 400);
            addButton(1, "Yes", 10, 360);
            addButton(2, "No", 10, 320);
            break;
        case 7:
            if (Account::getNbAccounts() < 8) {
                addButton(0, "New User", 10, 400);
            } else {
                addLabel(0, "No More Users", 10, 400);
            }
            addLabel(-2, "", 20, 400);
            addButton(Account::getNbAccounts() + 1, "Back", 10, 10);
            for (int i = 0; i < Account::getNbAccounts(); i++) {
                addButton(i + 1, Account::get(i).getName(), 10, 340 - 20 * (i + 1));
            }
            break;
        case 8:
            addButton(0, "Easier", 10, 400);
            addButton(1, "Difficult", 10, 360);
            addButton(2, "Insane", 10, 320);
            break;
        case 9:
            for (int i = 0; i < numchallengelevels; i++) {
                string name = "Level ";
                name += to_string(i + 1);
                if (name.size() < 17) {
                    name.append((17 - name.size()), ' ');
                }
                name += to_string(int(Account::active().getHighScore(i)));
                if (name.size() < 32) {
                    name.append((32 - name.size()), ' ');
                }
                int fasttime = (int)round(Account::active().getFastTime(i));
                name += to_string(int((fasttime - fasttime % 60) / 60));
                name += ":";
                if (fasttime % 60 < 10) {
                    name += "0";
                }
                name += to_string(fasttime % 60);

                addButton(i, name, 10, 400 - i * 25, i > Account::active().getProgress() ? 0.5 : 1, 0, 0);
            }

            addButton(-1, "             High Score      Best Time", 10, 440);
            addButton(numchallengelevels, "Back", 10, 10);
            break;
        case 10: {
            addLabel(0, campaignEndText[0], 80, 330);
            addLabel(1, campaignEndText[1], 80, 300);
            addLabel(2, campaignEndText[2], 80, 270);
            addButton(3, "Back", 10, 10);
            addLabel(4, string("Your score:         ") + to_string((int)Account::active().getCampaignScore()), 190, 200);
            addLabel(5, string("Highest score:      ") + to_string((int)Account::active().getCampaignHighScore()), 190, 180);
        } break;
        case 18:
            addButton(0, "", 70, 400);
            addButton(1, "", 10, 360);
            addButton(2, "", 40, 320);
            addButton(3, "Back", 10, 10);
            updateStereoConfigMenu();
            break;        
        case 19: // Updated case for the Mods menu
            updateModsMenu();
            break;
    }
}

void Menu::Tick()
{
    //escape key pressed
    if (Input::isKeyPressed(SDL_SCANCODE_ESCAPE) &&
        (mainmenu >= 3) && (mainmenu != 8) && !((mainmenu == 7) && entername)) {
        selected = -1;
        //finished with settings menu
        if (mainmenu == 3) {
            SaveSettings();
        }
        //effects
        if (mainmenu >= 3 && mainmenu != 8) {
            fireSound();
            flash();
        }
        //go back
        switch (mainmenu) {
            case 3:
            case 5:
                mainmenu = gameon ? 2 : 1;
                break;
            case 4:
            case 18:
                mainmenu = 3;
                break;
            case 6:
            case 7:
            case 9:
            case 10:
                mainmenu = 5;
                break;
            case 19:
                mainmenu = 1;
                break;
        }
    }

    //menu buttons
    selected = getSelected(mousecoordh * 640 / screenwidth, 480 - mousecoordv * 480 / screenheight);

    if (mainmenu == 19) {
        if (Input::MouseClicked()) {
            std::cerr << "Mouse clicked. Selected item: " << selected << std::endl;

            // Check if a mod has been clicked using the new rect button system
            if (selected >= 3000 && selected < 3000 + static_cast<int>(availableMods.size())) {
                selectedModIndex = selected - 3000;  // Update the selected mod index based on rect button ID
                drawModMenu();  // Redraw the menu to reflect the new selection
            }
        }
    }


    // some specific case where we do something even if the left mouse button is not pressed.
    if ((mainmenu == 5) && (endgame == 2)) {
        Account::active().endGame();
        endgame = 0;
    }
    if (mainmenu == 10) {
        endgame = 2;
    }
    if (mainmenu == 18 && Input::isKeyPressed(MOUSEBUTTON_RIGHT) && selected == 1) {
        stereoseparation -= 0.001;
        updateStereoConfigMenu();
    }

    static int oldmainmenu = mainmenu;

    if (Input::MouseClicked() && (selected >= 0)) { // handling of the left mouse clic in menus
        set<pair<int, int>>::iterator newscreenresolution;
        switch (mainmenu) {
            case 1:
            case 2:
                switch (selected) {
                    case 1:
                        if (gameon) { //resume
                            mainmenu = 0;
                            pause_sound(stream_menutheme);
                            resume_stream(leveltheme);
                        } else { //new game
                            fireSound(firestartsound);
                            flash();
                            mainmenu = (Account::hasActive() ? 5 : 7);
                            selected = -1;
                        }
                        break;
                    case 2: //options
                        fireSound();
                        flash();
                        mainmenu = 3;
                        if (newdetail > 2) {
                            newdetail = detail;
                        }
                        if (newdetail < 0) {
                            newdetail = detail;
                        }
                        if (newscreenwidth > 3000) {
                            newscreenwidth = screenwidth;
                        }
                        if (newscreenwidth < 0) {
                            newscreenwidth = screenwidth;
                        }
                        if (newscreenheight > 3000) {
                            newscreenheight = screenheight;
                        }
                        if (newscreenheight < 0) {
                            newscreenheight = screenheight;
                        }
                        break;
                    case 3:
                        fireSound();
                        flash();
                        if (gameon) { //restart
                            mainmenu = 0;
                            fireSound(firestartsound);
                            flash();
                            Game::RestartLevel();
                            pause_sound(stream_menutheme);
                            resume_stream(leveltheme);
                        } else { //mods
                            mainmenu = 19;
                        }
                        break;
                    case 4:
                        fireSound();
                        flash();
                        if (gameon) { //end game
                            gameon = 0;
                            mainmenu = 1;
                        } else { //quit
                            tryquit = 1;
                            pause_sound(stream_menutheme);
                        }
                        break;
                }
                break;
            case 3:
                fireSound();
                switch (selected) {
                    case 0:
                        newscreenresolution = resolutions.find(make_pair(newscreenwidth, newscreenheight));
                        /* Next one (end() + 1 is also end() so the ++ is safe even if it was not found) */
                        newscreenresolution++;
                        if (newscreenresolution == resolutions.end()) {
                            /* It was the last one (or not found), go back to the beginning */
                            newscreenresolution = resolutions.begin();
                        }
                        newscreenwidth = newscreenresolution->first;
                        newscreenheight = newscreenresolution->second;
                        break;
                    case 1:
                        newdetail++;
                        if (newdetail > 2) {
                            newdetail = 0;
                        }
                        break;
                    case 2:
                        bloodtoggle++;
                        if (bloodtoggle > 2) {
                            bloodtoggle = 0;
                        }
                        break;
                    case 4:
                        ismotionblur = !ismotionblur;
                        break;
                    case 5:
                        decalstoggle = !decalstoggle;
                        break;
                    case 6:
                        musictoggle = !musictoggle;
                        if (musictoggle) {
                            emit_stream_np(stream_menutheme);
                        } else {
                            pause_sound(leveltheme);
                            pause_sound(stream_fighttheme);
                            pause_sound(stream_menutheme);

                            for (int i = 0; i < 4; i++) {
                                oldmusicvolume[i] = 0;
                                musicvolume[i] = 0;
                            }
                        }
                        break;
                    case 7: // controls
                        flash();
                        mainmenu = 4;
                        selected = -1;
                        keyselect = -1;
                        break;
                    case 8:
                        flash();
                        SaveSettings();
                        mainmenu = gameon ? 2 : 1;
                        break;
                    case 9:
                        invertmouse = !invertmouse;
                        break;
                    case 10:
                        usermousesensitivity += .2;
                        if (usermousesensitivity > 2) {
                            usermousesensitivity = .2;
                        }
                        break;
                    case 11:
                        volume += .1f;
                        if (volume > 1.0001f) {
                            volume = 0;
                        }
                        OPENAL_SetSFXMasterVolume((int)(volume * 255));
                        break;
                    case 12:
                        flash();
                        newstereomode = stereomode;
                        mainmenu = 18;
                        keyselect = -1;
                        break;
                    case 13:
                        showdamagebar = !showdamagebar;
                        break;
                    case 14:
                        toggleFullscreen();
                        break;
                    case 15: //mod menu
                        fireSound(firestartsound);
                        flash();
                        mainmenu = 19;
                        break;
                }
                updateSettingsMenu();
                break;
            case 4:
                if (!waiting) {
                    fireSound();
                    if (selected < (devtools ? 10 : 9) && keyselect == -1) {
                        keyselect = selected;
                    }
                    if (keyselect != -1) {
                        setKeySelected();
                    }
                    if (selected == (devtools ? 10 : 9)) {
                        flash();
                        mainmenu = 3;
                    }
                }
                updateControlsMenu();
                break;
            case 5:
                fireSound();
                flash();
                if ((selected - NB_CAMPAIGN_MENU_ITEM >= Account::active().getCampaignChoicesMade())) {
                    startbonustotal = 0;

                    loading = 2;
                    loadtime = 0;
                    targetlevel = 7;

                    if (firstLoadDone) {
                        TickOnceAfter();
                    } else {
                        LoadStuff();
                    }

                    whichchoice = selected - NB_CAMPAIGN_MENU_ITEM - Account::active().getCampaignChoicesMade();

                    actuallevel = (Account::active().getCampaignChoicesMade() > 0 ? campaignlevels[Account::active().getCampaignChoicesMade() -1].nextlevel[whichchoice] : 0);
                    std::cerr << "Selected level: " << Account::active().getCampaignChoicesMade() + 1 << " (" << campaignlevels[actuallevel].mapname << ")" << std::endl;

                    visibleloading = true;
                    stillloading = 1;

                    LoadLevel(campaignlevels[actuallevel].mapname.c_str());

                    campaign = 1;
                    mainmenu = 0;
                    gameon = 1;
                    std::cerr << "Game started. Pausing menu theme." << std::endl;
                    pause_sound(stream_menutheme);
                }
                switch (selected) {
                    case 1:
                        startbonustotal = 0;

                        loading = 2;
                        loadtime = 0;
                        targetlevel = -1;
                        if (firstLoadDone) {
                            TickOnceAfter();
                        } else {
                            LoadStuff();
                        }
                        LoadLevel(-1);

                        mainmenu = 0;
                        gameon = 1;
                        pause_sound(stream_menutheme);
                        break;
                    case 2:
                        mainmenu = 9;
                        break;
                    case 3:
                        mainmenu = 6;
                        break;
                    case 4:
                        mainmenu = (gameon ? 2 : 1);
                        break;
                    case 5:
                        mainmenu = 7;
                        break;
                    case 6: {
                        // Get the list of all campaigns (both from the base game and mods)
                        std::vector<std::string> campaigns = ListCampaigns();
                        
                        // Debug: print the available campaigns
                        std::cerr << "Available campaigns: ";
                        for (const auto& campaign : campaigns) {
                            std::cerr << campaign << " ";
                        }
                        std::cerr << std::endl;

                        // Check if the current campaign is already in the list
                        std::string currentCampaign = Account::active().getCurrentCampaign();
                        std::cerr << "Current campaign: " << currentCampaign << std::endl;

                        auto it = std::find(campaigns.begin(), campaigns.end(), currentCampaign);

                        // If current campaign is not found or the list is empty, fall back to the first campaign
                        if (it == campaigns.end() || campaigns.empty()) {
                            std::cerr << "Current campaign not found in the list, falling back to the first campaign." << std::endl;
                            if (!campaigns.empty()) {
                                Account::active().setCurrentCampaign(campaigns.front());
                                std::cerr << "New campaign: " << campaigns.front() << std::endl;
                            }
                        } else {
                            // Move to the next campaign in the list, or cycle back to the first if at the end
                            std::cerr << "Cycling to the next campaign..." << std::endl;
                            ++it;
                            if (it == campaigns.end()) {
                                std::cerr << "Reached the end of the campaign list, cycling back to the first campaign." << std::endl;
                                it = campaigns.begin();
                            }
                            Account::active().setCurrentCampaign(*it);
                            std::cerr << "New campaign: " << *it << std::endl;
                        }

                        // Reload the campaign and update the menu
                        Load();
                        std::cerr << "Campaign loaded and menu updated." << std::endl;
                    }
                    break;
                }
                break;
            case 6:
                fireSound();
                if (selected == 1) {
                    flash();
                    Account::destroyActive();
                    mainmenu = 7;
                } else if (selected == 2) {
                    flash();
                    mainmenu = 5;
                }
                break;
            case 7:
                fireSound();
                if (selected == 0 && Account::getNbAccounts() < 8) {
                    entername = 1;
                } else if (selected < Account::getNbAccounts() + 1) {
                    flash();
                    mainmenu = 5;
                    Account::setActive(selected - 1);
                } else if (selected == Account::getNbAccounts() + 1) {
                    flash();
                    if (Account::hasActive()) {
                        mainmenu = 5;
                    } else {
                        mainmenu = 1;
                    }
                    newusername.clear();
                    newuserselected = 0;
                    entername = 0;
                }
                break;
            case 8:
                fireSound();
                flash();
                if (selected <= 2) {
                    Account::active().setDifficulty(selected);
                }
                mainmenu = 5;
                break;
            case 9:
                if (selected < numchallengelevels && selected <= Account::active().getProgress()) {
                    fireSound();
                    flash();

                    startbonustotal = 0;

                    loading = 2;
                    loadtime = 0;
                    targetlevel = selected;
                    if (firstLoadDone) {
                        TickOnceAfter();
                    } else {
                        LoadStuff();
                    }
                    LoadLevel(selected);           
                    campaign = 0;

                    mainmenu = 0;
                    gameon = 1;
                    pause_sound(stream_menutheme);
                }
                if (selected == numchallengelevels) {
                    fireSound();
                    flash();
                    mainmenu = 5;
                }
                break;
            case 10:
                if (selected == 3) {
                    fireSound();
                    flash();
                    mainmenu = 5;
                }
                break;
            case 18:
                if (selected == 1) {
                    stereoseparation += 0.001;
                } else {
                    fireSound();
                    if (selected == 0) {
                        newstereomode = (StereoMode)(newstereomode + 1);
                        while (!CanInitStereo(newstereomode)) {
                            printf("Failed to initialize mode %s (%i)\n", StereoModeName(newstereomode).c_str(), newstereomode);
                            newstereomode = (StereoMode)(newstereomode + 1);
                            if (newstereomode >= stereoCount) {
                                newstereomode = stereoNone;
                            }
                        }
                    } else if (selected == 2) {
                        stereoreverse = !stereoreverse;
                    } else if (selected == 3) {
                        flash();
                        mainmenu = 3;

                        stereomode = newstereomode;
                        InitStereo(stereomode);
                    }
                }
                updateStereoConfigMenu();
                break;
            case 19:
                fireSound(); // Play click sound
                if (selected == 1) { // "Back" button
                    mainmenu = 1; // Return to main menu (or previous menu)
                    Load(); // Reload the menu
                } else if (selected == 2) { // "Apply" button
                    applyModChanges(); // Apply the selected mods
                } else if (selected >= 1000 && selected < 2000 + static_cast<int>(enabledMods.size())) {
                    handleArrowButtonPress(selected); // Handle arrow button press if a mod button is clicked
                }
                break;
        }
    }

    OPENAL_SetFrequency(channels[stream_menutheme]);

    if (entername) {
        inputText(newusername, &newuserselected);
        if (!waiting) {                 // the input as finished
            if (!newusername.empty()) { // with enter
                Account::add(string(newusername));

                mainmenu = 8;

                flash();

                fireSound(firestartsound);

                newusername.clear();

                newuserselected = 0;
            }
            entername = 0;
            Load();
        }

        newuserblinkdelay -= multiplier;
        if (newuserblinkdelay <= 0) {
            newuserblinkdelay = .3;
            newuserblink = !newuserblink;
        }
    }

    if (entername) {
        setText(0, newusername, 20, 400, -1, -1);
        setText(-2, newuserblink ? "_" : "", 20 + newuserselected * 10, 400, -1, -1);
    }

    if (oldmainmenu != mainmenu) {
        Load();
    }
    oldmainmenu = mainmenu;
}

int setKeySelected_thread(void*)
{
    using namespace Game;
    int scancode = -1;
    SDL_Event evenement;
    while (scancode == -1) {
        SDL_WaitEvent(&evenement);
        switch (evenement.type) {
            case SDL_KEYDOWN:
                scancode = evenement.key.keysym.scancode;
                break;
            case SDL_MOUSEBUTTONDOWN:
                scancode = SDL_NUM_SCANCODES + evenement.button.button;
                break;
            default:
                break;
        }
    }
    if (scancode != SDL_SCANCODE_ESCAPE) {
        fireSound();
        switch (keyselect) {
            case 0:
                forwardkey = scancode;
                break;
            case 1:
                backkey = scancode;
                break;
            case 2:
                leftkey = scancode;
                break;
            case 3:
                rightkey = scancode;
                break;
            case 4:
                crouchkey = scancode;
                break;
            case 5:
                jumpkey = scancode;
                break;
            case 6:
                drawkey = scancode;
                break;
            case 7:
                throwkey = scancode;
                break;
            case 8:
                attackkey = scancode;
                break;
            case 9:
                consolekey = scancode;
                break;
            default:
                break;
        }
    }
    keyselect = -1;
    waiting = false;
    Menu::Load();
    return 0;
}

void Menu::setKeySelected()
{
    waiting = true;
    printf("launch thread\n");
    SDL_Thread* thread = SDL_CreateThread(setKeySelected_thread, NULL, NULL);
    if (thread == NULL) {
        fprintf(stderr, "Unable to create thread: %s\n", SDL_GetError());
        waiting = false;
        return;
    }
}

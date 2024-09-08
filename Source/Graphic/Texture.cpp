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

#include "Graphic/Texture.hpp"

#include "Utils/Folders.hpp"
#include "Utils/ImageIO.hpp"
#include <filesystem>

using namespace std;

extern bool trilinear;

void TextureRes::load() {
    ImageRec texture;
    std::string resourceTexturePath;

    // Correct malformed paths by removing redundant "Data/:"
    size_t found = filename.find("Data/:");
    if (found != std::string::npos) {
        filename.erase(found, 6); // Remove the incorrect "Data/:"
    }

    // Pass the cleaned-up path directly to getResourcePath for mod or base game search
    resourceTexturePath = Folders::getResourcePath(filename);

    // Check if the file was found
    if (resourceTexturePath.empty()) {
        std::cerr << "Texture file not found: " << filename << std::endl;
        return;
    }

    // Update the filename to the resolved resource path
    filename = resourceTexturePath;
    std::cout << "Loading Texture: " << filename << std::endl;

    // Clear any previous OpenGL errors
    while (glGetError() != GL_NO_ERROR);

    // Now, try loading the image
    if (!load_image(filename.c_str(), texture)) {
        std::cerr << "Texture " << filename << " loading failed during image loading" << std::endl;
        return;
    }

    // Proceed with binding and setting up the texture as usual
    skinsize = texture.sizeX;
    GLuint type = GL_RGBA;

    // Handle different texture bit depths
    if (texture.bpp == 24) {
        type = GL_RGB;
    } else if (texture.bpp == 32) {
        type = GL_RGBA;
    } else {
        std::cerr << "Unsupported texture format: " << texture.bpp << " bits per pixel" << std::endl;
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glDeleteTextures(1, &id);
    glGenTextures(1, &id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (hasMipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (trilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST));
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    // Set texture data for skin or standard texture
    if (isSkin) {
        free(data);
        const int nb = texture.sizeY * texture.sizeX * (texture.bpp / 8);
        data = (GLubyte*)malloc(nb * sizeof(GLubyte));
        datalen = 0;
        for (int i = 0; i < nb; i++) {
            if ((i + 1) % 4 || type == GL_RGB) {
                data[datalen++] = texture.data[i];
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, type, texture.sizeX, texture.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, type, texture.sizeX, texture.sizeY, 0, type, GL_UNSIGNED_BYTE, texture.data);
    }

    // Check for OpenGL errors
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error during texture setup: " << err << std::endl;
    }
}

void TextureRes::bind()
{
    glBindTexture(GL_TEXTURE_2D, id);
}

TextureRes::TextureRes(const string& _filename, bool _hasMipmap)
    : id(0)
    , filename(_filename)
    , hasMipmap(_hasMipmap)
    , isSkin(false)
    , skinsize(0)
    , data(NULL)
    , datalen(0)
{
    load();
}

TextureRes::TextureRes(const string& _filename, bool _hasMipmap, GLubyte* array, int* skinsizep)
    : id(0)
    , filename(_filename)
    , hasMipmap(_hasMipmap)
    , isSkin(true)
    , skinsize(0)
    , data(NULL)
    , datalen(0)
{
    load();
    *skinsizep = skinsize;
    for (int i = 0; i < datalen; i++) {
        array[i] = data[i];
    }
}

TextureRes::~TextureRes()
{
    free(data);
    glDeleteTextures(1, &id);
}

void Texture::load(const string& filename, bool hasMipmap)
{
    tex.reset(new TextureRes(Folders::getResourcePath(filename), hasMipmap));
}

void Texture::load(const string& filename, bool hasMipmap, GLubyte* array, int* skinsizep)
{
    tex.reset(new TextureRes(Folders::getResourcePath(filename), hasMipmap, array, skinsizep));
}

void Texture::bind()
{
    if (tex) {
        tex->bind();
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
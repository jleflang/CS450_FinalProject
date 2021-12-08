#pragma once
#ifndef LOAD_MTL_FILE_H
#define LOAD_MTL_FILE_H

#include "common.h"

#include "stb_image.h"

struct Texture
{
    int textW, textH, nrComp;
    unsigned char* img;
    unsigned short* img16;
};

class Material
{
private:

    enum texture_typ
    {
        diffuse,
        rough,
        reflect,
        normal
    };

    void ReadImageTexture(char*, int, int);

public:
    float Ns, Ni, d;
    float Ka[3], Kd[3], Ks[3], Ke[3];
    int illum;

    std::vector<Texture>* textures;

    Texture* LoadNorm();
    Texture* LoadKd();
    Texture* LoadNs();
    Texture* LoadRefl();

    void ReadNorm(char*);
    void ReadKd(char*);
    void ReadNs(char*);
    void ReadRefl(char*);

    Material()
    {
        Ns = Ni = d = 0.f;
        *Ka = { };
        *Kd = { };
        *Ks = { };
        *Ke = { };
        illum = 0;
        textures = new std::vector<Texture>;
        textures->reserve(4);

    };

    ~Material()
    {
        for (int i = 0; i < 4; i++)
        {
            stbi_image_free(textures[i].data()->img);
        }

        delete textures;
    };
};

struct mat
{
    std::string n;
    Material* m;
};

class MaterialSet
{
private:
    

public:

    std::vector <struct mat> obj_mats;

    int LoadMtlFile(char*);
    void Reset();

    MaterialSet()
    {
        Reset();
    };

    ~MaterialSet()
    {
        
    };
};

#endif // !LOAD_MTL_FILE_H


#include "loadmtlfile.h"

#define OBJDELIMS		" \t"
#define WINEOL			"\n"

char* ReadRestOfLine(FILE*);

void Material::ReadImageTexture(char* img, int typ, int nchan)
{
    // Find the beginning of the heap of textures and set the offset
    std::vector<Texture>::iterator cursor = textures->begin();
    cursor += typ;

	char* front = strtok(img, (char*)"\\\\");
	char* filename = strtok(NULL, WINEOL);

	std::string texture_dir("assets\\");
	texture_dir.append(front);
	texture_dir.append(filename);

    // Create and load the texture image
    Texture cur_texture = {};
	// Set STBI to flip images for texture loading
	stbi_set_flip_vertically_on_load(1);
	if (typ != texture_typ::normal)
	{
		unsigned char* tmp = stbi_load((char*)texture_dir.c_str(), &cur_texture.textH, &cur_texture.textW, &cur_texture.nrComp, nchan);
		if (tmp != NULL)
		{
#ifdef _DEBUG
			fprintf(stderr, "Image Texture %s loaded.\n", texture_dir.c_str());
#endif // _DEBUG

			cur_texture.img = tmp;
		}
	}
	else
	{
		unsigned short* tmp = stbi_load_16((char*)texture_dir.c_str(), &cur_texture.textH, &cur_texture.textW, &cur_texture.nrComp, nchan);
		if (tmp != NULL)
		{
#ifdef _DEBUG
			fprintf(stderr, "Image Texture %s loaded.\n", texture_dir.c_str());
#endif // _DEBUG

			cur_texture.img16 = tmp;
		}
	}

    textures->insert(cursor, cur_texture);
}

Texture* Material::LoadNorm()
{
    return &textures->at(texture_typ::normal);
}

Texture* Material::LoadKd()
{
    return &textures->at(texture_typ::diffuse);
}

Texture* Material::LoadNs()
{
    return &textures->at(texture_typ::rough);
}

Texture* Material::LoadRefl()
{
    return &textures->at(texture_typ::reflect);
}

void Material::ReadNorm(char *file)
{
    ReadImageTexture(file, texture_typ::normal, 3);
}

void Material::ReadKd(char *file)
{
    ReadImageTexture(file, texture_typ::diffuse, 3);
}

void Material::ReadNs(char *file)
{
    ReadImageTexture(file, texture_typ::rough, 1);
}

void Material::ReadRefl(char *file)
{
    ReadImageTexture(file, texture_typ::reflect, 1);
}

int MaterialSet::LoadMtlFile(char* file)
{

	char* cmd;		// the command string
	char* str;		// argument string

	Material* cur_material = NULL;
	struct mat cur_mat;

	bool isNewMat = true;

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
        return 1;

    for (; ; )
    {
		char* line = ReadRestOfLine(fp);
		if (line == NULL)
			break;

		if (isNewMat)
		{
			cur_material = new Material();

			cur_mat = { "", cur_material};
		}

		// skip this line if it is a comment:

		if (line[0] == '#')
			continue;

		// get the command string:

		cmd = strtok(line, OBJDELIMS);

		// A new material is starting or its the a random newline
		if (cmd == NULL)
		{
			if (!cur_mat.n.empty())
			{
				obj_mats.push_back(cur_mat);
				isNewMat = true;
			}

			continue;
		}

		if (strcmp(cmd, "newmtl") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_mat.n.append(str);

			isNewMat = false;

			continue;
		}

		if (strcmp(cmd, "Ns") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->Ns = atof(str);

			continue;
		}

		if (strcmp(cmd, "Ni") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->Ni = atof(str);

			continue;
		}

		if (strcmp(cmd, "d") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->d = atof(str);

			continue;
		}

		if (strcmp(cmd, "illum") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->illum = atoi(str);

			continue;
		}

		if (strcmp(cmd, "map_Kd") == 0)
		{
			str = strtok(NULL, WINEOL);
			cur_material->ReadKd(str);

			continue;
		}

		if (strcmp(cmd, "map_Ns") == 0)
		{
			str = strtok(NULL, WINEOL);
			cur_material->ReadNs(str);

			continue;
		}

		if (strcmp(cmd, "refl") == 0)
		{
			str = strtok(NULL, WINEOL);
			cur_material->ReadRefl(str);

			continue;
		}

		if (strcmp(cmd, "norm") == 0)
		{
			str = strtok(NULL, WINEOL);
			cur_material->ReadNorm(str);

			continue;
		}

		if (strcmp(cmd, "Ka") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->Ka[0] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Ka[1] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Ka[2] = atof(str);

			continue;

		}

		if (strcmp(cmd, "Kd") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->Kd[0] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Kd[1] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Kd[2] = atof(str);

			continue;

		}

		if (strcmp(cmd, "Ks") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->Ks[0] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Ks[1] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Ks[2] = atof(str);

			continue;

		}

		if (strcmp(cmd, "Ke") == 0)
		{
			str = strtok(NULL, OBJDELIMS);
			cur_material->Ke[0] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Ke[1] = atof(str);

			str = strtok(NULL, OBJDELIMS);
			cur_material->Ke[2] = atof(str);

			continue;

		}

    }

	fclose(fp);

    return 0;
}

void MaterialSet::Reset()
{

	obj_mats.clear();

}


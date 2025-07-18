#include "includes/loadobjfile.h"

// delimiters for parsing the obj file:

#define OBJDELIMS		" \t"


struct Vertex
{
	float x, y, z;
};


struct Normal
{
	float nx, ny, nz;
};


struct TextureCoord
{
	float s, t, p;
};


struct face
{
	int v, n, t;
};



void	Cross(float[3], float[3], float[3]);
char* ReadRestOfLine(FILE*);
void	ReadObjVTN(char*, int*, int*, int*);
float	Unit(float[3]);
float	Unit(float[3], float[3]);


int
LoadObjFile(char* name, std::vector<VertexBufferObject*> *object, MaterialSet *matlib)
{
	char* cmd;		// the command string
	char* str;		// argument string

	VertexBufferObject *cur_object_g = NULL;

	std::vector <struct Vertex> Vertices(10000);
	std::vector <struct Normal> Normals(10000);
	std::vector <struct TextureCoord> TextureCoords(10000);

	Vertices.clear();
	Normals.clear();
	TextureCoords.clear();

	struct Vertex sv = { };
	struct Normal sn = { };
	struct TextureCoord st = { };


	// open the input file:

	FILE* fp = NULL;
	if (fopen_s(&fp, name, "r") != 0)
	{
		fprintf(stderr, "Cannot open .obj file '%s'\n", name);
		return 1;
	}


	float xmin = 1.e+37f;
	float ymin = 1.e+37f;
	float zmin = 1.e+37f;
	float xmax = -xmin;
	float ymax = -ymin;
	float zmax = -zmin;

	//glBegin(GL_TRIANGLES);

	for (; ; )
	{
		char* line = ReadRestOfLine(fp);
		if (line == NULL)
			break;


		// skip this line if it is a comment:

		if (line[0] == '#')
			continue;


		// skip this line if it is something we don't feel like handling today:

		if (line[0] == 'g')
			continue;

		if (line[0] == 's')
			continue;

		// get the command string:
		char* tokptr = NULL;
		cmd = strtok_s(line, OBJDELIMS, &tokptr);


		// skip this line if it is empty:

		if (cmd == NULL)
			continue;


		if (strcmp(cmd, "mtllib") == 0)
		{
			std::string dir ("assets\\");
			str = strtok_s(NULL, (char*)"\n", &tokptr);
			dir.append(str);

			if (matlib->LoadMtlFile((char *)dir.c_str()) == 0)
			{
#ifdef _DEBUG
				fprintf(stderr, "Texture file loaded: %s\n", (char*)dir.c_str());
#endif
			}
			else
			{
#ifdef _DEBUG
				fprintf(stderr, "Texture file failed to load: %s\n", (char*)dir.c_str());
#endif
			}

			continue;

		}


		// Push and create a new VBO
		if (strcmp(cmd, "o") == 0)
		{
			if (cur_object_g != NULL)
			{
				object->push_back(cur_object_g);
				cur_object_g = NULL;
			}

			cur_object_g = new VertexBufferObject();

			cur_object_g->CollapseCommonVertices(false);
			cur_object_g->glBegin(GL_TRIANGLES);

			cur_object_g->SetVerbose(false);

		}

		if (strcmp(cmd, "usemtl") == 0)
		{
			str = strtok_s(NULL, (char*)"\n", &tokptr);

			cur_object_g->SetMaterial(str);

			continue;

		}


		if (strcmp(cmd, "v") == 0)
		{
			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			sv.x = atof(str);

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			sv.y = atof(str);

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			sv.z = atof(str);

			Vertices.push_back(sv);

			if (sv.x < xmin)	xmin = sv.x;
			if (sv.x > xmax)	xmax = sv.x;
			if (sv.y < ymin)	ymin = sv.y;
			if (sv.y > ymax)	ymax = sv.y;
			if (sv.z < zmin)	zmin = sv.z;
			if (sv.z > zmax)	zmax = sv.z;

			continue;
		}


		if (strcmp(cmd, "vn") == 0)
		{
			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			sn.nx = atof(str);

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			sn.ny = atof(str);

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			sn.nz = atof(str);

			Normals.push_back(sn);

			continue;
		}


		if (strcmp(cmd, "vt") == 0)
		{
			st.s = st.t = st.p = 0.;

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			st.s = atof(str);

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			if (str != NULL)
				st.t = atof(str);

			str = strtok_s(NULL, OBJDELIMS, &tokptr);
			if (str != NULL)
				st.p = atof(str);

			TextureCoords.push_back(st);

			continue;
		}


		if (strcmp(cmd, "f") == 0)
		{
			struct face vertices[10] = { };

			int sizev = (int)Vertices.size();
			int sizen = (int)Normals.size();
			int sizet = (int)TextureCoords.size();

			int numVertices = 0;
			bool valid = true;
			int vtx = 0;
			char* str;
			while ((str = strtok_s(NULL, OBJDELIMS, &tokptr)) != NULL)
			{
				int v, n, t;
				ReadObjVTN(str, &v, &t, &n);

				// if v, n, or t are negative, they are wrt the end of their respective list:

				if (v < 0)
					v += (sizev + 1);

				if (n < 0)
					n += (sizen + 1);

				if (t < 0)
					t += (sizet + 1);


				// be sure we are not out-of-bounds (<vector> will abort):

				if (t > sizet)
				{
					if (t != 0)
						fprintf(stderr, "Read texture coord %d, but only have %d so far\n", t, sizet);
					t = 0;
				}

				if (n > sizen)
				{
					if (n != 0)
						fprintf(stderr, "Read normal %d, but only have %d so far\n", n, sizen);
					n = 0;
				}

				if (v > sizev)
				{
					if (v != 0)
						fprintf(stderr, "Read vertex coord %d, but only have %d so far\n", v, sizev);
					v = 0;
					valid = false;
				}

				vertices[vtx].v = v;
				vertices[vtx].n = n;
				vertices[vtx].t = t;
				vtx++;

				if (vtx >= 10)
					break;

				numVertices++;
			}


			// if vertices are invalid, don't draw anything this time:

			if (!valid)
				continue;

			if (numVertices < 3)
				continue;


			// list the vertices:

			int numTriangles = numVertices - 2;

			for (int it = 0; it < numTriangles; it++)
			{
				int vv[3] = { };
				vv[0] = 0;
				vv[1] = it + 1;
				vv[2] = it + 2;


				// Calculate Tangent/Binormals
				struct Vertex tangent = { };
				struct Vertex bitangent = { };

				if (vertices[vv[0]].t != 0)
				{
					struct Vertex* v0 = &Vertices[vertices[vv[0]].v - 1];
					struct Vertex* v1 = &Vertices[vertices[vv[1]].v - 1];
					struct Vertex* v2 = &Vertices[vertices[vv[2]].v - 1];

					struct TextureCoord* tp0 = &TextureCoords[vertices[vv[0]].t - 1];
					struct TextureCoord* tp1 = &TextureCoords[vertices[vv[1]].t - 1];
					struct TextureCoord* tp2 = &TextureCoords[vertices[vv[2]].t - 1];

					struct Vertex deltaPos1 = { v1->x - v0->x, v1->y - v0->y, v1->z - v0->z };
					struct Vertex deltaPos2 = { v2->x - v0->x, v2->y - v0->y, v2->z - v0->z };

					struct TextureCoord uv1 = { tp1->s - tp0->s, tp1->t - tp0->t };
					struct TextureCoord uv2 = { tp2->s - tp0->s, tp2->t - tp0->t };

					float r = 1.f / (uv1.s * uv2.t - uv1.t * uv2.s);

					tangent = { (deltaPos1.x * uv2.t - deltaPos2.x * uv1.t) * r,
								(deltaPos1.y * uv2.t - deltaPos2.y * uv1.t) * r,
								(deltaPos1.z * uv2.t - deltaPos2.z * uv1.t) * r
							  };

					bitangent = { (deltaPos1.x * uv2.s - deltaPos2.x * uv1.s) * r,
								  (deltaPos1.y * uv2.s - deltaPos2.y * uv1.s) * r,
								  (deltaPos1.z * uv2.s - deltaPos2.z * uv1.s) * r
							    };

				}

				// get the planar normal, in case vertex normals are not defined:

				/*struct Vertex* v0 = &Vertices[vertices[vv[0]].v - 1];
				struct Vertex* v1 = &Vertices[vertices[vv[1]].v - 1];
				struct Vertex* v2 = &Vertices[vertices[vv[2]].v - 1];

				float v01[3], v02[3], norm[3];
				v01[0] = v1->x - v0->x;
				v01[1] = v1->y - v0->y;
				v01[2] = v1->z - v0->z;
				v02[0] = v2->x - v0->x;
				v02[1] = v2->y - v0->y;
				v02[2] = v2->z - v0->z;
				Cross(v01, v02, norm);
				Unit(norm, norm);
				glNormal3fv(norm);*/

				for (int vtx = 0; vtx < 3; vtx++)
				{

					if (vertices[vv[vtx]].n != 0)
					{
						struct Normal* np = &Normals[vertices[vv[vtx]].n - 1];
						cur_object_g->glNormal3f(np->nx, np->ny, np->nz);
					}

					if (vertices[vv[vtx]].t != 0)
					{
						struct TextureCoord* tp = &TextureCoords[vertices[vv[vtx]].t - 1];
						cur_object_g->glTexCoord2f(tp->s, tp->t);
					}

					struct Vertex* vp = &Vertices[vertices[vv[vtx]].v - 1];
					cur_object_g->AddTangent(tangent.x, tangent.y, tangent.z);
					cur_object_g->AddBitangent(bitangent.x, bitangent.y, bitangent.z);
					cur_object_g->glVertex3f(vp->x, vp->y, vp->z);
				}
			}
			continue;
		}


		if (strcmp(cmd, "s") == 0)
		{
			continue;
		}

	}

	//glEnd();
	fclose(fp);

#ifdef _DEBUG

	fprintf(stderr, "Obj file range: [%8.3f,%8.3f,%8.3f] -> [%8.3f,%8.3f,%8.3f]\n",
		xmin, ymin, zmin, xmax, ymax, zmax);
	fprintf(stderr, "Obj file center = (%8.3f,%8.3f,%8.3f)\n",
		(xmin + xmax) / 2., (ymin + ymax) / 2., (zmin + zmax) / 2.);
	fprintf(stderr, "Obj file  span = (%8.3f,%8.3f,%8.3f)\n",
		xmax - xmin, ymax - ymin, zmax - zmin);

#endif // DEBUG

	return 0;
}



void
Cross(float v1[3], float v2[3], float vout[3])
{
	float tmp[3] = { };

	tmp[0] = v1[1] * v2[2] - v2[1] * v1[2];
	tmp[1] = v2[0] * v1[2] - v1[0] * v2[2];
	tmp[2] = v1[0] * v2[1] - v2[0] * v1[1];

	vout[0] = tmp[0];
	vout[1] = tmp[1];
	vout[2] = tmp[2];
}



float
Unit(float v[3])
{
	float dist;

	dist = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

	if (dist > 0.0)
	{
		dist = sqrt(dist);
		v[0] /= dist;
		v[1] /= dist;
		v[2] /= dist;
	}

	return dist;
}



float
Unit(float vin[3], float vout[3])
{
	float dist;

	dist = vin[0] * vin[0] + vin[1] * vin[1] + vin[2] * vin[2];

	if (dist > 0.0)
	{
		dist = sqrt(dist);
		vout[0] = vin[0] / dist;
		vout[1] = vin[1] / dist;
		vout[2] = vin[2] / dist;
	}
	else
	{
		vout[0] = vin[0];
		vout[1] = vin[1];
		vout[2] = vin[2];
	}

	return dist;
}


char*
ReadRestOfLine(FILE* fp)
{
	static char* line;
	std::vector<char> tmp(1000);
	tmp.clear();

	for (; ; )
	{
		int c = getc(fp);

		if (c == EOF && tmp.size() == 0)
		{
			return NULL;
		}

		if (c == EOF || c == '\n')
		{
			delete[] line;
			line = new char[tmp.size() + 1];
			for (int i = 0; i < (int)tmp.size(); i++)
			{
				line[i] = tmp[i];
			}
			line[tmp.size()] = '\0';	// terminating null
			return line;
		}
		else
		{
			tmp.push_back(c);
		}
	}

	return (char*)"";
}


void
ReadObjVTN(char* str, int* v, int* t, int* n)
{
	// can be one of v, v//n, v/t, v/t/n:

	if (strstr(str, "//"))				// v//n
	{
		*t = 0;
		sscanf_s(str, "%d//%d", v, n);
		return;
	}
	else if (sscanf_s(str, "%d/%d/%d", v, t, n) == 3)	// v/t/n
	{
		return;
	}
	else
	{
		*n = 0;
		if (sscanf_s(str, "%d/%d", v, t) == 2)		// v/t
		{
			return;
		}
		else						// v
		{
			*n = *t = 0;
			sscanf_s(str, "%d", v);
		}
	}
}

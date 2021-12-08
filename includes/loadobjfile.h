#pragma once
#ifndef _LOAD_OBJ
#define _LOAD_OBJ

#include "common.h"

#define GLEW_STATIC
#include "glew.h"
#include <GL/gl.h>

#include "vertexbufferobject.h"
#include "loadmtlfile.h"

int LoadObjFile(char*, std::vector<VertexBufferObject*>*, MaterialSet*);
void	Cross(float[3], float[3], float[3]);
float	Unit(float[3]);
float	Unit(float[3], float[3]);

#endif
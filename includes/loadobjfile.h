#pragma once
#ifndef _LOAD_OBJ
#define _LOAD_OBJ

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <ctype.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#define GLEW_STATIC
#include "glew.h"
#include <GL/gl.h>

#include <vector>


#include "vertexbufferobject.h"

int LoadObjFile(char*, VertexBufferObject*);
void	Cross(float[3], float[3], float[3]);
float	Unit(float[3]);
float	Unit(float[3], float[3]);

#endif
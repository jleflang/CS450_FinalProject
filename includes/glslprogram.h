#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include <ctype.h>
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#define GLEW_STATIC
#include "includes/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "includes/glut.h"
#include "glm/glm/glm.hpp"
#include <map>
#include <stdarg.h>

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER	0x91B9
#endif


inline int GetOSU(int flag)
{
	int i;
	glGetIntegerv(flag, &i);
	return i;
}


void	CheckGlErrors(const char*);



class GLSLProgram
{
private:
	std::map<char*, int>	AttributeLocs;
	char* Cfile;
	unsigned int		Cshader;
	char* Ffile;
	unsigned int		Fshader;
	char* Gfile;
	GLuint			Gshader;
	bool			IncludeGstap;
	GLenum			InputTopology;
	GLenum			OutputTopology;
	GLuint			Program;
	char* TCfile;
	GLuint			TCshader;
	char* TEfile;
	GLuint			TEshader;
	std::map<char*, int>	UniformLocs;
	bool			Valid;
	char* Vfile;
	GLuint			Vshader;
	bool			Verbose;

	static int		CurrentProgram;

	void	AttachShader(GLuint);
	bool	CanDoBinaryFiles;
	bool	CanDoComputeShaders;
	bool	CanDoFragmentShaders;
	bool	CanDoGeometryShaders;
	bool	CanDoTessControlShaders;
	bool	CanDoTessEvaluationShaders;
	bool	CanDoVertexShaders;
	int	CompileShader(GLuint);
	bool	CreateHelper(char*, ...);
	int	GetAttributeLocation(char*);
	int	GetUniformLocation(char*);


public:
	GLSLProgram();

	bool	Create(char*, char* = NULL, char* = NULL, char* = NULL, char* = NULL, char* = NULL);
	void	DispatchCompute(GLuint, GLuint = 1, GLuint = 1);
	bool	IsExtensionSupported(const char*);
	bool	IsNotValid();
	bool	IsValid();
	void	LoadBinaryFile(char*);
	void	LoadProgramBinary(const char*, GLenum);
	void	SaveBinaryFile(char*);
	void	SaveProgramBinary(const char*, GLenum*);
	void	SetAttributeVariable(char*, int);
	void	SetAttributeVariable(char*, float);
	void	SetAttributeVariable(char*, float, float, float);
	void	SetAttributeVariable(char*, float[3]);
#ifdef VEC3_H
	void	SetAttributeVariable(char*, Vec3&);
#endif
#ifdef VERTEX_ARRAY_H
	void	SetAttributeVariable(char*, VertexArray&, GLenum);
#endif
#ifdef VERTEX_BUFFER_OBJECT_H
	void	SetAttributeVariable(char*, VertexBufferObject&, GLenum);
#endif
	void	SetGstap(bool);
	void	SetInputTopology(GLenum);
	void	SetOutputTopology(GLenum);
	void	SetUniformVariable(char*, int);
	void	SetUniformVariable(char*, float);
	void	SetUniformVariable(char*, float, float, float);
	void	SetUniformVariable(char*, float[3]);
	void	SetUniformVariable(char*, glm::mat4 &);
	void	SetUniformVariable(char*, glm::mat3 &);
	void	SetUniformVariable(char*, glm::vec3 &);

	void	SetVerbose(bool);
	void	Use();
	void	Use(GLuint);
	void	UseFixedFunction();
	void	UnUse();
};

#endif		// #ifndef GLSLPROGRAM_H

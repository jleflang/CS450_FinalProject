#ifndef VERTEX_BUFFER_OBJECT_H
#define VERTEX_BUFFER_OBJECT_H

#include "common.h"

#define GLEW_STATIC
#include "glew.h"
#include "glut.h"
#include <GL/gl.h>

#define BUFFER_OFFSET(bytes)	( (GLubyte *)NULL + (bytes) )
#define ELEMENT_OFFSET(a1,a2)	(  BUFFER_OFFSET( (char *)(a2) - (char *)(a1) )  )

bool	IsExtensionSupported( const char * );


struct Point
{
	float x, y, z;
	float nx, ny, nz;
	//float r, g, b;
	float s, t;
	float u, v, h;
	float ui, vi, hi;
};


class Key
{
    public:
		float x, y, z;

		Key( float _x, float _y, float _z )
		{
			x = _x;
			y = _y;
			z = _z;
		};

		Key( float _v[3] )
		{
			x = _v[0];
			y = _v[1];
			z = _v[2];
		};

		Key( const Key& k )
		{
			x = k.x;
			y = k.y;
			z = k.z;
		};
};

typedef std::map< Key, int >	PMap;



class VertexBufferObject
{
    private:
	bool				hasVertices, hasNormals, hasColors, hasTexCoords, hasTangents, hasBitangents;
	float				c_r, c_g, c_b;
	float				c_nx, c_ny, c_nz;
	float				c_s, c_t;
	float				c_u, c_v, c_w;
	float				c_uu, c_uv, c_uw;
	std::string			material;

	GLenum				topology;
	bool				verbose;
	bool				collapseCommonVertices;
	bool				isFirstDraw;
	bool				glBeginWasCalled;
	bool				drawWasCalled;
	bool				restartFound;

	std::vector <struct Point>	PointVec;
	PMap				PointMap;
	std::vector <GLuint>		ElementVec;
	struct Point *			parray;
	GLuint *			earray;
	GLuint				pbuffer;
	GLuint				ebuffer;
	GLuint				abuffer;

	const static GLuint RESTART_INDEX = ~0;	// 0xffffffff
	const static int TWO_VALUES   = 2;
	const static int THREE_VALUES = 3;

	GLuint AddVertex( GLfloat, GLfloat, GLfloat );
	void Reset( );

    public:
	void CollapseCommonVertices( bool );
	void Draw( );
	std::string GetMaterial();
	void SetMaterial(char*);
	void glBegin( GLenum );
	void glColor3f( GLfloat, GLfloat, GLfloat );
	void glColor3fv( GLfloat * );
	void glEnd( );
	void glNormal3f( GLfloat, GLfloat, GLfloat );
	void glNormal3fv( GLfloat * );
	void glTexCoord2f( GLfloat, GLfloat );
	void glTexCoord2fv( GLfloat * );
	void glVertex3f( GLfloat, GLfloat, GLfloat );
	void glVertex3fv( GLfloat * );
	void AddTangent(GLfloat, GLfloat, GLfloat);
	void AddBitangent(GLfloat, GLfloat, GLfloat);
	void Print( char * = (char *)"", FILE * = stderr );
	void RestartPrimitive( );
	void SetVerbose( bool );

	VertexBufferObject( )
	{
		verbose = false;
		parray = NULL;
		earray = NULL;
		pbuffer = 0;
		ebuffer = 0;
		Reset( );
		collapseCommonVertices = false;
		restartFound = false;
		glBeginWasCalled = false;
	};

	~VertexBufferObject( )
	{
		// really should be sure all dynamic arrays are deleted and all buffers are destroyed:
		// (it's possible to cause memory leaks in both cpu and gpu memory)

		Reset( );
	};
};
#endif

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include  "bcm_host.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

class Debayer
{
	uint32_t width, height;
	EGLNativeWindowType  hWnd;
	EGLDisplay eglDisplay;
	EGLContext eglContext;
	EGLSurface eglSurface;

	// Handle to a program object
	GLuint programObject;

	// Attribute locations
	GLint  positionLoc;
	GLint  texCoordLoc;

	// Sampler location
	GLint samplerLoc;

	// Texture handle
	GLuint textureId;

	// input to shader
	GLint firstRed;
	GLint sourceSize;

	GLboolean esCreateWindow();
	EGLBoolean CreateEGLContext();
	void initShader();
	GLuint CreateSimpleTexture2D( );
	GLuint compile_shader(GLenum type, const std::string& filename);
	GLuint link_program(const vector<GLuint>& shaders);

public:
	Debayer(uint32_t width, uint32_t height);
	void process(unsigned char * inGray, unsigned char * outRGB);
	~Debayer();
};

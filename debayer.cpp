#include "debayer.h"


Debayer::Debayer(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;
	bcm_host_init();
	if(esCreateWindow () == GL_FALSE)
		cout << "failed to create gl window" << endl;
	initShader();
}
void Debayer::process(unsigned char * inGray, unsigned char * outRGB)
{
	GLfloat vVertices[] = { 1.0f, -1.0f, 0.0f,  // Position 2
							0.0f,  0.0f,        // TexCoord 0
							1.0f,  1.0f, 0.0f,  // Position 3
							0.0f,  1.0f,        // TexCoord 1
						   -1.0f,  1.0f, 0.0f,  // Position 0
							1.0f,  1.0f,        // TexCoord 2
						   -1.0f, -1.0f, 0.0f,  // Position 1
							1.0f,  0.0f         // TexCoord 3
	};
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// Set the viewport
	glViewport ( 0, 0, width, height );

	// Clear the color buffer
	glClear ( GL_COLOR_BUFFER_BIT );

	// Use the program object
	glUseProgram ( programObject );

	// Load the vertex position
	glVertexAttribPointer ( positionLoc, 3, GL_FLOAT,
						   GL_FALSE, 5 * sizeof(GLfloat), vVertices );
	// Load the texture coordinate
	glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,
						   GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );

	glEnableVertexAttribArray ( positionLoc );
	glEnableVertexAttribArray ( texCoordLoc );

	// Bind the texture
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, textureId );

	// Load data to GPU
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, inGray );

	// Set the sampler texture unit to 0
	glUniform1i ( samplerLoc, 0 );

	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

	glReadPixels(0,0, width, height, GL_RGB, GL_UNSIGNED_BYTE, outRGB);
}

Debayer::~Debayer()
{
	// Delete texture object
	glDeleteTextures ( 1, &textureId );

	// Delete program object
	glDeleteProgram ( programObject );
}

GLboolean Debayer::esCreateWindow ()
{
	static EGL_DISPMANX_WINDOW_T nativewindow;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	uint32_t display_width;
	uint32_t display_height;

	// create an EGL window surface, passing context width/height
	if ( graphics_get_display_size(0 /* LCD */, &display_width, &display_height) < 0 )
	{
		return GL_FALSE;
	}

	display_width = this->width;
	display_height = this->height;

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = this->width;
	dst_rect.height = this->height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = this->width << 16;
	src_rect.height = this->height << 16;

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	nativewindow.element = vc_dispmanx_element_add ( dispman_update, dispman_display,
			  0/*layer*/, &dst_rect, 0/*src*/,
			  &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, DISPMANX_NO_ROTATE/*transform*/);;
	nativewindow.width = this->width;
	nativewindow.height = this->height;

	vc_dispmanx_update_submit_sync( dispman_update );

	this->hWnd = &nativewindow;

	if ( !CreateEGLContext() )
	{
		return GL_FALSE;
	}


	return GL_TRUE;
}

/// esCreateWindow flag - RGB color buffer
#define ES_WINDOW_RGB           0
/// esCreateWindow flag - ALPHA color buffer
#define ES_WINDOW_ALPHA         1
/// esCreateWindow flag - depth buffer
#define ES_WINDOW_DEPTH         2
/// esCreateWindow flag - stencil buffer
#define ES_WINDOW_STENCIL       4
/// esCreateWindow flat - multi-sample buffer
#define ES_WINDOW_MULTISAMPLE   8

EGLBoolean Debayer::CreateEGLContext ()
{
   EGLint numConfigs;
   EGLint majorVersion;
   EGLint minorVersion;
   EGLConfig config;

   EGLint attribList[] =
   {
       EGL_RED_SIZE,       5,
       EGL_GREEN_SIZE,     6,
       EGL_BLUE_SIZE,      5,
       EGL_ALPHA_SIZE,     (ES_WINDOW_RGB & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
       EGL_DEPTH_SIZE,     (ES_WINDOW_RGB & ES_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE,
       EGL_STENCIL_SIZE,   (ES_WINDOW_RGB & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
       EGL_SAMPLE_BUFFERS, (ES_WINDOW_RGB & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
       EGL_NONE
   };

   EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };


   // Get Display
   this->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if ( this->eglDisplay == EGL_NO_DISPLAY )
   {
      return EGL_FALSE;
   }

   // Initialize EGL
   if ( !eglInitialize(this->eglDisplay, &majorVersion, &minorVersion) )
   {
      return EGL_FALSE;
   }

   // Get configs
   if ( !eglGetConfigs(this->eglDisplay, NULL, 0, &numConfigs) )
   {
      return EGL_FALSE;
   }

   // Choose config
   if ( !eglChooseConfig(this->eglDisplay, attribList, &config, 1, &numConfigs) )
   {
      return EGL_FALSE;
   }

   // Create a surface
   this->eglSurface = eglCreateWindowSurface(this->eglDisplay, config, this->hWnd, NULL);
   if ( this->eglSurface == EGL_NO_SURFACE )
   {
      return EGL_FALSE;
   }

   // Create a GL context
   this->eglContext = eglCreateContext(this->eglDisplay, config, EGL_NO_CONTEXT, contextAttribs );
   if ( this->eglContext == EGL_NO_CONTEXT )
   {
      return EGL_FALSE;
   }

   // Make the context current
   if ( !eglMakeCurrent(this->eglDisplay, this->eglSurface, this->eglSurface, this->eglContext) )
   {
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

void Debayer::initShader()
{
	// Compile shaders:
	vector<GLuint> shaders;
	shaders.push_back(compile_shader(GL_VERTEX_SHADER, "demosaic.vrt"));
	shaders.push_back(compile_shader(GL_FRAGMENT_SHADER, "demosaic.frg"));

	// Link program:
	programObject = link_program(shaders);

	// Delete shaders again:
	for_each(shaders.begin(), shaders.end(), glDeleteShader);

	glUseProgram ( programObject );
	// Get locations:
	sourceSize = glGetUniformLocation(programObject, "sourceSize");
	firstRed = glGetUniformLocation(programObject, "firstRed");

	// BGGR = 0,0
	// GRBG = 0,1
	// GBRG = 1,0
	// RGGB = 1,1
	// Set constant uniforms:
	glUniform2f(firstRed, 1, 0);
	glUniform4f(sourceSize, width, height, 1.0 / width, 1.0 / height);

	// Get the attribute locations
	positionLoc = glGetAttribLocation ( programObject, "vert_pos" );
	texCoordLoc = glGetAttribLocation ( programObject, "texture_pos" );

	// Get the sampler location
	samplerLoc = glGetUniformLocation ( programObject, "source" );

	// Load the texture
	textureId = CreateSimpleTexture2D ();

	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
}

GLuint Debayer::CreateSimpleTexture2D( )
{
   // Texture object handle
   GLuint textureId;

   // Use tightly packed data
   glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

   // Generate a texture object
   glGenTextures ( 1, &textureId );

   // Bind the texture object
   glBindTexture ( GL_TEXTURE_2D, textureId );

   // Set the filtering mode
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return textureId;

}


GLuint Debayer::compile_shader(GLenum type, const std::string& filename)
{
    char *shader_text = NULL;
    int shader_length = 0;

    // Read file contents:
    //   Open file:
    ifstream file_stream(filename.c_str());

    //   Get file length:
    file_stream.seekg(0, ios::end);
    shader_length = file_stream.tellg();
    file_stream.seekg(0, ios::beg);

    //   Read into buffer:
    shader_text = new char[shader_length];
    file_stream.read(shader_text, shader_length);

    //    Close file:
    file_stream.close();


    // Create OpenGL shader:
    const GLuint handle = glCreateShader(type);
    const char *const_shader_text = shader_text;
    glShaderSource(handle, 1, &const_shader_text, &shader_length);
    delete[] shader_text;
    glCompileShader(handle);

    // Check for errors:
    GLint status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        // Get info log:
        GLint log_length;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        GLchar *log_text = new GLchar[log_length];
        glGetShaderInfoLog(handle, log_length, NULL, log_text);

        // Print log:
        cout << "Compilation of \"" << filename << "\" failed:\n" << log_text << "\n";
        delete[] log_text;

//        throw GLException("shader compilation failed");
    }

    return handle;
}

GLuint Debayer::link_program(const vector<GLuint>& shaders)
{
    typedef vector<GLuint>::const_iterator IT;

    // Create OpenGL program:
    const GLuint handle = glCreateProgram();

    // Attach the shaders:
    for(IT it = shaders.begin(); it != shaders.end(); ++it)
        glAttachShader(handle, *it);

    // Link:
    glLinkProgram(handle);

    // Detach the shaders:
    for(IT it = shaders.begin(); it != shaders.end(); ++it)
        glDetachShader(handle, *it);

    // Check for errors:
    GLint status;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        // Get info log:
        GLint log_length;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        GLchar *log_text = new GLchar[log_length];
        glGetProgramInfoLog(handle, log_length, NULL, log_text);

        // Print log:
        cout << "Linking of shader program failed:\n" << log_text << "\n";
        delete[] log_text;

//        throw GLException("program linking failed");
    }

    return handle;
}

#include <stdlib.h>
#include <stdio.h>

#include <bcm_host.h>
#include <interface/vchiq_arm/vchiq_if.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SDL/SDL.h>

#include "burner.h"

extern "C" {
#include "matrix.h"
}

typedef	struct ShaderInfo {
	GLuint program;
	GLint a_position;
	GLint a_texcoord;
	GLint u_vp_matrix;
	GLint u_texture;
} ShaderInfo;

static void drawQuad(const ShaderInfo *sh);
static GLuint createShader(GLenum type, const char *shaderSrc);
static GLuint createProgram(const char *vertexShaderSrc, const char *fragmentShaderSrc);

static EGL_DISPMANX_WINDOW_T nativeWindow;
static EGLDisplay display = NULL;
static EGLSurface surface = NULL;
static EGLContext context = NULL;

uint32_t physicalWidth = 0;
uint32_t physicalHeight = 0;

static int bufferWidth;
static int bufferHeight;
static int bufferBpp;
static int bufferPitch;
static unsigned char *bufferBitmap;

static int textureWidth;
static int textureHeight;
static int texturePitch;
static int textureFormat;
static unsigned char *textureBitmap;

static int screenRotated = 0;
static int screenFlipped = 0;

static ShaderInfo shader;
static GLuint buffers[3];
static GLuint texture;

static SDL_Surface *sdlScreen;

static const char* vertexShaderSrc =
	"uniform mat4 u_vp_matrix;\n"
	"attribute vec4 a_position;\n"
	"attribute vec2 a_texcoord;\n"
	"varying mediump vec2 v_texcoord;\n"
	"void main() {\n"
	"	v_texcoord = a_texcoord;\n"
	"	gl_Position = u_vp_matrix * a_position;\n"
	"}\n";

static const char* fragmentShaderSrc =
	"varying mediump vec2 v_texcoord;\n"
	"uniform sampler2D u_texture;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(u_texture, v_texcoord);\n"
	"}\n";

static const GLushort indices[] = {
	0, 1, 2,
	0, 2, 3,
};

static const int kVertexCount = 4;
static const int kIndexCount = 6;

static struct phl_matrix projection;

static const GLfloat vertices[] = {
	-0.5f, -0.5f, 0.0f,
	+0.5f, -0.5f, 0.0f,
	+0.5f, +0.5f, 0.0f,
	-0.5f, +0.5f, 0.0f,
};

static int piInitVideo()
{
	bcm_host_init();

	// get an EGL display connection
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetDisplay() failed: EGL_NO_DISPLAY\n");
		return 0;
	}

	// initialize the EGL display connection
	EGLBoolean result = eglInitialize(display, NULL, NULL);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglInitialize() failed: EGL_FALSE\n");
		return 0;
	}

	// get an appropriate EGL frame buffer configuration
	EGLint numConfig;
	EGLConfig config;
	static const EGLint attributeList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	result = eglChooseConfig(display, attributeList, &config, 1, &numConfig);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglChooseConfig() failed: EGL_FALSE\n");
		return 0;
	}

	result = eglBindAPI(EGL_OPENGL_ES_API);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglBindAPI() failed: EGL_FALSE\n");
		return 0;
	}

	// create an EGL rendering context
	static const EGLint contextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglCreateContext() failed: EGL_NO_CONTEXT\n");
		return 0;
	}

	// create an EGL window surface
	int32_t success = graphics_get_display_size(0, &physicalWidth, &physicalHeight);
	if (result < 0) {
		fprintf(stderr, "graphics_get_display_size() failed: < 0\n");
		return 0;
	}

	fprintf(stderr, "Width/height: %d/%d\n", physicalWidth, physicalHeight);

	VC_RECT_T dstRect;
	dstRect.x = 0;
	dstRect.y = 0;
	dstRect.width = physicalWidth;
	dstRect.height = physicalHeight;

	VC_RECT_T srcRect;
	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.width = physicalWidth << 16;
	srcRect.height = physicalHeight << 16;

	DISPMANX_DISPLAY_HANDLE_T dispManDisplay = vc_dispmanx_display_open(0);
	DISPMANX_UPDATE_HANDLE_T dispmanUpdate = vc_dispmanx_update_start(0);
	DISPMANX_ELEMENT_HANDLE_T dispmanElement = vc_dispmanx_element_add(dispmanUpdate,
		dispManDisplay, 0, &dstRect, 0, &srcRect,
		DISPMANX_PROTECTION_NONE, NULL, NULL, DISPMANX_NO_ROTATE);

	nativeWindow.element = dispmanElement;
	nativeWindow.width = physicalWidth;
	nativeWindow.height = physicalHeight;
	vc_dispmanx_update_submit_sync(dispmanUpdate);

	fprintf(stderr, "Initializing window surface...\n");

	surface = eglCreateWindowSurface(display, config, &nativeWindow, NULL);
	if (surface == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreateWindowSurface() failed: EGL_NO_SURFACE\n");
		return 0;
	}

	fprintf(stderr, "Connecting context to surface...\n");

	// connect the context to the surface
	result = eglMakeCurrent(display, surface, surface, context);
	if (result == EGL_FALSE) {
		fprintf(stderr, "eglMakeCurrent() failed: EGL_FALSE\n");
		return 0;
	}

	fprintf(stderr, "Initializing shaders...\n");

	// Init shader resources
	memset(&shader, 0, sizeof(ShaderInfo));
	shader.program = createProgram(vertexShaderSrc, fragmentShaderSrc);
	if (!shader.program) {
		fprintf(stderr, "createProgram() failed\n");
		return 0;
	}

	shader.a_position	= glGetAttribLocation(shader.program,	"a_position");
	shader.a_texcoord	= glGetAttribLocation(shader.program,	"a_texcoord");
	shader.u_vp_matrix	= glGetUniformLocation(shader.program,	"u_vp_matrix");
	shader.u_texture	= glGetUniformLocation(shader.program,	"u_texture");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);

	fprintf(stderr, "Initializing SDL video...\n");

	// We're doing our own video rendering - this is just so SDL-based keyboard
	// can work
	sdlScreen = SDL_SetVideoMode(0, 0, 0, 0);

	return 1;
}

static void piDestroyVideo()
{
	fprintf(stderr, "Destroying video...\n");

	if (sdlScreen) {
		SDL_FreeSurface(sdlScreen);
	}

	// Destroy shader resources
	if (shader.program) {
		glDeleteProgram(shader.program);
	}

	// Release OpenGL resources
	if (display) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroySurface(display, surface);
		eglDestroyContext(display, context);
		eglTerminate(display);
	}

	bcm_host_deinit();
}

static void piUpdateEmuDisplay()
{
	if (!shader.program) {
		fprintf(stderr, "Shader not initialized\n");
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, physicalWidth, physicalHeight);

	ShaderInfo *sh = &shader;

	glDisable(GL_BLEND);
	glUseProgram(sh->program);
	glUniformMatrix4fv(sh->u_vp_matrix, 1, GL_FALSE, &projection.xx);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	unsigned char *ps = (unsigned char *)bufferBitmap;
	unsigned char *pd = (unsigned char *)textureBitmap;

	for (int y = nVidImageHeight; y--;) {
		memcpy(pd, ps, nVidImagePitch);
		pd += texturePitch;
		ps += nVidImagePitch;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight,
		GL_RGB, textureFormat, textureBitmap);

	drawQuad(sh);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	eglSwapBuffers(display, surface);
}

static GLuint createShader(GLenum type, const char *shaderSrc)
{
	GLuint shader = glCreateShader(type);
	if (!shader) {
		fprintf(stderr, "glCreateShader() failed: %d\n", glGetError());
		return 0;
	}

	// Load and compile the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
	glCompileShader(shader);

	// Check the compile status
	GLint compiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char *)malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint createProgram(const char *vertexShaderSrc, const char *fragmentShaderSrc)
{
	GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSrc);
	if (!vertexShader) {
		fprintf(stderr, "createShader(GL_VERTEX_SHADER) failed\n");
		return 0;
	}

	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
	if (!fragmentShader) {
		fprintf(stderr, "createShader(GL_FRAGMENT_SHADER) failed\n");
		glDeleteShader(vertexShader);
		return 0;
	}

	GLuint programObject = glCreateProgram();
	if (!programObject) {
		fprintf(stderr, "glCreateProgram() failed: %d\n", glGetError());
		return 0;
	}

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	GLint linked = 0;
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = (char *)malloc(infoLen);
			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program: %s\n", infoLog);
			free(infoLog);
		}

		glDeleteProgram(programObject);
		return 0;
	}

	// Delete these here because they are attached to the program object.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

static void drawQuad(const ShaderInfo *sh)
{
	glUniform1i(sh->u_texture, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glVertexAttribPointer(sh->a_position, 3, GL_FLOAT,
		GL_FALSE, 3 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(sh->a_position);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glVertexAttribPointer(sh->a_texcoord, 2, GL_FLOAT,
		GL_FALSE, 2 * sizeof(GLfloat), NULL);
	glEnableVertexAttribArray(sh->a_texcoord);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);

	glDrawElements(GL_TRIANGLES, kIndexCount, GL_UNSIGNED_SHORT, 0);
}

static int closestPowerOfTwo(int num)
{
    int rv = 1;
    while (rv < num) {
    	rv *= 2;
    }
    return rv;
}

static int reinitTextures()
{
	fprintf(stderr, "Initializing textures...\n");
	
	textureWidth = closestPowerOfTwo(nVidImageWidth); // adjusted for rotation
	textureHeight = closestPowerOfTwo(nVidImageHeight); // adjusted for rotation
	texturePitch = textureWidth * bufferBpp;
	textureFormat = GL_UNSIGNED_SHORT_5_6_5;

	GLfloat minU = 0.0f;
	GLfloat minV = 0.0f;
	GLfloat maxU = ((float)nVidImageWidth / textureWidth - minU);
	GLfloat maxV = ((float)nVidImageHeight / textureHeight);
	GLfloat uvs[] = {
		minU, minV,
		maxU, minV,
		maxU, maxV,
		minU, maxV,
	};

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, textureFormat, NULL);

	glGenBuffers(3, buffers);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 3, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, kVertexCount * sizeof(GLfloat) * 2, uvs, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, kIndexCount * sizeof(GL_UNSIGNED_SHORT), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	float sx = 1.0f;
	float sy = 1.0f;
	float zoom = 1.0f;

	// Screen aspect ratio adjustment
	float a = (float)physicalWidth / (float)physicalHeight;
	float a0 = (float)bufferWidth / (float)bufferHeight;
	if (a > a0) {
		sx = a0/a;
	} else {
		sy = a/a0;
	}

	phl_matrix_identity(&projection);
	if (screenRotated) {
		phl_matrix_rotate_z(&projection, screenFlipped ? 270 : 90);
	}
	phl_matrix_ortho(&projection, -0.5f, +0.5f, +0.5f, -0.5f, -1.0f, 1.0f);
	phl_matrix_scale(&projection, sx * zoom, sy * zoom, 0);

	fprintf(stderr, "Setting up screen...\n");

	int bufferSize = bufferWidth * bufferHeight * bufferBpp;
	free(bufferBitmap);
	if ((bufferBitmap = (unsigned char *)malloc(bufferSize)) == NULL) {
		fprintf(stderr, "Error allocating buffer bitmap\n");
		return 0;
	}
	
	nBurnBpp = bufferBpp;
	nBurnPitch = nVidImagePitch;
	pVidImage = bufferBitmap;
	
	memset(bufferBitmap, 0, bufferSize);
	
	int textureSize = textureWidth * textureHeight * bufferBpp;
	free(textureBitmap);
	if ((textureBitmap = (unsigned char *)calloc(1, textureSize)) == NULL) {
		fprintf(stderr, "Error allocating buffer bitmap\n");
		return 0;
	}

	return 1;
}

// Specific to FB

static int FbInit()
{
	if (!piInitVideo()) {
		return 1;
	}
	
	int virtualWidth;
	int virtualHeight;

	if (bDrvOkay) {
		BurnDrvGetVisibleSize(&virtualWidth, &virtualHeight);
		if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
		    screenRotated = 1;
		}
		
		if (BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED) {
			screenFlipped = 1;
		}
		
		fprintf(stderr, "Game screen size: %dx%d (%s,%s)\n",
			virtualWidth, virtualHeight,
			screenRotated ? "rotated" : "not rotated",
			screenFlipped ? "flipped" : "not flipped");

		nVidImageDepth = 16;
		nVidImageBPP = 2;
		
		if (!screenRotated) {
			nVidImageWidth = virtualWidth;
			nVidImageHeight = virtualHeight;
		} else {
			nVidImageWidth = virtualHeight;
			nVidImageHeight = virtualWidth;
		}
		nVidImagePitch = nVidImageWidth * nVidImageBPP;
		
		SetBurnHighCol(nVidImageDepth);
		
		bufferWidth = virtualWidth;
		bufferHeight = virtualHeight;
		bufferBpp = nVidImageBPP;
		
		if (!reinitTextures()) {
			fprintf(stderr, "Error initializing textures\n");
			return 1;
		}
	}

	return 0;
}

static int FbExit()
{
	glDeleteBuffers(3, buffers);
	glDeleteTextures(1, &texture);

	free(bufferBitmap);
	free(textureBitmap);
	bufferBitmap = NULL;
	textureBitmap = NULL;

	piDestroyVideo();

	return 0;
}

static int FbRunFrame(bool bRedraw)
{
	if (pVidImage == NULL) {
		return 1;
	}

	if (bDrvOkay) {
		if (bRedraw) {								// Redraw current frame
			if (BurnDrvRedraw()) {
				BurnDrvFrame();						// No redraw function provided, advance one frame
			}
		} else {
			BurnDrvFrame();							// Run one frame and draw the screen
		}
	}

	return 0;
}

static int FbPaint(int bValidate)
{
	piUpdateEmuDisplay();
	return 0;
}

static int FbGetSettings(InterfaceInfo *)
{
	return 0;
}

static int FbVidScale(RECT *, int, int)
{
	return 0;
}

struct VidOut VidOutPi = { FbInit, FbExit, FbRunFrame, FbPaint, FbVidScale, FbGetSettings, _T("Raspberry Pi video output") };

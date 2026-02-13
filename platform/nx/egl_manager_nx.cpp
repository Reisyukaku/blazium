#include "egl_manager_nx.h"
#include "godot_nx.h"

#ifdef NX_ENABLED
#ifdef EGL_ENABLED

#define DEBUG_LOG(fmt, ...) ((void)0)

Error EGLManagerNX::initialize() {
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl_display, NULL, NULL);

    EGLConfig config;
	EGLint numConfigs;
	static const EGLint attributeList[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	eglChooseConfig(egl_display, attributeList, &config, 1, &numConfigs);
	if (numConfigs == 0) {
		DEBUG_LOG("No config found! error: %d", eglGetError());
		goto _fail1;
	}

	// Create an EGL window surface
	egl_surface = eglCreateWindowSurface(egl_display, config, nwindowGetDefault(), NULL);
	if (!egl_surface) {
		DEBUG_LOG("Surface creation failed! error: %d", eglGetError());
		goto _fail1;
	}

	static const EGLint contextAttributeList[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3, // request OpenGL ES 3.x
		EGL_NONE
	};

	// Create an EGL rendering context
	egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, contextAttributeList);
	if (!egl_context) {
		DEBUG_LOG("Context creation failed! error: %d", eglGetError());
		goto _fail2;
	}

	// Connect the context to the surface
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
	return OK;

_fail2:
	eglDestroySurface(egl_display, egl_surface);
	egl_surface = NULL;
_fail1:
	eglTerminate(egl_display);
	egl_display = NULL;
_fail0:
	return ERR_UNCONFIGURED;
}

void EGLManagerNX::release_current() {
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void EGLManagerNX::make_current() {
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
}

void EGLManagerNX::swap_buffers() {
	eglSwapBuffers(egl_display, egl_surface);
}

const char *EGLManagerNX::_get_platform_extension_name() const {
    return "EGL_NX";
}

EGLenum EGLManagerNX::_get_platform_extension_enum() const {
    return EGL_NONE;
}

EGLenum EGLManagerNX::_get_platform_api_enum() const {
    return EGL_OPENGL_ES_API;
}

Vector<EGLAttrib> EGLManagerNX::_get_platform_display_attributes() const {
    return Vector<EGLAttrib>();
}

Vector<EGLint> EGLManagerNX::_get_platform_context_attribs() const {
    Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_MAJOR_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_NONE);

	return ret;
}

#endif // EGL_ENABLED
#endif // NX_ENABLED
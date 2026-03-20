#include "egl_manager_nx.h"
#include "godot_nx.h"

#ifdef NX_ENABLED
#ifdef EGL_ENABLED

#define DEBUG_LOG(fmt, ...) ((void)0)

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
	Vector<EGLAttrib> ret;
	ret.push_back(EGL_NONE);
	return ret;
}

Vector<EGLint> EGLManagerNX::_get_platform_context_attribs() const {
	Vector<EGLint> ret;
	ret.push_back(EGL_CONTEXT_MAJOR_VERSION);
	ret.push_back(3);
	ret.push_back(EGL_CONTEXT_MINOR_VERSION);
	ret.push_back(0);
	ret.push_back(EGL_NONE);

	return ret;
}


#endif // EGL_ENABLED
#endif // NX_ENABLED
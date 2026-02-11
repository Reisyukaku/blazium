#pragma once

#ifdef NX_ENABLED
#ifdef EGL_ENABLED

#include "core/os/os.h"
#include "drivers/egl/egl_manager.h"

class EGLManagerNX : public EGLManager {
public:
	virtual const char *_get_platform_extension_name() const override;
	virtual EGLenum _get_platform_extension_enum() const override;
	virtual EGLenum _get_platform_api_enum() const override;
	virtual Vector<EGLAttrib> _get_platform_display_attributes() const override;
	virtual Vector<EGLint> _get_platform_context_attribs() const override;
};

#endif // EGL_ENABLED
#endif // NX_ENABLED

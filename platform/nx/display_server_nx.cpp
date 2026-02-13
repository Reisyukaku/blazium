#include "display_server_nx.h"
#include "core/os/os.h"
#include "godot_nx.h"

#if defined(GLES3_ENABLED)
#include "drivers/gles3/rasterizer_gles3.h"
#endif

DisplayServerNX *DisplayServerNX::get_singleton() {
    return static_cast<DisplayServerNX *>(DisplayServer::get_singleton());
}

void DisplayServerNX::Initialize() {
    Error r_error = OK;
    current_width = 1280;
    current_height = 720;
    is_docked = false;

    egl_manager = memnew(EGLManagerNX);
    if (egl_manager->initialize() != OK) {
        r_error = ERR_CANT_CREATE;
        return;
    }
    
    
}

void DisplayServerNX::gl_window_make_current(DisplayServer::WindowID p_window_id) {
    if (egl_manager) {
        egl_manager->window_make_current(MAIN_WINDOW_ID);
    }
}

int64_t DisplayServerNX::window_get_native_handle(HandleType p_handle_type, WindowID p_window) const {
    ERR_FAIL_COND_V(p_window != MAIN_WINDOW_ID, 0);
    switch (p_handle_type) {
        case WINDOW_HANDLE: {
            return 0;
        }
        case WINDOW_VIEW: {
            return 0; // Not supported.
        }
#ifdef GLES3_ENABLED
        case OPENGL_CONTEXT: {
            if (egl_manager) {
                return (int64_t)egl_manager->get_context(p_window);
            }
            return 0;
        } break;
        case EGL_DISPLAY: {
            if (egl_manager) {
                return (int64_t)egl_manager->get_display(p_window);
            }
            return 0;
        }
        case EGL_CONFIG: {
            if (egl_manager) {
                return (int64_t)egl_manager->get_config(p_window);
            }
            return 0;
        }
#endif
        default: {
            return 0;
        }
    }
}

bool DisplayServerNX::is_touchscreen_available() const {
    return appletGetOperationMode() == AppletOperationMode_Handheld;
}

Point2i DisplayServerNX::mouse_get_position() const {
    HidTouchScreenState touch_state;
    if (hidGetTouchScreenStates(&touch_state, 1) && touch_state.count > 0) {
        return Point2i(touch_state.touches[0].x, touch_state.touches[0].y);
    }
    
    return Point2i(current_width / 2, current_height / 2);
}

void DisplayServerNX::update_screen_mode() {
    
    bool new_docked = (appletGetOperationMode() == AppletOperationMode_Console);
    
    if (new_docked != is_docked) {
        is_docked = new_docked;
        
        if (is_docked) {
            current_width = 1920;
            current_height = 1080;
        } else {
            current_width = 1280;
            current_height = 720;
        }
        
        if (egl_manager) {
            // May need to recreate surface or just update viewport
            // This depends on how libnx handles resolution changes
        }

        nwindowSetDimensions(nwindowGetDefault(), current_width, current_height);

        window_set_size(Size2i(current_width, current_height));
    }
}

Size2i DisplayServerNX::window_get_size(int p_window) const {
    return Size2i(current_width, current_height);
}

Size2i DisplayServerNX::screen_get_size(int p_screen) const {
    return Size2i(current_width, current_height);
}

void DisplayServerNX::process_events() {
    update_screen_mode();
}

void DisplayServerNX::swap_buffers() {
    if (egl_manager) {
        egl_manager->swap_buffers();
    }
}
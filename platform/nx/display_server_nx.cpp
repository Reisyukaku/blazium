#include "display_server_nx.h"
#include "core/os/os.h"
#include "os_nx.h"
#include "godot_nx.h"

#if defined(GLES3_ENABLED)
#include "drivers/gles3/rasterizer_gles3.h"
#endif

DisplayServerNX *DisplayServerNX::get_singleton() {
    return static_cast<DisplayServerNX *>(DisplayServer::get_singleton());
}

void DisplayServerNX::Initialize() {
	current_width = 1280;
	current_height = 720;
	is_docked = false;

	egl_manager = memnew(EGLManagerNX);
	if (egl_manager->initialize(EGL_DEFAULT_DISPLAY) != OK) {
		ERR_PRINT("NXP: Failed to initialize EGL Manager");
		return;
	}

	if (egl_manager->window_create(MAIN_WINDOW_ID, EGL_DEFAULT_DISPLAY, nwindowGetDefault(), current_width, current_height) != OK) {
		ERR_PRINT("NXP: Failed to create EGL window");
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
			egl_manager->window_destroy(MAIN_WINDOW_ID);
			nwindowSetDimensions(nwindowGetDefault(), current_width, current_height);
			egl_manager->window_create(MAIN_WINDOW_ID, EGL_DEFAULT_DISPLAY, nwindowGetDefault(), current_width, current_height);
		}
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

	OS_NX *os = static_cast<OS_NX *>(OS::get_singleton());
	padUpdate(&os->padState);

	u64 buttons_down = padGetButtonsDown(&os->padState);
	u64 buttons_up = padGetButtonsUp(&os->padState);

	auto handle_button = [&](u64 libnx_button, JoyButton godot_button) {
		if (buttons_down & libnx_button) {
			Input::get_singleton()->joy_button(0, godot_button, true);
		}
		if (buttons_up & libnx_button) {
			Input::get_singleton()->joy_button(0, godot_button, false);
		}
	};

	handle_button(HidNpadButton_A, JoyButton::A);
	handle_button(HidNpadButton_B, JoyButton::B);
	handle_button(HidNpadButton_X, JoyButton::X);
	handle_button(HidNpadButton_Y, JoyButton::Y);
	handle_button(HidNpadButton_StickL, JoyButton::LEFT_STICK);
	handle_button(HidNpadButton_StickR, JoyButton::RIGHT_STICK);
	handle_button(HidNpadButton_L, JoyButton::LEFT_SHOULDER);
	handle_button(HidNpadButton_R, JoyButton::RIGHT_SHOULDER);
	handle_button(HidNpadButton_Plus, JoyButton::START);
	handle_button(HidNpadButton_Minus, JoyButton::BACK);
	handle_button(HidNpadButton_Up, JoyButton::DPAD_UP);
	handle_button(HidNpadButton_Down, JoyButton::DPAD_DOWN);
	handle_button(HidNpadButton_Left, JoyButton::DPAD_LEFT);
	handle_button(HidNpadButton_Right, JoyButton::DPAD_RIGHT);

	// ZL/ZR as buttons and axes (since Switch ZL/ZR are digital)
	if (buttons_down & HidNpadButton_ZL) {
		Input::get_singleton()->joy_button(0, JoyButton::INVALID, true); // No specific button for trigger in enum, use axis
		Input::get_singleton()->joy_axis(0, JoyAxis::TRIGGER_LEFT, 1.0f);
	}
	if (buttons_up & HidNpadButton_ZL) {
		Input::get_singleton()->joy_axis(0, JoyAxis::TRIGGER_LEFT, 0.0f);
	}
	if (buttons_down & HidNpadButton_ZR) {
		Input::get_singleton()->joy_axis(0, JoyAxis::TRIGGER_RIGHT, 1.0f);
	}
	if (buttons_up & HidNpadButton_ZR) {
		Input::get_singleton()->joy_axis(0, JoyAxis::TRIGGER_RIGHT, 0.0f);
	}

	// Sticks
	HidAnalogStickState stick_l = padGetStickPos(&os->padState, 0);
	HidAnalogStickState stick_r = padGetStickPos(&os->padState, 1);

	static float last_l_x = 0, last_l_y = 0, last_r_x = 0, last_r_y = 0;

	float l_x = (float)stick_l.x / 32767.0f;
	float l_y = (float)-stick_l.y / 32767.0f; // Invert Y
	float r_x = (float)stick_r.x / 32767.0f;
	float r_y = (float)-stick_r.y / 32767.0f; // Invert Y

	if (l_x != last_l_x) Input::get_singleton()->joy_axis(0, JoyAxis::LEFT_X, l_x);
	if (l_y != last_l_y) Input::get_singleton()->joy_axis(0, JoyAxis::LEFT_Y, l_y);
	if (r_x != last_r_x) Input::get_singleton()->joy_axis(0, JoyAxis::RIGHT_X, r_x);
	if (r_y != last_r_y) Input::get_singleton()->joy_axis(0, JoyAxis::RIGHT_Y, r_y);

	last_l_x = l_x; last_l_y = l_y; last_r_x = r_x; last_r_y = r_y;
}

void DisplayServerNX::swap_buffers() {
    if (egl_manager) {
        egl_manager->swap_buffers();
    }
}
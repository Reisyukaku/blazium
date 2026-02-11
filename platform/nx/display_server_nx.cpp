#include "display_server_nx.h"
#include "core/os/os.h"
#include "godot_nx.h"

    DisplayServerNX *DisplayServerNX::get_singleton() {
        return static_cast<DisplayServerNX *>(DisplayServer::get_singleton());
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
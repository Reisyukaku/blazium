#pragma once

#include "servers/display_server.h"
#include "egl_manager_nx.h"

class DisplayServerNX : public DisplayServer {
public:
    static DisplayServerNX *get_singleton();

    void update_screen_mode();

	bool is_touchscreen_available() const override;
	Point2i mouse_get_position() const override;

    virtual Size2i window_get_size(int p_window = MAIN_WINDOW_ID) const override;
    virtual Size2i screen_get_size(int p_screen = SCREEN_OF_MAIN_WINDOW) const override;
    
    void process_events() override;

	virtual void swap_buffers() override;

    void gl_window_make_current(DisplayServer::WindowID p_window_id) override;
    int64_t window_get_native_handle(HandleType p_handle_type, WindowID p_window) const override;

    void Initialize();

private:
    int current_width, current_height;
    bool is_docked;
    EGLManagerNX *egl_manager = nullptr;
};
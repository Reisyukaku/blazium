#pragma once

#include "servers/display_server.h"
#include "egl_manager_nx.h"

class DisplayServerNX : public DisplayServer {
    EGLManagerNX *egl_manager = nullptr;

public:
    static DisplayServerNX *get_singleton();

    void update_screen_mode();

	bool is_touchscreen_available() const override;
	Point2i mouse_get_position() const override;

    virtual Size2i window_get_size(int p_window = MAIN_WINDOW_ID) const override;
    virtual Size2i screen_get_size(int p_screen = SCREEN_OF_MAIN_WINDOW) const override;
    
    void process_events() override;

	virtual void swap_buffers() override;

private:
    int current_width, current_height;
    bool is_docked;
};
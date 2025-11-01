#pragma once

#include "servers/display_server.h"

class DisplayServerNX : public DisplayServer {
public:
    static DisplayServerNX *get_singleton();

    // touch
	virtual bool is_touchscreen_available() const override;

    // mouse
	virtual Point2i mouse_get_position() const override;

    // others
	virtual void swap_buffers() override;
};
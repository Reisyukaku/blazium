/**************************************************************************/
/*  os_nx.h                                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef OS_NX_H
#define OS_NX_H

#include "core/config/project_settings.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "drivers/unix/ip_unix.h"
#include "drivers/unix/net_socket_unix.h"

class OS_NX : public OS {
protected:
    virtual void initialize_core();

public:
    // Lifecycle
    void initialize() override;
    void initialize_joypads() override;
    void set_main_loop(MainLoop *p_main_loop) override;
    void delete_main_loop() override;
    void finalize() override;
    void finalize_core() override;

    // Internal features
    bool _check_internal_feature_support(const String &p_feature) override;

    // Video / input
    Vector<String> get_video_adapter_driver_info() const override;
    String get_stdin_string(int64_t p_buffer_size = 1024) override;
    PackedByteArray get_stdin_buffer(int64_t p_buffer_size = 1024) override;

    // Process / execution
    Error execute(const String &p_path, const List<String> &p_arguments, String *r_pipe = nullptr, int *r_exitcode = nullptr, bool read_stderr = false, Mutex *p_pipe_mutex = nullptr, bool p_open_console = false) override;
    Error create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id = nullptr, bool p_open_console = false) override;
    Error kill(const ProcessID &p_pid) override;
    bool is_process_running(const ProcessID &p_pid) const override;
    int get_process_exit_code(const ProcessID &p_pid) const override;

    // Environment
    bool has_environment(const String &p_var) const override;
    String get_environment(const String &p_var) const override;
    void set_environment(const String &p_var, const String &p_value) const override;
    void unset_environment(const String &p_var) const override;

    // Platform info
    String get_name() const override;
    String get_distribution_name() const override;
    String get_version() const override;

    // Main loop / timing
    MainLoop *get_main_loop() const override;
    OS::DateTime get_datetime(bool utc = false) const override;
    void delay_usec(uint32_t p_usec) const override;
    uint64_t get_ticks_usec() const override;

    Error get_entropy(uint8_t *r_buffer, int p_bytes) override;
    TimeZoneInfo get_time_zone_info() const override;

private:
    MainLoop *main_loop = nullptr;
};

#endif // OS_NX_H

/**************************************************************************/
/*  os_nx.cpp                                                             */
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

#include <stdarg.h>
#include "os_nx.h"
#include "core/os/os.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "drivers/unix/dir_access_unix.h"
#include "drivers/unix/file_access_unix.h"
#include "drivers/unix/thread_posix.h"
#include "drivers/unix/net_socket_unix.h"
#include "drivers/unix/ip_unix.h"
#include "display_server_nx.h"

static u64 start_tick = 0;

void OS_NX::initialize() {
	initialize_swkbd();
	DisplayServerNX::get_singleton()->Initialize();
}

void OS_NX::initialize_core() {
#ifdef THREADS_ENABLED
	init_thread_posix();
#endif
	start_tick = armGetSystemTick();

	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_FILESYSTEM);

#ifndef UNIX_SOCKET_UNAVAILABLE
	NetSocketUnix::make_default();
	IPUnix::make_default();
#endif
}

void OS_NX::finalize() {
	//
}

void OS_NX::finalize_core() {
#ifndef UNIX_SOCKET_UNAVAILABLE
	NetSocketUnix::cleanup();
#endif
}

void OS_NX::initialize_joypads() {
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	padInitializeDefault(&padState);
}

void OS_NX::initialize_swkbd() {
	/*swkbdInlineLaunchForLibraryApplet(&inline_keyboard, SwkbdInlineMode_AppletDisplay, 0);
	swkbdInlineSetChangedStringCallback(&inline_keyboard, keyboard_string_changed_callback);
	swkbdInlineSetMovedCursorCallback(&inline_keyboard, keyboard_moved_cursor_callback);
	swkbdInlineSetDecidedEnterCallback(&inline_keyboard, keyboard_decided_enter_callback);
	swkbdInlineSetDecidedCancelCallback(&inline_keyboard, keyboard_decided_cancel_callback);*/
}

void OS_NX::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

void OS_NX::delete_main_loop() {
	if (main_loop)
		memdelete(main_loop);
	main_loop = nullptr;
}

bool OS_NX::_check_internal_feature_support(const String &p_feature) {
	return false;
}

Vector<String> OS_NX::get_video_adapter_driver_info() const {
	return Vector<String>();
}

String OS_NX::get_stdin_string(int64_t p_buffer_size) {
	return "";
}

PackedByteArray OS_NX::get_stdin_buffer(int64_t p_buffer_size) {
	return PackedByteArray();
}

Error OS_NX::execute(const String &p_path, const List<String> &p_arguments, String *r_pipe, int *r_exitcode, bool read_stderr, Mutex *p_pipe_mutex, bool p_open_console) {
	return ERR_UNAVAILABLE;
}

Error OS_NX::create_process(const String &p_path, const List<String> &p_arguments, ProcessID *r_child_id, bool p_open_console) {
	return ERR_UNAVAILABLE;
}

Error OS_NX::kill(const ProcessID &p_pid) {
	return ERR_UNAVAILABLE;
}

bool OS_NX::is_process_running(const ProcessID &p_pid) const {
	return false;
}

int OS_NX::get_process_exit_code(const ProcessID &p_pid) const {
	return 0;
}

bool OS_NX::has_environment(const String &p_var) const {
	return false;
}

String OS_NX::get_environment(const String &p_var) const {
	return "";
}

void OS_NX::set_environment(const String &p_var, const String &p_value) const {
	//
}

void OS_NX::unset_environment(const String &p_var) const {
	//
}

String OS_NX::get_name() const {
	return "NX";
}

String OS_NX::get_distribution_name() const {
	return "NX";
}

String OS_NX::get_version() const {
	return "1.0";
}

MainLoop *OS_NX::get_main_loop() const {
	return main_loop;
}

OS::DateTime OS_NX::get_datetime(bool utc) const {
	u64 timestamp = 0;
	TimeCalendarTime caltime;
	TimeCalendarAdditionalInfo info;

	if (R_SUCCEEDED(timeGetCurrentTime(utc ? TimeType_UserSystemClock : TimeType_LocalSystemClock, &timestamp))) {
		if (R_SUCCEEDED(timeToCalendarTimeWithMyRule(timestamp, &caltime, &info))) {
			DateTime dt;
			dt.year = caltime.year;
			dt.month = (Month)caltime.month;
			dt.day = caltime.day;
			dt.hour = caltime.hour;
			dt.minute = caltime.minute;
			dt.second = caltime.second;
			return dt;
		}
	}
	return {};
}

void OS_NX::delay_usec(uint32_t p_usec) const {
	svcSleepThread(p_usec * 1000ULL);
}

uint64_t OS_NX::get_ticks_usec() const {
	u64 ticks = armGetSystemTick() - start_tick;
	return armTicksToNs(ticks) / 1000;
}

Error OS_NX::get_entropy(uint8_t *r_buffer, int p_bytes) {
	if (R_FAILED(csrngGetRandomBytes(r_buffer, p_bytes))) {
		return ERR_CANT_CREATE;
	}
	return OK;
}

OS::TimeZoneInfo OS_NX::get_time_zone_info() const {
	TimeZoneInfo tz;
	tz.bias = 0;
	tz.name = "UTC";
	return tz;
}


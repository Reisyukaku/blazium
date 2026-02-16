/**************************************************************************/
/*  export_plugin.cpp                                                     */
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

#include "export_plugin.h"

#include "logo_svg.gen.h"
#include "run_icon_svg.gen.h"

#include "core/config/project_settings.h"
#include "core/io/image_loader.h"
#include "editor/editor_node.h"
#include "editor/editor_paths.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/themes/editor_scale.h"

#include "modules/svg/image_loader_svg.h"

static const int EXPORT_FORMAT_NRO = 0;
static const int EXPORT_FORMAT_NSP = 1;

String EditorExportPlatformNX::get_name() const {
	return "Nintendo Switch";
}

String EditorExportPlatformNX::get_os_name() const {
	return "NX";
}

Error EditorExportPlatformNX::store_file_at_path(const String &p_path, const Vector<uint8_t> &p_data) {
	Error err = OK;

	String p_dir = p_path.get_base_dir();
	if (!DirAccess::exists(p_dir)) {
		Ref<DirAccess> filesystem_da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		ERR_FAIL_COND_V_MSG(filesystem_da.is_null(), ERR_CANT_CREATE, "Cannot create directory '" + p_dir + "'.");
		err = filesystem_da->make_dir_recursive(p_dir);
		ERR_FAIL_COND_V_MSG(err, ERR_CANT_CREATE, "Cannot create directory '" + p_dir + "'.");
	}
	if (err != OK) 
		return err;

	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), ERR_CANT_CREATE, "Cannot create file '" + p_path + "'.");
	fa->store_buffer(p_data.ptr(), p_data.size());

	return err;
}

Error EditorExportPlatformNX::_save_romfs_files(void *p_userdata, const String &p_path, const Vector<uint8_t> &p_data, int p_file, int p_total, const Vector<String> &p_enc_in_filters, const Vector<String> &p_enc_ex_filters, const Vector<uint8_t> &p_key, uint64_t p_seed){
	NxExportData *export_data = static_cast<NxExportData *>(p_userdata);
	const String path = ResourceUID::ensure_path(p_path);
	const String dst_path = path.replace_first("res://", export_data->asset_path + "/");
	print_verbose("Saving project files from " + path + " into " + dst_path);
	Error err = store_file_at_path(dst_path, p_data);
	return err;
}

Error EditorExportPlatformNX::_remove_romfs_files(void *p_userdata, const String &p_path)
{
	Error err = OK;
	//TODO
	return err;
}

Error EditorExportPlatformNX::_save_shared_obj(void *p_userdata, const SharedObject &p_so)
{
	Error err = OK;
	//TODO: this is for dll creation (offical NRO)
	return err;
}

Error EditorExportPlatformNX::_write_or_error(const uint8_t *p_content, int p_size, String p_path, String p_stage, bool p_compress) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_null()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR(p_stage), vformat(TTR("Could not write file: \"%s\"."), p_path));
		return ERR_FILE_CANT_WRITE;
	}
	if (p_compress) {
		PackedByteArray compressed_data;
		compressed_data.resize(Compression::get_max_compressed_buffer_size(p_size, Compression::MODE_GZIP));
		int compressed_size = Compression::compress(compressed_data.ptrw(), p_content, p_size, Compression::MODE_GZIP);
		f->store_buffer(compressed_data.ptr(), compressed_size);
	} else {
		f->store_buffer(p_content, p_size);
	}
	return OK;
}

Error EditorExportPlatformNX::_extract_template(const String &p_template, const String &p_name) {
	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);
	unzFile pkg = unzOpen2(p_template.utf8().get_data(), &io);

	if (!pkg) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"), vformat(TTR("Could not open template for export: \"%s\"."), p_template));
		return ERR_FILE_NOT_FOUND;
	}

	if (unzGoToFirstFile(pkg) != UNZ_OK) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"), vformat(TTR("Invalid export template: \"%s\"."), p_template));
		unzClose(pkg);
		return ERR_FILE_CORRUPT;
	}

	do {
		//get filename
		unz_file_info info;
		char fname[16384];
		unzGetCurrentFileInfo(pkg, &info, fname, 16384, nullptr, 0, nullptr, 0);

		String file = String::utf8(fname);

		// Skip folders.
		if (file.ends_with("/")) {
			continue;
		}

		Vector<uint8_t> data;
		data.resize(info.uncompressed_size);

		//read
		unzOpenCurrentFile(pkg);
		unzReadCurrentFile(pkg, data.ptrw(), data.size());
		unzCloseCurrentFile(pkg);

		//write
		String dst = p_template.get_base_dir();
		Error err = _write_or_error(data.ptr(), data.size(), dst, "Prepare Templates", false);
		if (err != OK) {
			unzClose(pkg);
			return err;
		}
	} while (unzGoToNextFile(pkg) == UNZ_OK);
	unzClose(pkg);
	return OK;
}

Error EditorExportPlatformNX::export_as_nro(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags) {
	Error err = OK;

	String devkitpro = OS::get_singleton()->get_environment("DEVKITPRO");

	String title = p_preset->get("application/title");
    String author = p_preset->get("application/author");
    String version = p_preset->get("application/version");
    String icon = p_preset->get("application/icon_256x256");

	String tmp_dir = EditorPaths::get_singleton()->get_cache_dir().path_join("nx_export_temp");
    
    // Create temp directory
    Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
    if (!dir->dir_exists(tmp_dir)) {
        err = dir->make_dir_recursive(tmp_dir);
        if (err != OK) {
            add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to create temp directory: " + tmp_dir);
            return err;
        }
    }
    
    // Export romfs
    NxExportData expData;
    expData.asset_path = tmp_dir.path_join("romfs");
    expData.debug = p_debug;
    
    err = dir->make_dir_recursive(expData.asset_path);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to create romfs directory");
        return err;
    }
    
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Exporting project files...");
    err = export_project_files(p_preset, p_debug, 
                               EditorExportPlatformNX::_save_romfs_files, 
                               EditorExportPlatformNX::_remove_romfs_files, 
                               (void *)&expData, 
                               EditorExportPlatformNX::_save_shared_obj);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to export project files, error: " + itos(err));
        return err;
    }
    
    // Create NACP
    String nacp_path = tmp_dir.path_join("game.nacp");
    
    List<String> args;
    args.push_back("--create");
    args.push_back(title);
    args.push_back(author);
    args.push_back(version);
    args.push_back(nacp_path);
    
    int exit_code;
    String output;
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Creating NACP...");
    err = OS::get_singleton()->execute("nacptool", args, &output, &exit_code);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to execute nacptool: " + itos(err));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Output: " + output);
        return ERR_CANT_CREATE;
    }
    if (exit_code != 0) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "nacptool exit code: " + itos(exit_code));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), output);
        return ERR_CANT_CREATE;
    }
    
    // Get template
    String error_msg;
    String template_path = p_debug ? p_preset->get("custom_template/debug") : p_preset->get("custom_template/release");
    
    if (template_path.is_empty()) {
        add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Looking for template...");
        template_path = find_export_template("nx.zip", &error_msg);
        if (template_path.is_empty()) {
            add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Template not found: " + error_msg);
            return ERR_FILE_NOT_FOUND;
        }
    }
    
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Template: " + template_path);
    
    if (!FileAccess::exists(template_path)) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Template file does not exist");
        return ERR_FILE_NOT_FOUND;
    }

    String elf_filename = p_debug ? "nx_debug.arm64" : "nx_release.arm64";
    String engine_elf = template_path.get_base_dir().path_join(elf_filename);
    if (!FileAccess::exists(engine_elf))
    {
        err = _extract_template(template_path, elf_filename);
        if (err) {
            add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to extract template: " + err);
            //return err;
        }
    }

    // Build NRO
    String out_nro = p_path.ends_with(".nro") ? p_path : p_path + ".nro";

    List<String> nro_args;
    nro_args.push_back(engine_elf);
    nro_args.push_back(out_nro);
    nro_args.push_back("--nacp=" + nacp_path);
    
    if (!icon.is_empty() && FileAccess::exists(icon)) {
        nro_args.push_back("--icon=" + icon);
    }
    
    nro_args.push_back("--romfsdir=" + expData.asset_path);
    
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Running elf2nro...");
    err = OS::get_singleton()->execute("elf2nro", nro_args, &output, &exit_code);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to execute elf2nro: " + itos(err));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), output);
        return ERR_CANT_CREATE;
    }
    if (exit_code != 0) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "elf2nro exit code: " + itos(exit_code));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), output);
        return ERR_CANT_CREATE;
    }

	add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Export NRO successful: " + out_nro);

	return err;
}

Error EditorExportPlatformNX::export_as_nsp(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags) {
	Error err = OK;

	String devkitpro = OS::get_singleton()->get_environment("DEVKITPRO");

	String title = p_preset->get("application/title");
    String author = p_preset->get("application/author");
    String version = p_preset->get("application/version");
    String icon = p_preset->get("application/icon_256x256");
	String npdm_json = p_preset->get("application/npdm_json");

	String buildPfs0 = devkitpro.path_join("tools").path_join("bin").path_join("build_pfs0");
	String elf2nso = devkitpro.path_join("tools").path_join("bin").path_join("elf2nso");
	String npdmTool = devkitpro.path_join("tools").path_join("bin").path_join("npdmtool");
    String buildRomfs = devkitpro.path_join("tools").path_join("bin").path_join("build_romfs");

	String tmp_dir = EditorPaths::get_singleton()->get_cache_dir().path_join("nx_export_temp");

	int exit_code;
    String output;

	// Create temp directory
    Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
    if (!dir->dir_exists(tmp_dir)) {
        err = dir->make_dir_recursive(tmp_dir);
        if (err != OK) {
            add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to create temp directory: " + tmp_dir);
            return err;
        }
    }

	// Get template
    String error_msg;
    String template_path = p_debug ? p_preset->get("custom_template/debug") : p_preset->get("custom_template/release");
    
    if (template_path.is_empty()) {
        add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Looking for template...");
        template_path = find_export_template("nx.zip", &error_msg);
        if (template_path.is_empty()) {
            add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Template not found: " + error_msg);
            return ERR_FILE_NOT_FOUND;
        }
    }
    
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Template: " + template_path);
    
    if (!FileAccess::exists(template_path)) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Template file does not exist");
        return ERR_FILE_NOT_FOUND;
    }

    String elf_filename = p_debug ? "nx_debug.arm64" : "nx_release.arm64";
	String json_filename = "npdm.json";
    String engine_elf = template_path.get_base_dir().path_join(elf_filename);
    if (!FileAccess::exists(engine_elf))
    {
        err = _extract_template(template_path, elf_filename);
        if (err) {
            add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to extract template: " + err);
            //return err;
        }

		//If no json specified, use default
		if(npdm_json.is_empty())
		{
			err = _extract_template(template_path, json_filename);
			if (err) {
				add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to extract default npdm json: " + err);
				//return err;
			}
			npdm_json = template_path.get_base_dir().path_join(json_filename);
		}
    }
    
    // Export romfs
    NxExportData expData;
    expData.asset_path = tmp_dir.path_join("romfs");
    expData.debug = p_debug;
    
    err = dir->make_dir_recursive(expData.asset_path);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to create romfs directory");
        return err;
    }
    
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Exporting project files...");
    err = export_project_files(p_preset, p_debug, 
                               EditorExportPlatformNX::_save_romfs_files, 
                               EditorExportPlatformNX::_remove_romfs_files, 
                               (void *)&expData, 
                               EditorExportPlatformNX::_save_shared_obj);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to export project files, error: " + itos(err));
        return err;
    }

	//Build romfs
	List<String> romfs_args;
    romfs_args.push_back(expData.asset_path);
    romfs_args.push_back(tmp_dir.path_join("romfs.bin"));
	err = OS::get_singleton()->execute("build_romfs", romfs_args, &output, &exit_code);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to execute build_romfs: " + itos(err));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Output: " + output);
        return ERR_CANT_CREATE;
    }

	//Build NSO
	List<String> nso_args;
    nso_args.push_back(engine_elf);
    nso_args.push_back(tmp_dir.path_join("exefs").path_join("main"));
	err = OS::get_singleton()->execute("elf2nso", nso_args, &output, &exit_code);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to execute elf2nso: " + itos(err));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Output: " + output);
        return ERR_CANT_CREATE;
    }

	//Build NPDM
	List<String> npdm_args;
    npdm_args.push_back(npdm_json);
    npdm_args.push_back(tmp_dir.path_join("exefs").path_join("main.npdm"));
	err = OS::get_singleton()->execute("npdmtool", npdm_args, &output, &exit_code);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to execute npdmtool: " + itos(err));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Output: " + output);
        return ERR_CANT_CREATE;
    }

	//Build PFS0
	List<String> pfs0_args;
	String dest_file = p_path.path_join(title + ".nsp");
    pfs0_args.push_back(tmp_dir.path_join("exefs"));
    pfs0_args.push_back(dest_file);
	err = OS::get_singleton()->execute("build_pfs0", pfs0_args, &output, &exit_code);
    if (err != OK) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Failed to execute build_pfs0: " + itos(err));
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), "Output: " + output);
        return ERR_CANT_CREATE;
    }

	return err;
}

Error EditorExportPlatformNX::export_project(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, BitField<EditorExportPlatform::DebugFlags> p_flags) {
    Error err = OK;
    
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Starting NX Export...");
    add_message(EXPORT_MESSAGE_INFO, TTR("Export"), "Path: " + p_path);
    
	int exportType = p_preset->get("application/export_format");
    
	String devkitpro = OS::get_singleton()->get_environment("DEVKITPRO");
    if (devkitpro.is_empty()) {
        add_message(EXPORT_MESSAGE_ERROR, TTR("Export"), 
                   "DEVKITPRO environment variable not set. Please install DevkitPro.");
        return ERR_CANT_CREATE;
    }

	switch(exportType)
	{
		case EXPORT_FORMAT_NRO:
		{
			err = export_as_nro(p_preset, p_debug, p_path, p_flags);
			break;
		}
		case EXPORT_FORMAT_NSP:
		{
			err = export_as_nsp(p_preset, p_debug, p_path, p_flags);
			break;
		}
	}
    
    return err;
}

void EditorExportPlatformNX::get_export_options(List<ExportOption> *r_options) const {
	String title = ProjectSettings::get_singleton()->get("application/config/name");
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/title", PROPERTY_HINT_PLACEHOLDER_TEXT, "App Title"), title));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/author", PROPERTY_HINT_PLACEHOLDER_TEXT, "App Author"), "Blazium"));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/version", PROPERTY_HINT_PLACEHOLDER_TEXT, "App Version"), "1.0"));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/icon_256x256", PROPERTY_HINT_GLOBAL_FILE, "*.jpg"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/npdm_json", PROPERTY_HINT_GLOBAL_FILE, "*.json"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::INT, "application/export_format", PROPERTY_HINT_ENUM, "NRO,NSP"), EXPORT_FORMAT_NRO, false, true));

	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "custom_template/debug", PROPERTY_HINT_GLOBAL_FILE, "*.elf"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "custom_template/release", PROPERTY_HINT_GLOBAL_FILE, "*.elf"), ""));
}

bool EditorExportPlatformNX::get_export_option_visibility(const EditorExportPreset *p_preset, const String &p_option) const {
	if (p_option == "custom_template/debug" ||
		p_option == "custom_template/release")
			return p_preset->are_advanced_options_enabled();

	return true;
}

bool EditorExportPlatformNX::has_valid_export_configuration(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates, bool p_debug) const {
	String err;
	bool valid = true;

	return valid;
}

bool EditorExportPlatformNX::has_valid_project_configuration(const Ref<EditorExportPreset> &p_preset, String &r_error) const {
	String err;
	bool valid = true;

	return valid;
}

bool EditorExportPlatformNX::poll_export() {
	return false;
}

Ref<ImageTexture> EditorExportPlatformNX::get_option_icon(int p_index) const {
	return p_index == 1 ? stop_icon : EditorExportPlatform::get_option_icon(p_index);
}

int EditorExportPlatformNX::get_options_count() const {
	return menu_options;
}

void EditorExportPlatformNX::get_preset_features(const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const {
    r_features->push_back("nx");
}

String EditorExportPlatformNX::get_option_label(int p_index) const {
	return "";
}

String EditorExportPlatformNX::get_option_tooltip(int p_index) const {
	return "";
}

Error EditorExportPlatformNX::run(const Ref<EditorExportPreset> &p_preset, int p_device, BitField<EditorExportPlatform::DebugFlags> p_debug_flags) {
	String nxlink = EditorSettings::get_singleton()->get("export/nx/nxlink");
	//TODO: push over nxlink (nro only?)
	return OK;
}

List<String> EditorExportPlatformNX::get_binary_extensions(const Ref<EditorExportPreset> &p_preset) const {
	List<String> extensions;
	extensions.push_back("nro");
	extensions.push_back("nsp");
	return extensions;
}

String EditorExportPlatformNX::get_export_option_warning(const EditorExportPreset *p_preset, const StringName &p_name) const {
	return "";
}

Ref<Texture2D> EditorExportPlatformNX::get_logo() const {
	return run_icon;
}

void EditorExportPlatformNX::get_platform_features(List<String> *r_features) const {
	r_features->push_back("nx");
}

Ref<Texture2D> EditorExportPlatformNX::get_run_icon() const {
	return run_icon;
}

void EditorExportPlatformNX::cleanup() {
	// no-op
}


EditorExportPlatformNX::EditorExportPlatformNX() {
	if (EditorNode::get_singleton()) {
		Ref<Image> img = memnew(Image);
		const bool upsample = !Math::is_equal_approx(Math::round(EDSCALE), EDSCALE);

		ImageLoaderSVG::create_image_from_string(img, _nx_logo_svg, EDSCALE, upsample, false);
		run_icon = ImageTexture::create_from_image(img);

		Ref<Theme> theme = EditorNode::get_singleton()->get_editor_theme();
		if (theme.is_valid()) {
			stop_icon = theme->get_icon(SNAME("Stop"), EditorStringName(EditorIcons));
		} else {
			stop_icon.instantiate();
		}
	}
}

EditorExportPlatformNX::~EditorExportPlatformNX()
{
	
}
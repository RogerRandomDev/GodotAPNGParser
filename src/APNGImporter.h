#pragma once

#include <godot_cpp/classes/editor_import_plugin.hpp>

class APNGImporter : public godot::EditorImportPlugin {
    GDCLASS(APNGImporter, godot::EditorImportPlugin);

protected:
    static void _bind_methods();

public:
    // Metadata Configuration
    virtual godot::String _get_importer_name() const override;
    virtual godot::String _get_visible_name() const override;
    virtual godot::PackedStringArray _get_recognized_extensions() const override;
    virtual godot::String _get_save_extension() const override;
    virtual godot::String _get_resource_type() const override;

    // UI Configuration
    virtual int32_t _get_preset_count() const override;
    virtual godot::String _get_preset_name(int32_t p_idx) const override;
    virtual godot::TypedArray<godot::Dictionary> _get_import_options(const godot::String &p_path, int32_t p_preset_index) const override;
    virtual bool _get_option_visibility(const godot::String &p_path, const godot::StringName &p_option_name, const godot::Dictionary &p_options) const override;

    // Main Import Execution
    virtual godot::Error _import(const godot::String &p_source_file, const godot::String &p_save_path, 
                                 const godot::Dictionary &p_options, 
                                 const godot::TypedArray<godot::String> &p_platform_variants, 
                                 const godot::TypedArray<godot::String> &p_gen_files) const override;
};
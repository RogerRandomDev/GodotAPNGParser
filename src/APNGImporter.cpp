#include "APNGImporter.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/sprite_frames.hpp> // Or whichever resource type you generate
#include "godotAPNG.h"

using namespace godot;

void APNGImporter::_bind_methods() {}

String APNGImporter::_get_importer_name() const { return "GodotAPNG"; }
String APNGImporter::_get_visible_name() const { return "APNG"; }

PackedStringArray APNGImporter::_get_recognized_extensions() const {
    PackedStringArray ext;
    ext.push_back("apng");
    return ext;
}

String APNGImporter::_get_save_extension() const { return "res"; } // Internal engine resource
String APNGImporter::_get_resource_type() const { return "SpriteFrames"; } 

int32_t APNGImporter::_get_preset_count() const { return 1; }
String APNGImporter::_get_preset_name(int32_t p_idx) const { return "Default"; }

TypedArray<Dictionary> APNGImporter::_get_import_options(const String &p_path, int32_t p_preset_index) const {
    TypedArray<Dictionary> options;
    Dictionary nameOption;
    nameOption["name"]="animation/name";
    nameOption["type"]=godot::Variant::STRING;
    nameOption["default_value"]="default";
    options.push_back(nameOption);
    Dictionary fpsOption;
    fpsOption["name"]="animation/fps";
    fpsOption["type"]=godot::Variant::INT;
    fpsOption["default_value"]=0;
    fpsOption["property_hint"]= godot::PROPERTY_HINT_RANGE;
    fpsOption["hint_string"]="0, 1, 1, or_greater";
    options.push_back(fpsOption);
    Dictionary compressionOption;
    compressionOption["name"]="compression/mode";
    compressionOption["type"]=godot::Variant::INT;
    compressionOption["default_value"]=5;
    compressionOption["property_hint"] = godot::PROPERTY_HINT_ENUM;
    compressionOption["hint_string"] = "S3TC,ETC,ETC2,BPTC,ASTC,NONE"; 
    options.push_back(compressionOption);
    Dictionary shareIdenticalOption;
    shareIdenticalOption["name"]="compression/shareIdenticalFrames";
    shareIdenticalOption["type"]=godot::Variant::BOOL;
    shareIdenticalOption["default_value"]=false;
    options.push_back(shareIdenticalOption);



    return options;
}

bool APNGImporter::_get_option_visibility(const String &p_path, const StringName &p_option_name, const Dictionary &p_options) const {
    return true;
}

Error APNGImporter::_import(const String &p_source_file, const String &p_save_path, 
                                const Dictionary &p_options, 
                                const TypedArray<String> &p_platform_variants, 
                                const TypedArray<String> &p_gen_files) const {
    
    // 1. Open and parse your custom raw file format
    bool file_exists = FileAccess::file_exists(p_source_file);
    if (not file_exists) {
        return ERR_FILE_NOT_FOUND;
    }
    int compressionValue = p_options["compression/mode"];
    godot::Image::CompressMode compression = static_cast<godot::Image::CompressMode>(compressionValue);
    String animationName = p_options["animation/name"];
    int fps = p_options["animation/fps"];
    bool shareIdentical = p_options["compression/shareIdenticalFrames"];

    Ref<SpriteFrames> custom_resource = GodotAPNGParser::APNGFileToSpriteFrames(p_source_file,fps,compression,shareIdentical,animationName);
    

    // 3. Save it to Godot's designated cache folder (.godot/imported/)
    String full_save_path = String(p_save_path) + "." + _get_save_extension();
    return ResourceSaver::get_singleton()->save(custom_resource, full_save_path);
}

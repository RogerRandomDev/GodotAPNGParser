// MyEditorPlugin.h
#pragma once
#include <godot_cpp/classes/editor_plugin.hpp>
#include "APNGImporter.h"

class APNGEditorPlugin : public godot::EditorPlugin {
    GDCLASS(APNGEditorPlugin, godot::EditorPlugin);

private:
    godot::Ref<APNGImporter> importer;

protected:
    static void _bind_methods() {}

public:
    void _enter_tree() override {
        importer.instantiate();
        add_import_plugin(importer);
    }

    void _exit_tree() override {
        remove_import_plugin(importer);
        importer.unref();
    }
};

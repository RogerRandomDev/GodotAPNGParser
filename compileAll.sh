#!/bin/bash

set -e

PROJECT_NAME="godot_apng"

GODOT_CPP="godot-cpp"
SOURCE_DIR="src"

OUTPUT_DIR="project/addons/GodotAnimatedPNGParser"
OUTPUT_ENGINE_DIR="addons/GodotAnimatedPNGParser"

GDEXTENSION_FILE="$OUTPUT_DIR/GodotAPNGParser.gdextension"

ENTRY_SYMBOL="godot_apng_library_init"


# ============================================================
# Build godot-cpp
# ============================================================

echo "========================================"
echo "Building godot-cpp"
echo "========================================"

pushd godot-cpp


# Linux
echo "Building godot-cpp: Linux debug"
scons target=template_debug platform=linux api_version=4.5

echo "Building godot-cpp: Linux release"
scons target=template_release platform=linux api_version=4.5


# Windows
echo "Building godot-cpp: Windows debug"
scons target=template_debug platform=windows api_version=4.5 use_mingw=yes

echo "Building godot-cpp: Windows release"
scons target=template_release platform=windows api_version=4.5 use_mingw=yes


popd

# ============================================================
# Build APNG extension
# ============================================================

echo "========================================"
echo "Building Godot APNG Parser"
echo "========================================"


# Linux
echo "Building APNG Parser: Linux debug"
scons target=template_debug platform=linux api_version=4.5

echo "Building APNG Parser: Linux release"
scons target=template_release platform=linux api_version=4.5


# Windows
echo "Building APNG Parser: Windows debug"
scons target=template_debug platform=windows api_version=4.5 use_mingw=yes

echo "Building APNG Parser: Windows release"
scons target=template_release platform=windows api_version=4.5 use_mingw=yes


# ============================================================
# Generate .gdextension
# ============================================================

echo "========================================"
echo "Generating .gdextension"
echo "========================================"

mkdir -p "$OUTPUT_DIR"

cat > "$GDEXTENSION_FILE" <<EOF
[configuration]

entry_symbol = "$ENTRY_SYMBOL"
compatibility_minimum = "4.5"
reloadable = true

[libraries]

linux.debug.x86_64 = "res://$OUTPUT_ENGINE_DIR/libGodotAPNGParser.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://$OUTPUT_ENGINE_DIR/libGodotAPNGParser.linux.template_release.x86_64.so"

windows.debug.x86_64 = "res://$OUTPUT_ENGINE_DIR/libGodotAPNGParser.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://$OUTPUT_ENGINE_DIR/libGodotAPNGParser.windows.template_release.x86_64.dll"
EOF

echo "Generated: $GDEXTENSION_FILE"

echo "========================================"
echo "Build complete!"
echo "========================================"

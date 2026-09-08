#ifndef GDAPNGPARSE_H
#define GDAPNGPARSE_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/sprite_frames.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/file_access.hpp>



namespace godot {

struct APNGAnimationFrame {
	Ref<Image> frame;
	float delay;
};

struct APNGAnimation {
	Vector2i size;
	Vector<APNGAnimationFrame> frames;
};

class GodotAPNGParser : public Object {
	GDCLASS(GodotAPNGParser, Object)
public:
	static Image::CompressMode compression;
	GodotAPNGParser();
	~GodotAPNGParser();
	static Ref<SpriteFrames> APNGToSpriteFrames(PackedByteArray buffer,int fps=-1);
	static Ref<SpriteFrames> APNGFileToSpriteFrames(String path,int fps = -1);
	static Image::CompressMode GetCompression();
	static void SetCompression(Image::CompressMode mode);
protected:
    static void _bind_methods();
private:
	static bool is_png(PackedByteArray buffer);
	static APNGAnimation parse_apng(PackedByteArray buffer);
	static APNGAnimationFrame _decode_frame(PackedByteArray buffer,int width,int height,int color_type);
	static int paeth(int a,int b,int c);
	static uint32_t chunk_name(PackedByteArray buffer,int pos);
	static int u16(PackedByteArray buffer,int pos);
	static int u32(PackedByteArray buffer,int pos);
};

}
#endif

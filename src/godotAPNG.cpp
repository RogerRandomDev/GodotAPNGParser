#include "godotAPNG.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


using namespace godot;

static const uint8_t PNG_SIGNATURE[8] = {
    0x89, 0x50, 0x4E, 0x47,
    0x0D, 0x0A, 0x1A, 0x0A
};

Image::CompressMode GodotAPNGParser::compression=Image::COMPRESS_MAX;

GodotAPNGParser::GodotAPNGParser() {
}

GodotAPNGParser::~GodotAPNGParser() {
}

void GodotAPNGParser::_bind_methods() {
    ClassDB::bind_static_method(
        "GodotAPNGParser",
        D_METHOD("apng_to_sprite_frames", "buffer","fps","compression_mode","animation_name"),
        &GodotAPNGParser::APNGToSpriteFrames,
        DEFVAL(-1),
        DEFVAL(5),
        DEFVAL("default")
    );
    ClassDB::bind_static_method(
        "GodotAPNGParser",
        D_METHOD("apng_file_to_sprite_frames", "path", "fps","compression_mode","animation_name"),
        &GodotAPNGParser::APNGFileToSpriteFrames,
        DEFVAL(-1),
        DEFVAL(5),
        DEFVAL("default")
    );

    ClassDB::bind_static_method(
        "GodotAPNGParser", 
        D_METHOD("get_compression"),
        &GodotAPNGParser::GetCompression
    );
    ClassDB::bind_static_method(
        "GodotAPNGParser", 
        D_METHOD("set_compression","mode"),
        &GodotAPNGParser::SetCompression
    );
}

Image::CompressMode GodotAPNGParser::GetCompression(){
    return GodotAPNGParser::compression;
}
void GodotAPNGParser::SetCompression(Image::CompressMode mode){
    GodotAPNGParser::compression=mode;
}


bool GodotAPNGParser::is_png(PackedByteArray buffer) {

    if (buffer.size() < 8) {
        return false;
    }

    for (int i = 0; i < 8; i++) {
        if (buffer[i] != PNG_SIGNATURE[i]) {
            return false;
        }
    }

    return true;
}


int GodotAPNGParser::u16(PackedByteArray buffer, int pos) {

    if (pos + 1 >= buffer.size()) {
        return 0;
    }

    return
        ((uint8_t)buffer[pos] << 8) |
        ((uint8_t)buffer[pos + 1]);
}


int GodotAPNGParser::u32(PackedByteArray buffer, int pos) {

    if (pos + 3 >= buffer.size()) {
        return 0;
    }

    return
        ((uint8_t)buffer[pos] << 24) |
        ((uint8_t)buffer[pos + 1] << 16) |
        ((uint8_t)buffer[pos + 2] << 8) |
        ((uint8_t)buffer[pos + 3]);
}


uint32_t GodotAPNGParser::chunk_name(PackedByteArray buffer, int pos) {

    if (pos + 3 >= buffer.size()) {
        return 0;
    }

    return
        ((uint32_t)(uint8_t)buffer[pos] << 24) |
        ((uint32_t)(uint8_t)buffer[pos + 1] << 16) |
        ((uint32_t)(uint8_t)buffer[pos + 2] << 8) |
        ((uint32_t)(uint8_t)buffer[pos + 3]);
}



int GodotAPNGParser::paeth(int a, int b, int c) {

    int p = a + b - c;

    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if (pa <= pb && pa <= pc) {
        return a;
    }

    if (pb <= pc) {
        return b;
    }

    return c;
}


static void append_u32(PackedByteArray &out, uint32_t value) {

    out.append((uint8_t)((value >> 24) & 0xFF));
    out.append((uint8_t)((value >> 16) & 0xFF));
    out.append((uint8_t)((value >> 8) & 0xFF));
    out.append((uint8_t)(value & 0xFF));
}


static uint32_t crc32(const uint8_t *data, int length) {

    uint32_t crc = 0xFFFFFFFF;

    for (int i = 0; i < length; i++) {

        crc ^= data[i];

        for (int j = 0; j < 8; j++) {

            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}


static void append_chunk(
    PackedByteArray &png,
    const char type[4],
    const PackedByteArray &data
) {

    append_u32(png, data.size());

    int crc_start = png.size();

    for (int i = 0; i < 4; i++) {
        png.append(type[i]);
    }

    for (int i = 0; i < data.size(); i++) {
        png.append(data[i]);
    }

    const uint8_t *crc_data =
        png.ptr() + crc_start;

    uint32_t crc =
        crc32(crc_data, 4 + data.size());

    append_u32(png, crc);
}



APNGAnimationFrame GodotAPNGParser::_decode_frame(
    PackedByteArray buffer,
    int width,
    int height,
    int color_type
) {

    APNGAnimationFrame result;

    result.delay = 0.0f;

    if (buffer.is_empty()) {
        return result;
    }

    Ref<Image> image;
    image.instantiate();

    Error err = image->load_png_from_buffer(buffer);

    if (err != OK || image.is_null()) {

        UtilityFunctions::printerr(
            "GodotAPNGParser: Failed to decode APNG frame PNG"
        );

        return result;
    }

    result.frame=image;

    return result;
}

Ref<Image> GodotAPNGParser::check_for_identical_frames(
    Ref<Image> image,
    Vector<APNGAnimationFrame> checkAgainst
) {
    Ref<Image> match_image;
    for(int i=0;i<checkAgainst.size();i++){
        if(checkAgainst.write[i].frame->get_data() == image->get_data()) return checkAgainst.write[i].frame;
    }
    return match_image;

}



APNGAnimation GodotAPNGParser::parse_apng(
    PackedByteArray buffer
) {

    APNGAnimation animation;

    animation.size = Vector2i(0, 0);

    if (!is_png(buffer)) {
        return animation;
    }



    int pos = 8;

    PackedByteArray ihdr;
    PackedByteArray idat;

    int bit_depth = 8;
    int color_type = 6;

    bool has_actl = false;



    struct FrameData {

        int width = 0;
        int height = 0;

        int x = 0;
        int y = 0;

        int delay_num = 0;
        int delay_den = 1000;

        int dispose_op = 0;
        int blend_op = 0;

        PackedByteArray compressed;
    };


    Vector<FrameData> frames;

    FrameData *current_frame = nullptr;


    while (pos + 8 <= buffer.size()) {

        uint32_t length = (uint32_t)u32(buffer, pos);

        if (length > (uint32_t)(buffer.size() - pos - 12)) {
            break;
        }

        uint32_t type = chunk_name(buffer, pos + 4);

        int data_pos = pos + 8;

        // IHDR
        if (type == 0x49484452) {

            if (length >= 13) {

                animation.size.x =
                    u32(buffer, data_pos);

                animation.size.y =
                    u32(buffer, data_pos + 4);

                bit_depth =
                    buffer[data_pos + 8];

                color_type =
                    buffer[data_pos + 9];

                ihdr.resize(13);

                for (int i = 0; i < 13; i++) {
                    ihdr[i] = buffer[data_pos + i];
                }
            }
        }

        // acTL
        else if (type == 0x6163544C) {

            has_actl = true;
        }

        // fcTL
        else if (type == 0x6663544C) {

            if (length >= 26) {

                FrameData frame;

                frame.width =
                    u32(buffer, data_pos + 4);

                frame.height =
                    u32(buffer, data_pos + 8);

                frame.x =
                    u32(buffer, data_pos + 12);

                frame.y =
                    u32(buffer, data_pos + 16);

                frame.delay_num =
                    u16(buffer, data_pos + 20);

                frame.delay_den =
                    u16(buffer, data_pos + 22);

                if (frame.delay_den == 0) {
                    frame.delay_den = 100;
                }

                frame.dispose_op =
                    buffer[data_pos + 24];

                frame.blend_op =
                    buffer[data_pos + 25];

                frames.push_back(frame);

                current_frame =
                    &frames.write[frames.size() - 1];
            }
        }

        // IDAT
        else if (type == 0x49444154) {

            if (current_frame != nullptr) {

                for (uint32_t i = 0; i < length; i++) {
                    current_frame->compressed.append(
                        buffer[data_pos + i]
                    );
                }

            } else {

                // Non-APNG normal PNG data.
                for (uint32_t i = 0; i < length; i++) {
                    idat.append(
                        buffer[data_pos + i]
                    );
                }
            }
        }

        // fdAT
        else if (type == 0x66644154) {

            if (current_frame != nullptr && length >= 4) {

                // fdAT has a 4-byte sequence number
                // before the actual IDAT data.

                for (uint32_t i = 4; i < length; i++) {

                    current_frame->compressed.append(
                        buffer[data_pos + i]
                    );
                }
            }
        }

        // IEND
        else if (type == 0x49454E44) {

            break;
        }

        pos += 12 + length;
    }



    if (!has_actl || frames.is_empty()) {

        // Treat the whole thing as a regular PNG if it wasnt an apng.

        APNGAnimationFrame frame =
            _decode_frame(
                buffer,
                animation.size.x,
                animation.size.y,
                color_type
            );

        frame.delay = 0.1f;

        animation.frames.push_back(frame);

        return animation;
    }

    // Persistent full-size APNG canvas.
    Ref<Image> canvas = Image::create(
        animation.size.x,
        animation.size.y,
        false,
        Image::FORMAT_RGBA8
    );

    if (canvas.is_null()) {
        UtilityFunctions::printerr(
            "GodotAPNGParser: Failed to create APNG canvas"
        );
        return animation;
    }

    canvas->fill(Color(0, 0, 0, 0));

    for (int i = 0; i < frames.size(); i++) {
        FrameData &frame = frames.write[i];

        // Validate frame rectangle.
        if (frame.width <= 0 ||
            frame.height <= 0 ||
            frame.x < 0 ||
            frame.y < 0 ||
            frame.x + frame.width > animation.size.x ||
            frame.y + frame.height > animation.size.y) {

            UtilityFunctions::printerr(
                "GodotAPNGParser: Invalid APNG frame rectangle"
            );
            continue;
        }

        /*
        * dispose_op == 2 means "PREVIOUS".
        *
        * Save the canvas BEFORE applying this frame so it can be
        * restored after the frame has been displayed.
        */
        Ref<Image> previous_canvas;

        if (frame.dispose_op == 2) {
            previous_canvas = canvas->duplicate();
        }

        // ------------------------------------------------------------
        // Build a normal PNG containing this APNG frame's compressed
        // image data so Godot can decode it.
        // ------------------------------------------------------------

        PackedByteArray png;

        for (int j = 0; j < 8; j++) {
            png.append(PNG_SIGNATURE[j]);
        }

        PackedByteArray frame_ihdr;

        append_u32(frame_ihdr, frame.width);
        append_u32(frame_ihdr, frame.height);

        frame_ihdr.append(bit_depth);
        frame_ihdr.append(color_type);

        // Compression method
        frame_ihdr.append(0);

        // Filter method
        frame_ihdr.append(0);

        // Interlace method
        frame_ihdr.append(0);

        append_chunk(
            png,
            "IHDR",
            frame_ihdr
        );

        // Copy palette/transparency information if needed.
        int chunk_pos = 8;

        while (chunk_pos + 8 <= buffer.size()) {
            uint32_t chunk_length =
                (uint32_t)u32(buffer, chunk_pos);

            if (chunk_length >
                (uint32_t)(buffer.size() - chunk_pos - 12)) {
                break;
            }

            uint32_t chunk_type =
                chunk_name(buffer, chunk_pos + 4);

            if (chunk_type == 0x504C5445) { // PLTE
                PackedByteArray data;

                for (uint32_t j = 0; j < chunk_length; j++) {
                    data.append(
                        buffer[chunk_pos + 8 + j]
                    );
                }

                append_chunk(
                    png,
                    "PLTE",
                    data
                );
            }
            else if (chunk_type == 0x74524E53) { // tRNS
                PackedByteArray data;

                for (uint32_t j = 0; j < chunk_length; j++) {
                    data.append(
                        buffer[chunk_pos + 8 + j]
                    );
                }

                append_chunk(
                    png,
                    "tRNS",
                    data
                );
            }

            chunk_pos += 12 + chunk_length;

            if (chunk_type == 0x49444154) { // IDAT
                break;
            }
        }

        // APNG frame's compressed image data.
        append_chunk(
            png,
            "IDAT",
            frame.compressed
        );

        PackedByteArray empty;
        append_chunk(
            png,
            "IEND",
            empty
        );

        // ------------------------------------------------------------
        // Decode the individual frame rectangle.
        // ------------------------------------------------------------

        APNGAnimationFrame decoded =
            _decode_frame(
                png,
                frame.width,
                frame.height,
                color_type
            );

        if (decoded.frame.is_null() ||
            decoded.frame->is_empty()) {

            UtilityFunctions::printerr(
                "GodotAPNGParser: Failed to decode APNG frame"
            );
            continue;
        }

        // Make sure the frame and canvas use the same format.
        if (decoded.frame->get_format() != Image::FORMAT_RGBA8) {
            decoded.frame->convert(Image::FORMAT_RGBA8);
        }

        Rect2i source_rect(
            0,
            0,
            frame.width,
            frame.height
        );

        Vector2i destination(
            frame.x,
            frame.y
        );

        // ------------------------------------------------------------
        // Composite the frame onto the persistent canvas.
        //
        // blend_op:
        //   0 = SOURCE
        //   1 = OVER
        // ------------------------------------------------------------

        if (frame.blend_op == 0) {
            // SOURCE:
            // Replace the pixels in this rectangle completely.
            canvas->blit_rect(
                decoded.frame,
                source_rect,
                destination
            );
        }
        else {
            // OVER:
            // Alpha-composite the frame over what is already there.
            canvas->blend_rect(
                decoded.frame,
                source_rect,
                destination
            );
        }

        // ------------------------------------------------------------
        // Store the COMPLETE canvas as this SpriteFrames frame.
        //
        // Important: duplicate it because the canvas continues
        // changing when the next APNG frame is processed.
        // ------------------------------------------------------------

        APNGAnimationFrame output;

        output.frame = canvas->duplicate();

        output.delay =
            (float)frame.delay_num /
            (float)frame.delay_den;

        if (output.delay <= 0.0f) {
            output.delay = 0.001f;
        }

        animation.frames.push_back(output);

        // ------------------------------------------------------------
        // Apply disposal AFTER storing/displaying the frame.
        // ------------------------------------------------------------

        if (frame.dispose_op == 1) {
            // BACKGROUND:
            // Clear this frame's rectangle to transparent.

            canvas->fill_rect(
                Rect2i(
                    frame.x,
                    frame.y,
                    frame.width,
                    frame.height
                ),
                Color(0, 0, 0, 0)
            );
        }
        else if (frame.dispose_op == 2) {
            // PREVIOUS:
            // Restore the canvas from before this frame.
            if (!previous_canvas.is_null()) {
                canvas = previous_canvas;
            }
        }
    }


    return animation;
}

Ref<SpriteFrames> GodotAPNGParser::APNGToSpriteFrames(
    PackedByteArray buffer,
    int fps,
    godot::Image::CompressMode compressionMode,
    bool shareIdentical,
    String animationName
) {
    if(compressionMode < 0) compressionMode=GodotAPNGParser::compression;

    Ref<SpriteFrames> sprite_frames;sprite_frames.instantiate();

    if (!is_png(buffer)) {

        UtilityFunctions::printerr(
            "GodotAPNGParser: Input is not a PNG"
        );

        return sprite_frames;
    }


    APNGAnimation animation =
        parse_apng(buffer);


    if (animation.frames.is_empty()) {

        UtilityFunctions::printerr(
            "GodotAPNGParser: No frames found"
        );

        return sprite_frames;
    }

    sprite_frames->remove_animation(StringName("default"));
    if (
        sprite_frames->has_animation(
            StringName(animationName)
        )
    ) {
        sprite_frames->remove_animation(
            StringName(animationName)
        );
    }


    sprite_frames->add_animation(
        StringName(animationName)
    );

    sprite_frames->set_animation_loop(
        StringName(animationName),
        true
    );
    

    float manualDelay=1.0;
    if(fps>0) manualDelay=(1.0/(float) fps);

    for (int i = 0; i < animation.frames.size(); i++) {

        APNGAnimationFrame &frame =
            animation.frames.write[i];
        
        if (frame.frame.is_null() || frame.frame->is_empty()) {
            continue;
        }
        //if it ever errors, we cancel out of this with what we have
        if(compressionMode!=Image::COMPRESS_MAX){
            godot::Error compression_err = frame.frame->compress(compressionMode);
            if(compression_err != godot::OK){
                godot::UtilityFunctions::push_error("APNG Importer: Selected compression failed.");
                return sprite_frames;
            }
        }

        // check for an identical frame. if we find one, use it instead.
        // helps save some memory but is slow, dont use if you aren't pre-compiling.
        if(shareIdentical){
            Ref<Image> matched_frame = check_for_identical_frames(
                frame.frame,animation.frames
            );
            if(!(matched_frame.is_null() || frame.frame->is_empty())) frame.frame=matched_frame;
        }
        
        Ref<ImageTexture> texture = 
            ImageTexture::create_from_image(
                frame.frame
            );


        if (texture.is_null()) {
            continue;
        }
        
        sprite_frames->add_frame(
            StringName(animationName),
            texture,
            fps==-1?frame.delay:manualDelay
        );


    }

    return sprite_frames;
}

Ref<SpriteFrames> GodotAPNGParser::APNGFileToSpriteFrames(
    String path,
    int fps,
    godot::Image::CompressMode compressionMode,
    bool shareIdentical,
    String animationName
) {
    Ref<SpriteFrames> empty;

    if (!FileAccess::file_exists(path)) {
        UtilityFunctions::printerr(
            "GodotAPNGParser: File does not exist: ",
            path
        );

        return empty;
    }

    Ref<FileAccess> file = FileAccess::open(
        path,
        FileAccess::READ
    );

    if (file.is_null()) {
        UtilityFunctions::printerr(
            "GodotAPNGParser: Failed to open file: ",
            path
        );

        return empty;
    }

    PackedByteArray buffer = file->get_buffer(
        file->get_length()
    );
    return APNGToSpriteFrames(buffer, fps, compressionMode,shareIdentical,animationName);
}
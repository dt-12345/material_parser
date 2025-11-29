#pragma once

#include "binary_file.h"
#include "math_types.h"

struct UserData {
    BinString* name;
    void* data;
    u32 count;
    u8 type; // S32, F32, String, Bytes
    u8 reserved[0x2b];
};
static_assert(sizeof(UserData) == 0x40);

struct ResBone {
    BinString* name;
    UserData* user_data_array;
    ResDic* user_data_dict;
    void* _18;
    u16 index;
    u16 parent_index;
    u16 smooth_bone_index;
    u16 rigid_bone_index;
    u16 billboard_index;
    u16 user_data_count;
    u32 options;
    Vector3f translate;
    Quatf rotation;
    Vector3f scale;
};
static_assert(sizeof(ResBone) == 0x58);

struct ResSkeleton {
    BinBlockSignature signature;
    u32 flags;
    ResDic* bone_dict;
    ResBone* bone_array;
    u16* bone_index_array;
    Matrix34f* inverse_transform_array;
    void* user_pointer;
    u16* mirrored_bone_index_array;
    u16 bone_count;
    u16 smooth_bone_count;
    u16 rigid_bone_count;
    u16 _3e;
};
static_assert(sizeof(ResSkeleton) == 0x40);

struct ResVertexAttribute {
    BinString* name;
    u32 format;
    u16 offset;
    u8 stream_index;
    u8 is_dynamic_vbo;
};
static_assert(sizeof(ResVertexAttribute) == 0x10);

struct ResBufferInfo {
    s32 stride;
    s32 divisor;
};
static_assert(sizeof(ResBufferInfo) == 0x8);

struct ResVertexBufferInfo : public ResBufferInfo {
    u32 _08;
    u32 _0c;
};
static_assert(sizeof(ResVertexBufferInfo) == 0x10);

struct ResVertex {
    BinBlockSignature signature;
    u32 flags;
    ResVertexAttribute* attribute_array;
    ResDic* attribute_dict;
    void* memory_pool;
    void* vertex_buffer;
    void** vertex_buffer_array;
    size_t* vertex_buffer_size_array;
    ResVertexBufferInfo* buffer_info_array;
    void* user_pointer;
    u32 base_offset;
    u8 attribute_count;
    u8 buffer_count;
    u16 index;
    u32 vertex_count;
    u16 _54;
    u16 buffer_alignment;
};
static_assert(sizeof(ResVertex) == 0x58);

struct ResSubMeshRange {
    u32 offset;
    u32 index_count;
};
static_assert(sizeof(ResSubMeshRange) == 0x8);

struct ResMesh {
    ResSubMeshRange* sub_mesh_range_array;
    void* memory_pool;
    void* buffer;
    ResBufferInfo* index_buffer_info_array;
    u32 offset;
    u32 primitive_topology;
    u32 index_format;
    u32 index_count;
    u32 base_index;
    u16 sub_mesh_count;
    u16 _36;
};
static_assert(sizeof(ResMesh) == 0x38);

struct ResKeyShape {
    u8 attribute_location_array[18];
    u8 _12[2];
};
static_assert(sizeof(ResKeyShape) == 0x14);

struct Bounding {
    Vector3f center;
    Vector3f extent;
};
static_assert(sizeof(Bounding) == 0x18);

struct Sphere {
    Vector3f center;
    Vector3f extent;
    f32 radius;
    u32 _1c;
};
static_assert(sizeof(Sphere) == 0x20);

struct ResShape {
    BinBlockSignature signature;
    u32 flags;
    BinString* name;
    ResVertex* vertex;
    ResMesh* mesh;
    u16* skin_bone_index_array;
    ResKeyShape* key_shape_array;
    ResDic* key_shape_dict;
    Bounding* bounding_box_array;
    Sphere* bounding_sphere_array;
    void* user_pointer;
    u16 index;
    u16 material_index;
    u16 bone_index;
    u16 vertex_index;
    u16 skin_bone_index_count;
    u8 vertex_skin_weight;
    u8 mesh_count;
    u8 key_shape_count;
    u8 attribute_count;
    u16 _5e;
};
static_assert(sizeof(ResShape) == 0x60);

struct ResRenderInfo {
    BinString* name; // same as the dict key
    u8 type; // s32, f32, string
};
static_assert(sizeof(ResRenderInfo) == 0x10);

struct ResShaderParam {
    void* runtime_convert_tex_callback;
    BinString* name;
    u16 offset;
    u8 type;
};
static_assert(sizeof(ResShaderParam) == 0x18);

struct ResShaderReflection {
    BinString* archive_name;
    BinString* shading_model_name;
    ResRenderInfo* render_info_array;
    ResDic* render_info_dict;
    ResShaderParam* shader_param_array;
    ResDic* shader_param_dict;
    ResDic* vertex_attribute_dict;
    ResDic* sampler_dict;
    ResDic* static_option_dict;
    u16 render_info_count;
    u16 shader_param_count;
    u16 shader_param_data_size;
    u16 _4e;
    u16 _50;
};

struct ResShaderData {
    ResShaderReflection* shader_reflection;
    BinString** vertex_attribute_name_array;
    u8* vertex_attribute_index_array; // index into the ResShaderReflection dictionaries
    BinString** sampler_name_array;
    u8* sampler_index_array;
    u64* static_option_bool_value_array; // bit array of the values
    BinString** static_option_string_array;
    u16* static_option_index_array;
    u32 _40;
    u8 vertex_attribute_count;
    u8 sampler_count;
    u16 bool_static_option_count;
    u16 total_static_option_count;
    u16 _4a;
    u32 _4c;
};
static_assert(sizeof(ResShaderData) == 0x50);

struct ResMaterial {
    BinBlockSignature signature;
    u32 flags;
    BinString* name;
    ResShaderData* shader_data;
    void** texture_view_array;
    BinString** texture_name_array;
    void* sampler_array;
    void* sampler_info_array;
    ResDic* sampler_dict;
    void* render_info_value_array;
    u16* render_info_value_count_array;
    u16* render_info_value_offset_array;
    void* shader_param_value_array;
    u32 shader_param_ubo_offset_array;
    void* _68;
    UserData* user_data_array;
    ResDic* user_data_dict;
    u32* shader_param_convert_flag_array;
    void* user_pointer;
    u64* sampler_descriptor_array;
    u64* texture_descriptor_array;
    u16 index;
    u8 sampler_count;
    u8 texture_count;
    u16 _a4;
    u16 user_data_count;
    u16 _a8;
    u16 shading_model_ubo_size;
    u32 _ac;
};
static_assert(sizeof(ResMaterial) == 0xb0);

struct ResModel {
    BinBlockSignature signature;
    u32 flags;
    BinString* name;
    BinString* _10;
    ResSkeleton* skeleton;
    ResVertex* vertex_array;
    ResShape* shape_array;
    ResDic* shape_dict;
    ResMaterial* material_array;
    ResDic* material_dict;
    ResShaderReflection* shader_reflection_array;
    UserData* user_data_array;
    ResDic* user_data_dict;
    void* user_pointer;
    u16 vertex_count;
    u16 shape_count;
    u16 material_count;
    u16 shader_reflection_count;
    u16 user_data_count;
    u16 _72;
    u32 _74;
};
static_assert(sizeof(ResModel) == 0x78);

struct ResShaderParamAnim {
    BinString* name;
    u16 base_curve;
    u16 float_curve_count;
    u16 int_curve_count;
    u16 base_constant;
    u16 constant_count;
    u16 sub_shader_param_index;
};
static_assert(sizeof(ResShaderParamAnim) == 0x18);

struct ResTexturePatternAnim {
    BinString* tex_name;
    u16 base_curve;
    u16 base_constant;
    u8 sub_sampler_index;
    u8 _0d;
    u8 _0e;
    u8 _0f;
};
static_assert(sizeof(ResTexturePatternAnim) == 0x10);

struct ResAnimCurve {
    void* frame_array;
    void* value_array;
    u16 packed_types;
    u16 frame_count;
    u32 base_result_offset;
    f32 start_key;
    f32 end_key;
    f32 frame_data_scale;
    f32 frame_data_add;
    f32 frame_delta;
    u32 _2c;
};
static_assert(sizeof(ResAnimCurve) == 0x30);

struct ResPerMaterialAnim {
    BinString* model_name;
    ResShaderParamAnim* shader_param_anim_array;
    ResTexturePatternAnim* texture_pattern_anim_array;
    ResAnimCurve* anim_curve_array;
    void* constant_array;
    u16 base_shader_param_curve;
    u16 base_texture_pattern_curve;
    u16 base_visibility_curve_index;
    u16 base_visibility_constant_index;
    u16 visibility_constant_index;
    u16 shader_param_anim_count;
    u16 texture_pattern_anim_count;
    u16 constant_count;
    u16 anim_curve_count;
    u16 _3a;
    u16 _3c;
    u16 _3e;
};
static_assert(sizeof(ResPerMaterialAnim) == 0x40);

struct ResMaterialAnim {
    BinBlockSignature signature;
    u32 flags;
    BinString* name;
    BinString* _10;
    ResModel* model;
    u16* bind_table;
    ResPerMaterialAnim* material_anim_data_array;
    void** texture_view_array;
    BinString** texture_name_array;
    UserData* user_data_array;
    ResDic* user_data_dict;
    u64* texture_descriptor_array;
    u32 frame_count;
    u32 baked_size;
    u16 user_data_count;
    u16 per_material_anim_count;
    u16 total_curve_count;
    u16 shader_param_anim_count;
    u16 texture_pattern_anim_count;
    u16 material_visibility_anim_count;
    u16 texture_count;
    u16 _6e;
};
static_assert(sizeof(ResMaterialAnim) == 0x70);

struct ResFile : public BinaryFileHeader {
    BinString* filename;
    ResModel* models;
    ResDic* model_dict;
    void* _38;
    void* _40;
    void* _48;
    void* _50;
    void* skeletal_anims;
    ResDic* skeletal_anim_dict;
    ResMaterialAnim* material_anims;
    ResDic* material_anim_dict;
    void* bone_vis_anims;
    ResDic* bone_vis_anim_dict;
    void* shape_anims;
    ResDic* shape_anim_dict;
    void* scene_anims;
    ResDic* scene_anim_dict;
    void* mem_pool;
    void* mem_pool_info;
    union {
        void* embedded_files;
        u64* external_string_keys;
    };
    union {
        ResDic* embedded_file_dict;
        ResDic* external_strings;
    };
    void* _c8;
    BinString* default_string; // for ExternalBinaryString.bfres, used as the fallback if no match found
    u32 _d8;
    u16 model_count;
    u16 _de;
    u16 _e0;
    u16 skeletal_anim_count;
    u16 material_anim_count;
    u16 bone_vis_anim_count;
    u16 shape_anim_count;
    u16 scene_anim_count;
    u16 embedded_file_count;
    u8 options;

    static ResFile* ResCast(void* ptr) {
        ResFile* file = reinterpret_cast<ResFile*>(ptr);
        if (!file->IsRelocated()) {
            auto reloc_table = file->GetRelocationTable();
            reloc_table->Relocate();
        }
        return file;
    }

    bool IsRelocatedExternalStrings() const {
        return (options >> 1 & 1) == 0;
    }

    bool ContainsExternalStrings() const {
        return options >> 2 & 1;
    }

    void SetRelocatedExternalStrings(bool relocated) {
        options = options & (0xff ^ (relocated << 1));
    }
    
    // only for use if the file contains external shader strings
    size_t FindExternalStringIndex(u64 key) const;

    const BinString* RelocateExternalString(BinString*& out_string) const;

    void RelocateExternalStrings(const ResFile* external_strings);
};
static_assert(sizeof(ResFile) == 0xf0);
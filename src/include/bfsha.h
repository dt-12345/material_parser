#pragma once

#include "binary_file.h"

namespace gfx {

// nn::gfx::ResShaderFile -> bnsh
struct ResShaderFile : public BinaryFileHeader {
    static ResShaderFile* ResCast(void* ptr) {
        ResShaderFile* file = reinterpret_cast<ResShaderFile*>(ptr);
        if (!file->IsRelocated()) {
            auto reloc_table = file->GetRelocationTable();
            reloc_table->Relocate();
        }
        return file;
    }
};

} // namespace gfx

namespace g3d2 {

struct ResShaderArchive;
struct ResShadingModel;

struct ResShaderOption {
    BinString* name;
    ResDic* choice_dict;
    u32* choice_array;
    u16 choice_count;
    u16 default_choice;
    u16 _1c;
    u8 use_block_buffer;
    u8 dynamic_index_offset;
    u32 option_mask;
    u8 option_index;
    u8 bit_offset;
};
static_assert(sizeof(ResShaderOption) == 0x28);

struct ResVertexAttributeLocation {
    u8 location;
    u8 index;
};
static_assert(sizeof(ResVertexAttributeLocation) == 0x2);

struct ResShaderProgram {
    u32* sampler_interface_slots;
    u32* image_interface_slots;
    u32* ubo_interface_slots;
    u32* ssbo_interface_slots;
    void* _20;
    ResShadingModel* parent_model;
    u32 _30;
    u16 init_mask;
    u8 _36[10];
};
static_assert(sizeof(ResShaderProgram) == 0x40);

struct ResShadingModel {
    BinString* name;
    ResShaderOption* static_option_array;
    ResDic* static_option_dict;
    ResShaderOption* dynamic_option_array;
    ResDic* dynamic_option_dict;
    ResVertexAttributeLocation* vertex_attribute_array;
    ResDic* vertex_attribute_dict;
    void* sampler_location_array;
    ResDic* sampler_dict;
    void* _48;
    ResDic* _50;
    void* uniform_block_array;
    ResDic* uniform_block_dict;
    void* _68;
    void* shader_storage_block_array;
    ResDic* shader_storage_block_dict;
    void* _80;
    ResShaderProgram* program_array;
    u32* key_table;
    ResShaderArchive* parent_archive;
    void* interfaces;
    ::gfx::ResShaderFile* shader; // bnsh
    void* runtime_data_pointer;
    void* _b8;
    void* _c0;
    void* runtime_user_pointer;
    void* _d0;
    void* _d8;
    s32 default_key_index;
    u16 static_option_count;
    u16 dynamic_option_count;
    u16 shader_program_count;
    u8 static_key_count;
    u8 dynamic_key_count;
    u8 vertex_attribute_location_count;
    u8 sampler_location_count;
    u8 image_location_count;
    u8 ubo_location_count;
    u8 material_uniform_location_count;
    u8 _f1;
    u8 _f2;
    u8 uniform_location_count;
    u8 shader_storage_block_location_count;
    u8 _f5;
    u8 _f6;
    u8 _f7;
    u8 option_uniform_location_index;
    s8 vertex_stage_base_location_index;
    s8 geometry_stage_base_location_index;
    s8 fragment_stage_base_location_index;
    s8 compute_stage_base_location_index;
    s8 hull_stage_base_location_index;
    s8 domain_stage_base_location_index;
    s8 slots_per_location;
};
static_assert(sizeof(ResShadingModel) == 0x100);

struct ResShaderArchive {
    BinString* name;
    BinString* _08;
    ResShadingModel* shading_model_array;
    ResDic* shading_model_dict;
    void* user_pointer;
    void* initialize_callback;
    void* work_memory;
    u16 shader_container_count;
    u16 flags;
    u32 _3c;
    u16 shading_model_count;
    u16 _42;
};
static_assert(sizeof(ResShaderArchive) == 0x48);

// nn::g3d2::ResShaderFile -> bfsha
struct ResShaderFile : public BinaryFileHeader {
    ResShaderArchive* archive;
    StringPool* string_pool;
    u64 string_pool_size;

    static ResShaderFile* ResCast(void* ptr) {
        ResShaderFile* file = reinterpret_cast<ResShaderFile*>(ptr);
        if (!file->IsRelocated()) {
            auto reloc_table = file->GetRelocationTable();
            reloc_table->Relocate();
        }

        for (size_t i = 0; i < file->archive->shading_model_count; ++i)
            ::gfx::ResShaderFile::ResCast(file->archive->shading_model_array[i].shader);
        
        return file;
    }
};
static_assert(sizeof(ResShaderFile) == 0x38);

} // namespace g3d2
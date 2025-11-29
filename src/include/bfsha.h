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

enum ShaderStage {
    ShaderStage_Vertex,
    ShaderStage_Hull,
    ShaderStage_Domain,
    ShaderStage_Geometry,
    ShaderStage_Pixel,
    ShaderStage_Compute,
    ShaderStage_End,
};

enum ShaderInterfaceType {
    ShaderInterfaceType_Input,
    ShaderInterfaceType_Output,
    ShaderInterfaceType_Sampler,
    ShaderInterfaceType_ConstantBuffer,
    ShaderInterfaceType_UnorderedAccessBuffer,
    ShaderInterfaceType_Image,
    ShaderInterfaceType_SeparateTexture,
    ShaderInterfaceType_SeparateSampler,
    ShaderInterfaceType_End,
};

enum ShaderCodeType {
    ShaderCodeType_Binary,
    ShaderCodeType_Ir,
    ShaderCodeType_Source,
    ShaderCodeType_SourceArray,
    ShaderCodeType_End,
};

struct ResShaderInterfaceInfo {
    ResDic* input_attr_dict;
    ResDic* output_attr_dict;
    ResDic* sampler_dict;
    ResDic* uniform_dict;
    ResDic* storage_block_dict;
    s32 output_base_index;
    s32 sampler_base_index;
    s32 uniform_base_index;
    s32 storage_block_base_index;
    s32* slots;
    s32 work_group_x;
    s32 work_group_y;
    s32 work_group_z;
    s32 image_base_index;
    ResDic* image_dict;
    ResDic** separate_dicts;
    s32 separate_tex_base_index;
    s32 separate_sampler_base_index;

    const ResDic* GetResDic(ShaderInterfaceType ifc_type) const {
        switch (ifc_type) {
            case ShaderInterfaceType_Input: return input_attr_dict;
            case ShaderInterfaceType_Output: return output_attr_dict;
            case ShaderInterfaceType_Sampler: return sampler_dict;
            case ShaderInterfaceType_ConstantBuffer: return uniform_dict;
            case ShaderInterfaceType_UnorderedAccessBuffer: return storage_block_dict;
            case ShaderInterfaceType_Image: return image_dict;
            case ShaderInterfaceType_SeparateTexture:
            case ShaderInterfaceType_SeparateSampler:
                if (separate_dicts == nullptr) {
                    return nullptr;
                } else {
                    return separate_dicts[ifc_type - ShaderInterfaceType_SeparateTexture];
                }
            default: return nullptr;
        }
    }

    s32 GetBaseIndex(ShaderInterfaceType ifc_type) const {
        switch (ifc_type) {
            case ShaderInterfaceType_Input: return 0;
            case ShaderInterfaceType_Output: return output_base_index;
            case ShaderInterfaceType_Sampler: return sampler_base_index;
            case ShaderInterfaceType_ConstantBuffer: return uniform_base_index;
            case ShaderInterfaceType_UnorderedAccessBuffer: return storage_block_base_index;
            case ShaderInterfaceType_Image: return image_base_index;
            case ShaderInterfaceType_SeparateTexture: return separate_tex_base_index;
            case ShaderInterfaceType_SeparateSampler: return separate_sampler_base_index;
            default: return -1;
        }
    }

    s32 GetInterfaceSlot(ShaderInterfaceType ifc_type, u32 index) const {
        const ResDic* dic = GetResDic(ifc_type);
        if (dic == nullptr || index >= dic->node_count) {
            return -1;
        }
        return slots[index + GetBaseIndex(ifc_type)];
    }

};
static_assert(sizeof(ResShaderInterfaceInfo) == 0x68);

struct ResShaderInterfaceSlotTable {
    ResShaderInterfaceInfo* stages[ShaderStage_End];
    char _30[0x10];
};
static_assert(sizeof(ResShaderInterfaceSlotTable) == 0x40);

struct ResShaderCode {
    void* runtime_buffer;
    u8* control;
    u8* code;
    u32 code_size;
    u32 control_size;
    void* extra_info;
    u32 extra_scratch_mem_size;
    u32 scaled_scratch_mem_size;
    void* ext_data;
    u64 _38;
};
static_assert(sizeof(ResShaderCode) == 0x40);

struct ResShaderVariation;

struct ResShaderProgram {
    u8 option;
    u8 shader_code_type;
    u16 _02;
    u32 _04;
    ResShaderCode* shader_code_ptrs[ShaderStage_End];
    uintptr_t _38;
    uintptr_t _40;
    uintptr_t _48;
    uintptr_t _50;
    uintptr_t _58;
    u32 object_size;
    u32 _64;
    void* runtime_shader;
    ResShaderVariation* parent_variation;
    ResShaderInterfaceSlotTable* interfaces;
    u64 _80;
    u64 _88;
    u64 _90;
    u64 _98;
};
static_assert(sizeof(ResShaderProgram) == 0xa0);

struct ResShaderContainer;

struct ResShaderVariation {
    ResShaderProgram* source_code;
    ResShaderProgram* ir;
    ResShaderProgram* binary;
    ResShaderContainer* parent_container;
    uintptr_t _20;
    uintptr_t _28;
    uintptr_t _30;
    uintptr_t _38;
};
static_assert(sizeof(ResShaderVariation) == 0x40);

// first block of file
struct ResShaderContainer : public BinaryBlockHeader {
    u16 _10;
    u8 _12;
    u8 _13;
    u8 _14;
    u8 _15;
    u8 _16;
    u8 _17;
    u32 _18;
    u32 variation_count;
    ResShaderVariation* variations;
    void* memory_info;
};
static_assert(sizeof(ResShaderContainer) == 0x30);

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
    u8 index;
    s8 location;
};
static_assert(sizeof(ResVertexAttributeLocation) == 0x2);

struct ResShaderProgram {
    s32* sampler_interface_slots;
    s32* image_interface_slots;
    s32* ubo_interface_slots;
    s32* ssbo_interface_slots;
    ::gfx::ResShaderVariation* variation;
    ResShadingModel* parent_model;
    u32 _30;
    u16 init_mask;
    u8 _36[10];
};
static_assert(sizeof(ResShaderProgram) == 0x40);

struct ResInterfaceInfo {
    BinString** sampler_locations; 
    BinString** image_locations; 
    BinString** ubo_locations; 
    BinString** ssbo_locations; 
};

struct ResMember {
    BinString* annotation;
    u32 index;
    u16 offset;
    u16 block_index;
};

enum BlockType : u8 {
    None, Material, Shape, Skeleton, Option,
};

struct ResBufferObject {
    ResMember* members;
    ResDic* member_dict;
    u8* default_value;
    u8 index;
    BlockType type;
    u16 size;
    u32 member_count;
};

struct ResSampler {
    BinString* annotation;
    u8 index;
    u8 padding[7];
};

struct ResShadingModel {
    BinString* name;

    ResShaderOption* static_option_array;
    ResDic* static_option_dict;

    ResShaderOption* dynamic_option_array;
    ResDic* dynamic_option_dict;

    ResVertexAttributeLocation* vertex_attribute_array;
    ResDic* vertex_attribute_dict;

    ResSampler* sampler_array;
    ResDic* sampler_dict;

    // seemingly unused, but probably images, right? there's nothing else left
    void* _48;
    ResDic* _50;

    ResBufferObject* uniform_block_array;
    ResDic* uniform_block_dict;
    ResMember* uniform_array;

    ResBufferObject* shader_storage_block_array;
    ResDic* shader_storage_block_dict;
    ResMember* shader_storage_block_member_array;

    ResShaderProgram* program_array;
    u32* key_table;
    ResShaderArchive* parent_archive;
    ResInterfaceInfo* interfaces;

    // BNSH
    ::gfx::ResShaderFile* shader;

    // user data fields filled in at runtime
    void* runtime_mutex_pointer; // nn::os::Mutex*
    void* _b8;
    void* _c0;
    void* runtime_user_pointer; // gsys::G3dResShadingModelEx*
    void* _d0;

    u32 uniform_count;
    u32 shader_storage_block_member_count;

    s32 default_key_index;
    u16 static_option_count;
    u16 dynamic_option_count;
    u16 shader_program_count;

    // # of u32s used up for each type
    u8 static_key_count;
    u8 dynamic_key_count;

    // corresponds to vertex attribute dict/array
    u8 vertex_attribute_count;
    // corresponds to sampler dict/array
    u8 sampler_count;
    // corresponds to image dict/array presumably?
    u8 image_count;
    u8 uniform_block_count;

    s8 material_uniform_index;
    s8 shape_uniform_index;
    s8 skeleton_uniform_index;
    s8 option_uniform_index;

    // corresponds to ssbo dict/array
    u8 shader_storage_block_count;

    // ???
    s8 _f5;
    s8 _f6;
    s8 _f7;
    s8 _f8;

    // base indices for interface slots
    s8 vertex_stage_base_location_index;
    s8 geometry_stage_base_location_index;
    s8 fragment_stage_base_location_index;
    s8 compute_stage_base_location_index;
    s8 hull_stage_base_location_index;
    s8 domain_stage_base_location_index;

    s8 shader_stage_count;
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
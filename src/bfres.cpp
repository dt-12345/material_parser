#include "bfres.h"

size_t ResFile::FindExternalStringIndex(u64 key) const {
    if (external_strings->node_count == 0)
        return 0;

    u64 max = external_strings->node_count;
    size_t index = 0;

    while (max) {
        const u64 mid = max >> 1;
        if (external_string_keys[mid + index] < key) {
            index += (mid + 1);
            max -= (mid + 1);
        } else {
            max = mid;
        }
    }

    return index;
}

const BinString* ResFile::RelocateExternalString(BinString*& out_string) const {
    const u64 key = reinterpret_cast<u64>(out_string);
    const size_t index = FindExternalStringIndex(key);
    if (external_string_keys[index] != key)
        out_string = default_string;
    else
        out_string = external_strings->entries[index + 1].key;

    return out_string;
}

void ResFile::RelocateExternalStrings(const ResFile* ext_strings) {
    for (u32 i = 0; i < model_count; ++i) {
        for (u32 j = 0; j < models[i].shader_reflection_count; ++j) {
            const auto& reflection = models[i].shader_reflection_array[j];

            if (reflection.static_option_dict && reflection.static_option_dict->entries[0].key == nullptr) {
                reflection.static_option_dict->entries[0].key = ext_strings->default_string;
                if (reflection.static_option_dict->node_count > 0) {
                    for (u32 k = 0; k < reflection.static_option_dict->node_count; ++k)
                        ext_strings->RelocateExternalString(reflection.static_option_dict->entries[k + 1].key);
                }
            }

            if (reflection.render_info_count != 0) {
                const BinString* root_key = reflection.render_info_dict->entries[0].key;
                if (root_key == nullptr)
                    reflection.render_info_dict->entries[0].key = ext_strings->default_string;
                for (u32 k = 0; k < reflection.render_info_count; ++k) {
                    const BinString* string = ext_strings->RelocateExternalString(reflection.render_info_array[k].name);
                    if (root_key == nullptr)
                        reflection.render_info_dict->entries[k + 1].key = const_cast<BinString*>(string);
                }
            }

            if (reflection.shader_param_count != 0) {
                const BinString* root_key = reflection.shader_param_dict->entries[0].key;
                if (root_key == nullptr)
                    reflection.shader_param_dict->entries[0].key = ext_strings->default_string;
                for (u32 k = 0; k < reflection.shader_param_count; ++k) {
                    const BinString* string = ext_strings->RelocateExternalString(reflection.shader_param_array[k].name);
                    if (root_key == nullptr)
                        reflection.shader_param_dict->entries[k + 1].key = const_cast<BinString*>(string);
                }
            }
        }
    }

    for (u32 i = 0; i < material_anim_count; ++i) {
        for (u32 j = 0; j < material_anims[i].per_material_anim_count; ++j) {
            const auto& anim = material_anims[i].material_anim_data_array[j];
            for (u32 k = 0; k < anim.shader_param_anim_count; ++k) {
                ext_strings->RelocateExternalString(anim.shader_param_anim_array[k].name);
            }
        }
    }

    SetRelocatedExternalStrings(true);
}
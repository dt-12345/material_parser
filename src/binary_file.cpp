#include "binary_file.h"

void RelocationTable::Relocate() {
    uintptr_t base = reinterpret_cast<uintptr_t>(this) - this_offset;

    for (s32 i = 0; i < section_count; ++i) {
        const auto section = GetSection(i);
        const size_t offset = section->ptr != nullptr ? section->offset - section->position : base;

        for (s32 j = 0; j < section->entry_count; ++j) {
            const auto entry = GetEntry(section->base_entry_index + j);
            uintptr_t* dest = reinterpret_cast<uintptr_t*>(base + entry->position);
            for (u32 k = 0; k < entry->array_count; ++k) {
                for (u8 l = 0; l < entry->relocation_count; ++l) {
                    if (*dest != 0)
                        *dest += offset;
                    ++dest;
                }
                dest += entry->stride;
            }
        }
    }

    reinterpret_cast<BinaryFileHeader*>(base)->SetRelocated(true);
}
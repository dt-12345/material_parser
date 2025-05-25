#pragma once

#include "types.h"

#include <iterator>
#include <string_view>

struct BinaryBlockHeader;
struct RelocationTable;

struct BinString {
    u16 length;
    char data[2];
    
    BinString(const BinString&) = delete;
    auto operator=(const BinString&) = delete;

    int GetLength() const { return static_cast<int>(length); }
    const char* GetData() const { return data; }

    std::string_view Get() const { return {data, length}; }
    operator std::string_view() const { return Get(); }
};

union BinFileSignature {
    char _magic[8];
    u64 _packed;
};

union BinBlockSignature {
    char _magic[4];
    u32 _packed;
};

struct BinVersion {
    u8 micro;
    u8 minor;
    u16 major;
};

struct BinaryFileHeader {
    BinFileSignature signature;
    BinVersion version;
    u16 bom;
    u8 alignment_shift;
    u8 address_size;
    u32 filename_offset;
    u16 flags;
    u16 first_block_offset;
    u32 rel_table_offset;
    u32 file_size;

    bool IsRelocated() const {
        return flags & 1;
    }

    void SetRelocated(bool is_relocated) {
        flags = (flags & 0xfffe) | is_relocated;
    }

    bool IsSignatureValid(u64 sig) const {
        return signature._packed == sig;
    }

    bool IsVersionValid(int major, int minor, int micro) const {
        return version.major == major && version.minor == minor && version.micro == micro;
    }

    bool IsValid(u64 sig, int major, int minor, int micro) const {
        return IsSignatureValid(sig) && IsVersionValid(major, minor, micro);
    }

    size_t GetAlignment() const {
        return 1 << alignment_shift;
    }

    BinaryBlockHeader* GetFirstBlock() const {
        return reinterpret_cast<BinaryBlockHeader*>(reinterpret_cast<uintptr_t>(this) + first_block_offset);
    }

    RelocationTable* GetRelocationTable() const {
        return reinterpret_cast<RelocationTable*>(reinterpret_cast<uintptr_t>(this) + rel_table_offset);
    }
};

struct BinaryBlockHeader {
    BinBlockSignature signature;
    u32 next_block_offset;
    u32 block_size;
    u32 reserved;
};

struct RelocationTable {
    
    struct Section {
        union {
            u64 offset;
            void* ptr;
        };
        u32 position;
        u32 size;
        s32 base_entry_index;
        s32 entry_count;
    };

    struct Entry {
        u32 position;
        u16 array_count;
        u8 relocation_count;
        u8 stride;
    };
    
    BinBlockSignature signature;
    u32 this_offset; // offset of relocation table from start of file
    s32 section_count;
    Section sections[1];

    void Relocate();

    const Section* GetSection(int index) const {
        // no bounds check
        return &sections[index];
    }

    const Entry* GetEntry(int index) const {
        // no bounds check
        return reinterpret_cast<const Entry*>(&sections[section_count]) + index;
    }
};

struct ResDic {
    struct Entry {
        s32 ref_bit;
        u16 children[2];
        BinString* key;
    };

    BinBlockSignature signature;
    u32 node_count;
    Entry entries[1];

    // https://github.com/open-ead/nnheaders/blob/master/include/nn/util/util_ResDic.h
    int FindIndex(const std::string_view& key) const {
        const Entry* entry = FindImpl(key);
        return entry->key->Get() == key ? static_cast<int>(std::distance(&entries[1], entry)) : -1;
    }

private:
    static int ExtractRefBit(const std::string_view& key, int ref_bit) {
        int char_index = ref_bit >> 3;
        if (static_cast<size_t>(char_index) < key.length()) {
            int bit_index = ref_bit & 7;
            return (key[key.length() - char_index - 1] >> bit_index) & 1;
        }
        return 0;
    }

    static bool Older(const Entry* parent, const Entry* child) {
        return parent->ref_bit < child->ref_bit;
    }

    Entry* GetChild(const Entry* parent, const std::string_view& key) {
        int bit = ExtractRefBit(key, parent->ref_bit);
        int index = parent->children[bit];
        return &entries[index];
    }

    const Entry* GetChild(const Entry* parent, const std::string_view& key) const {
        int bit = ExtractRefBit(key, parent->ref_bit);
        int index = parent->children[bit];
        return &entries[index];
    }

    const Entry* FindImpl(const std::string_view& key) const {
        const Entry* parent = &entries[0];
        const Entry* child = &entries[parent->children[0]];

        while (Older(parent, child)) {
            parent = child;
            child = GetChild(parent, key);
        }

        return child;
    }
};

struct StringPool : public BinaryBlockHeader {
    u32 string_count;
    u32 _14;
};
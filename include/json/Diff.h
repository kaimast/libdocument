#pragma once

#include "defines.h"

namespace json
{

enum class DiffType : uint8_t
{
    Modified,
    Deleted,
    Added,
};

class Diff
{
public:
    Diff(DiffType type, const std::string& path, const uint8_t* value, uint32_t length);
    Diff(const Diff &other) = delete;

    const BitStream& content() const
    {
        return m_content;
    }

    json::Document as_document() const;

    void compress(BitStream &bstream, bool write_size = true) const;

private:
    BitStream m_content;
};

}

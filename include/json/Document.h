#pragma once

#include "defines.h"

#include "json/BitStream.h"
#include "json/Diff.h"
#include "json/Iterator.h"

#ifdef USE_GEO
#include "geo/vector2.h"
#endif

namespace json
{

class Document
{
public:
    Document(Document &&other) noexcept;

    /// Filter parent by a set of paths
    Document(const Document &parent, const std::vector<std::string> &paths, bool force = false);

    /// Create a view on a subset of the document
    Document(const Document &parent, const uint32_t pos);
    Document(const Document &parent, const std::string &path, bool force = false);

    explicit Document(BitStream &data);
    explicit Document(const std::string& str);

    Document(const uint8_t *data, uint32_t length, DocumentMode mode);
    Document(uint8_t *data, uint32_t length, DocumentMode mode);

    ~Document() = default;

    void assign(BitStream &&data)
    {
        m_content = std::move(data);
    }

    bool empty() const
    {
        return m_content.size() == 0;
    }

    ObjectType get_type() const;

    /**
     * Get the number of children held by this object
     *
     * \note This is only supported for maps and arrays
     */
    uint32_t get_size() const;

#ifdef USE_GEO
    geo::vector2d as_vector2() const;
#endif

    std::string as_string() const;
    integer_t as_integer() const;
    float_t as_float() const;
    bool as_boolean() const;
    BitStream as_bitstream() const;

    bool add(const std::string &path, const json::Document &value);
    
    /**
     * Get the key of the n-th child
     *
     * \note This is only supported for maps
     */
    std::string get_key(size_t pos) const;
    
    /**
     * Returns a read-only view of the child as position pos
     *
     * \note This is only supported for arrays and maps
     */
    json::Document get_child(size_t pos) const;

    bool matches_predicates(const json::Document &predicates) const;

    void iterate(Iterator &iterator) const;

    const BitStream& data() const
    {
        return m_content;
    }

    BitStream& data()
    {
        return m_content;
    }

    void operator=(Document &&other)
    {
        m_content = std::move(other.m_content);
    }

    /**
     * Discard contents of the document
     */
    void clear()
    {
        m_content.clear();
    }

    int64_t hash() const;

    void detach_data(uint8_t* &data, uint32_t &len)
    {
        m_content.detach(data, len);
    }

    /**
     * Create an identical copy of this document
     *
     * \param force_copy
     *      Ensure the new document is operation on a new buffer
     */
    Document duplicate(bool force_copy = false) const;

    bool insert(const std::string &path, const json::Document &doc);

    void compress(BitStream &bstream) const;

    std::string str() const;

    /**
     * Get a human-readable representation of the document that is nicely formatted
     */
    std::string pretty_str(int indent) const;

    /// Size in bytes of this document
    uint32_t byte_size() const
    {
        return m_content.size();
    }

    Diffs diff(const Document &other) const;

protected:
    Document() {}

    BitStream m_content;
};

/**
 * Construct a document from a std::string
 */
class String : public Document
{
public:
    String(const std::string &str);
};

class Binary : public Document
{
public:
    Binary(uint8_t *data, uint32_t length)
        : Document()
    {
        m_content << ObjectType::Binary;
        m_content << length;
        m_content.write_raw_data(data, length);
        m_content.move_to(0);
    }

    Binary(BitStream &bstream)
        : Document()
    {
        m_content << ObjectType::Binary;
        m_content << static_cast<uint32_t>(bstream.size());
        m_content.write_raw_data(bstream.data(), bstream.size());
        m_content.move_to(0);
    }
};

/**
 * Construct a document from an integer
 */
class Integer : public Document
{
public:
    Integer(integer_t i);
};

inline bool operator==(const Document &first, const Document &second)
{
    return first.data() == second.data();
}
}

inline BitStream& operator<<(BitStream &bs, const json::Document& doc)
{
    doc.compress(bs);
    return bs;
}

inline BitStream& operator>>(BitStream &bs, json::Document& doc)
{
    doc = json::Document(bs);
    return bs;
}

#ifndef IS_ENCLAVE
inline std::ostream& operator<<(std::ostream &os, const json::Document &doc)
{
    return os << doc.str();
}
#endif


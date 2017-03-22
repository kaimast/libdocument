#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include <stack>
#include <cctype>
#include <vector>
#include <time.h>

#ifndef IS_ENCLAVE
#include <ostream>
#endif

#include "BitStream.h"

namespace json
{

class Document;

enum class ObjectType : uint8_t
{
    Map,
    Array,
    String,
    Datetime,
    Integer,
    Float,
    True,
    False,
    Binary,
    Null
};

enum class DocumentMode
{
    ReadOnly,
    ReadWrite,
    Copy
};

enum class DiffType : uint8_t
{
    Modified,
    Deleted,
    Added,
};

inline bool is_valid_key(const std::string &str)
{
    if(str.size() == 0)
        return false;

    for(auto c: str)
    {
        if(!isalnum(c) && !(c == '_'))
            return false;
    }

    return true;
}

inline bool is_keyword(const std::string &str)
{
    return str == "+";
}

typedef int64_t integer_t;
typedef double float_t;

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

private:
    BitStream m_content;
};

class Iterator
{
public:
    virtual void handle_string(const std::string &key, const std::string &str) = 0;
    virtual void handle_integer(const std::string &key, const integer_t value) = 0;
    virtual void handle_float(const std::string &key, const float_t value) = 0;
    virtual void handle_map_start(const std::string &key) = 0;
    virtual void handle_boolean(const std::string &key, const bool value) = 0;
    virtual void handle_null(const std::string &key) = 0;
    virtual void handle_map_end() = 0;
    virtual void handle_array_start(const std::string &key) = 0;
    virtual void handle_array_end() = 0;
    virtual void handle_binary(const std::string &key, const uint8_t *data, uint32_t size) = 0;
    virtual void handle_datetime(const std::string &key, const tm& value) = 0;
};

class Document
{
public:
#ifndef IS_ENCLAVE
    Document(Document &&other);
#endif

    /// Filter parent by a set of paths
    Document(const Document &parent, const std::vector<std::string> &paths, bool force = false);

    /// Create a view on a subset of the document
    Document(const Document &parent, const std::string &path, bool force = false);

    explicit Document(BitStream &data);
    explicit Document(const std::string& str);

    Document(const uint8_t *data, uint32_t length, DocumentMode mode);
    Document(uint8_t *data, uint32_t length, DocumentMode mode);

    ~Document();

    /// Because we don't have move semantics...
    void copy(const Document &other);

    bool empty() const
    {
        return m_content.size() == 0;
    }

    ObjectType get_type() const;

    /// Get the number of objects
    uint32_t get_size() const;

    std::string as_string() const;
    integer_t as_integer() const;
    float_t as_float() const;
    bool as_boolean() const;
    BitStream as_bitstream() const;
//    const uint8_t* as_binary() const;

    bool add(const std::string &path, const json::Document &value);

    bool matches_predicates(const json::Document &predicates) const;

    void iterate(Iterator &iterator) const;

    const BitStream& data() const
    {
        return m_content;
    }

    int64_t hash() const;

    void detach_data(uint8_t* &data, uint32_t &len)
    {
        m_content.detach(data, len);
    }

#ifndef IS_ENCLAVE
    Document duplicate() const;
#endif

    bool insert(const std::string &path, const json::Document &doc);

    void compress(BitStream &bstream) const;

    std::string str() const;

    bool operator==(const Document& other) const
    {
        return this->m_content == other.m_content;
    }

    /// Size in bytes of this document
    uint32_t byte_size() const
    {
        return m_content.size();
    }

    std::vector<json::Diff*> diff(const Document &other) const;

protected:
    Document() {}

    BitStream m_content;
};

class String : public Document
{
public:
    String(const std::string &str)
        : Document()
    {
        uint32_t length = str.size();
        m_content << ObjectType::String;
        m_content << length;
        m_content.write_raw_data(reinterpret_cast<const uint8_t*>(str.c_str()), length);
    }
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

class Writer
{
public:
    Writer(BitStream &result);

    void start_map(const std::string &key);
    void end_map();

    void start_array(const std::string &key);
    void end_array();

    /// Write data that is already binary formatted.
    void write_raw_data(const std::string &key, const uint8_t *data, uint32_t size);

    void write_document(const std::string &key, const json::Document &other)
    {
        write_raw_data(key, other.data().data(), other.data().size());
    }

    void write_null(const std::string &key);
    void write_boolean(const std::string &key, const bool boolean);
    void write_datetime(const std::string &key, const tm &value);
    void write_integer(const std::string &key, const integer_t &value);
    void write_string(const std::string &key, const std::string &value);
    void write_float(const std::string &key, const float_t &value);

private:
    void handle_key(const std::string &key);
    void check_end();

    BitStream &m_result;

    enum mode_t {IN_ARRAY, IN_MAP, DONE};

    std::stack<mode_t> m_mode;
    std::stack<uint32_t> m_starts;
    std::stack<uint32_t> m_sizes;
};

}

inline BitStream& operator<<(BitStream &bs, const json::Document& doc)
{
    doc.compress(bs);
    return bs;
}

#ifndef IS_ENCLAVE
inline std::ostream& operator<<(std::ostream &os, const json::Document &doc)
{
    return os << doc.str();
}
#endif

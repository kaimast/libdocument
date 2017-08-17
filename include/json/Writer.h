#pragma once

#include <geo/vector2.h>
#include "defines.h"

namespace json
{

class Writer
{
public:
    Writer(BitStream &result);

    Writer();
    ~Writer();

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

    void write_vector2(const std::string &key, const geo::vector2d &vec);
    void write_null(const std::string &key);
    void write_boolean(const std::string &key, const bool boolean);
    void write_datetime(const std::string &key, const tm &value);
    void write_integer(const std::string &key, const integer_t &value);
    void write_string(const std::string &key, const std::string &value);
    void write_float(const std::string &key, const float_t &value);

    json::Document make_document()
    {
        uint8_t *data;
        uint32_t len;
        m_result.detach(data, len);

        return json::Document(data, len, DocumentMode::ReadWrite);
    }

private:
    void handle_key(const std::string &key);
    void check_end();

    BitStream *m_result_ptr;
    BitStream &m_result;

    enum mode_t {IN_ARRAY, IN_MAP, DONE};

    std::stack<mode_t> m_mode;
    std::stack<uint32_t> m_starts;
    std::stack<uint32_t> m_sizes;
};

}

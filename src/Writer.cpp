#include "json/json.h"

namespace json
{

Writer::Writer(BitStream &result)
    : m_result(result)
{
}

void Writer::start_array(const std::string &key)
{
    handle_key(key);

    m_result << ObjectType::Array;

    uint32_t start_pos = m_result.pos();
    uint32_t byte_size = 0, size = 0;
    m_result << byte_size << size;

    m_starts.push(start_pos);
    m_sizes.push(size);
    m_mode.push(IN_ARRAY);
}

void Writer::end_array()
{
    uint32_t end_pos = m_result.pos();
    uint32_t start_pos = m_starts.top();
    m_result.move_to(start_pos);
    uint32_t byte_size = end_pos - (start_pos + sizeof(uint32_t));
    uint32_t size = m_sizes.top();

    m_result << byte_size << size;
    m_result.move_to(end_pos);

    assert(m_mode.size() > 0);
    assert(m_mode.top() == IN_ARRAY);

    m_mode.pop();
    m_starts.pop();
    m_sizes.pop();

    check_end();
}

void Writer::start_map(const std::string &key)
{
    handle_key(key);

    m_result << ObjectType::Map;

    uint32_t start = m_result.pos();
    uint32_t byte_size = 0, size = 0;
    m_result << byte_size;
    m_result << size;

    m_mode.push(IN_MAP);
    m_sizes.push(size);
    m_starts.push(start);
}

void Writer::end_map()
{
    uint32_t end_pos = m_result.pos();
    uint32_t start_pos = m_starts.top();
    uint32_t size = m_sizes.top();
    m_result.move_to(start_pos);
    uint32_t byte_size = (end_pos - (start_pos + sizeof(uint32_t)));

    m_result << byte_size << size;

    m_result.move_to(end_pos);

    assert(m_mode.size() > 0);
    assert(m_mode.top() == IN_MAP);

    m_sizes.pop();
    m_starts.pop();
    m_mode.pop();

    check_end();
}

void Writer::check_end()
{
    if(m_mode.size() == 0)
    {
        m_mode.push(DONE);
        //m_result.move_to(0);
    }
}

void Writer::write_raw_data(const std::string &key, const uint8_t *data, uint32_t size)
{
    handle_key(key);
    m_result.write_raw_data(data, size);
    check_end();
}

void Writer::write_float(const std::string &key, const double &val)
{
    handle_key(key);
    m_result << ObjectType::Float << val;
    check_end();
}

void Writer::write_integer(const std::string &key, const integer_t &val)
{
    handle_key(key);
    m_result << ObjectType::Integer << val;
    check_end();
}

void Writer::write_boolean(const std::string &key, const bool value)
{
    handle_key(key);

    if(value == true)
        m_result << ObjectType::True;
    else
        m_result << ObjectType::False;

    check_end();
}

void Writer::write_null(const std::string &key)
{
    handle_key(key);
    m_result << ObjectType::Null;
    check_end();
}

void Writer::write_datetime(const std::string &key, const tm &val)
{
    handle_key(key);
    m_result << ObjectType::Datetime;
    m_result << val;
    check_end();
}

void Writer::write_string(const std::string &key, const std::string &str)
{
    handle_key(key);
    m_result << ObjectType::String << str;
    check_end();
}

void Writer::handle_key(const std::string &key)
{
    if(m_mode.size() == 0)
    {
        return;
    }

    if(m_mode.top() == DONE)
    {
        throw std::runtime_error("Cannot write more. Already done");
    }

    assert(m_mode.size() > 0);

    auto size = m_sizes.top();
    m_sizes.pop();
    m_sizes.push(size + 1);

    if(m_mode.top() == IN_MAP)
    {
        m_result << key;
    }
}

}

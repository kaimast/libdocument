#include "json.h"
#include "Iterator.h"

#include "defines.h"
using std::to_string;

namespace json
{

void Printer::handle_key(const std::string &key)
{
    const auto& m = mode.top();

    if(key == "")
    {
        assert(mode.empty());
        return;
    }
    else if(m == FIRST_IN_MAP)
    {
        result += '"' + key + "\":";
        mode.pop();
        mode.push(IN_MAP);
    }
    else if(m == IN_MAP)
    {
        result += ",\"" + key + "\":";
    }
    else if(m == FIRST_IN_ARRAY)
    {
        mode.pop();
        mode.push(IN_ARRAY);
    }
    else if(m == IN_ARRAY)
    {
        result += ",";
    }
    else
    {
        throw std::runtime_error("Unknown parse mode");
    }
}

void Printer::handle_string(const std::string &key, const std::string &str)
{
    handle_key(key);
    result += '"' + str + '"';
}

void Printer::handle_integer(const std::string &key, const integer_t i)
{
    handle_key(key);
    result += to_string(i);
}

void Printer::handle_float(const std::string &key, const json::float_t d)
{
    handle_key(key);
    result += to_string(d);
}

void Printer::handle_map_start(const std::string &key)
{
    handle_key(key);
    mode.push(FIRST_IN_MAP);
    result += "{";
}

void Printer::handle_binary(const std::string &key, const uint8_t *data, uint32_t len)
{
    handle_key(key);
    result += "b'";

    for(uint32_t i = 0; i < len; ++i)
    {
        uint8_t val = data[i];
        uint8_t up = (0xF0) & val << 4;
        uint8_t down = (0x0F) & val;

        result += to_hex(up);
        result += to_hex(down);
    }

    result += '\'';
}

void Printer::handle_boolean(const std::string &key, const bool value)
{
    handle_key(key);

    if(value)
    {
        result += keyword(TRUE);
    }
    else
    {
        result += keyword(FALSE);
    }
}

void Printer::handle_null(const std::string &key)
{
    handle_key(key);
    result += keyword(NIL);
}

void Printer::handle_datetime(const std::string &key, const tm& val)
{
    handle_key(key);

    result += "d\"";
    result += to_string(val.tm_year, 4) + "-" + to_string(val.tm_mon, 2) + "-" + to_string(val.tm_mday, 2);
    result += " " + to_string(val.tm_hour, 2) + ":" + to_string(val.tm_min, 2) + ":" + to_string(val.tm_sec, 2);
    result += '"';
}

void Printer::handle_map_end()
{
    mode.pop();
    result += "}";
}

void Printer::handle_array_start(const std::string &key)
{
    handle_key(key);
    result += "[";
    mode.push(FIRST_IN_ARRAY);
}

void Printer::handle_array_end()
{
    result += "]";
    mode.pop();
}

IterationEngine::IterationEngine(const BitStream &data, json::Iterator &iterator_)
    : iterator(iterator_)
{
    view.assign(data.data(), data.size(), true);
}

void IterationEngine::run()
{
    if(view.size() > 0)
    {
        parse_next("");
    }
}

void IterationEngine::parse_next(const std::string &key)
{
    ObjectType type;
    view >> type;

    switch(type)
    {
    case ObjectType::String:
    {
        std::string str;
        view >> str;
        iterator.handle_string(key, str);
        break;
    }
    case ObjectType::Integer:
    {
        integer_t i;
        view >> i;
        iterator.handle_integer(key, i);
        break;
    }
    case ObjectType::Float:
    {
        double val;
        view >> val;
        iterator.handle_float(key, val);
        break;
    }
    case ObjectType::Datetime:
    {
        tm val;
        view >> val;
        iterator.handle_datetime(key, val);
        break;
    }
    case ObjectType::Binary:
    {
        uint32_t size = 0;
        uint8_t *data = nullptr;
        view >> size;
        view.read_raw_data(&data, size);
        iterator.handle_binary(key, data, size);
        break;
    }
    case ObjectType::Map:
    {
        handle_map(key);
        break;
    }
    case ObjectType::Array:
    {
        handle_array(key);
        break;
    }
    case ObjectType::True:
    {
        iterator.handle_boolean(key, true);
        break;
    }
    case ObjectType::False:
    {
        iterator.handle_boolean(key, false);
        break;
    }
    case ObjectType::Null:
    {
        iterator.handle_null(key);
        break;
    }
    default:
    {
        throw std::runtime_error("Document Iteration failed: unknown object type");
    }
    }
}

void IterationEngine::handle_map(const std::string &key)
{
    iterator.handle_map_start(key);

    uint32_t byte_size = 0;
    view >> byte_size;

    uint32_t size = 0;
    view >> size;

    for(uint32_t i = 0; i < size; ++i)
    {
        std::string key;
        view >> key;
        parse_next(key);
    }

    iterator.handle_map_end();
}

void IterationEngine::handle_array(const std::string &key)
{
    iterator.handle_array_start(key);

    uint32_t byte_size = 0, size = 0;
    view >> byte_size >> size;

    for(uint32_t i = 0; i < size; ++i)
    {
        parse_next(to_string(i));
    }

    iterator.handle_array_end();
}

}

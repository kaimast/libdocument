#include "json.h"
#include "Iterator.h"
#include "defines.h"

namespace json
{

PredicateChecker::PredicateChecker(const json::Document &document)
    : m_document(document), m_matched(true)
{
}

bool PredicateChecker::get_result() const
{
    return m_matched;
}

void PredicateChecker::push_path(const std::string &path)
{
    size_t pos = 0;
    size_t last_pos = 0;

    // Split up condensed paths...
    while((pos = path.find_first_of('.', last_pos)) != std::string::npos)
    {
        const std::string key = path.substr(last_pos, pos-last_pos);
        last_pos = pos+1;

        push_key(key);
    }

    const std::string last_key = path.substr(last_pos, pos-last_pos);
    push_key(last_key);
}

void PredicateChecker::push_key(const std::string &key)
{
    if(key == "")
    {
        assert(m_path.size() == 0);
        return;
    }

    if(key == keyword(IN))
    {
        m_mode.push_back(predicate_mode::IN);

        json::Document view(m_document, path_string(m_path));

        m_in_type = view.get_type();
        m_in_found = false;

        switch(m_in_type)
        {
        case ObjectType::Integer:
            m_in_value_int = view.as_integer();
            break;
        case ObjectType::String:
            m_in_value_str = view.as_string();
            break;
        case ObjectType::Float:
            m_in_value_float = view.as_float();
            break;
        default:
            m_in_type = ObjectType::Null;
            break;
        }
    }
    else
    {
        m_mode.push_back(predicate_mode::NORMAL);
    }

    m_path.push_back(key);
}

void PredicateChecker::pop_path()
{
    if(m_path.size() == 0)
        return;

    if(m_mode.back() == predicate_mode::IN)
    {
        if(!m_in_found)
            m_matched = false;
    }

    m_path.pop_back();
    m_mode.pop_back();
}

void PredicateChecker::handle_binary(const std::string &key, const uint8_t *data, uint32_t len)
{

}

void PredicateChecker::handle_string(const std::string &key, const std::string &value)
{
    push_path(key);

    if(mode() == predicate_mode::NORMAL)
    {
        const auto full_paths = path_strings(m_path, m_document);
        bool found = false;

        for(auto &full_path : full_paths)
        {
            Document view(m_document, full_path);

            if(view.empty() || view.get_type() != ObjectType::String)
            {
                continue;
            }

            std::string other = view.as_string();
            if(other == value)
                found = true;
        }

        if(!found)
            m_matched = false;
    }
    else
    {
        if(m_in_type == ObjectType::String && value == m_in_value_str)
            m_in_found = true;
    }

    pop_path();
}

void PredicateChecker::handle_integer(const std::string &key, const integer_t value)
{
    push_path(key);

    if(mode() == predicate_mode::NORMAL)
    {
        const auto full_paths = path_strings(m_path, m_document);
        bool found = false;

        for(auto &full_path : full_paths)
        {
            Document view(m_document, full_path);

            if(view.empty() || view.get_type() != ObjectType::Integer)
            {
                continue;
            }

            integer_t other = view.as_integer();
            if(other == value)
                found = true;
        }

        if(!found)
            m_matched = false;
    }
    else
    {
        if(m_in_type == ObjectType::Integer && m_in_value_int == value)
            m_in_found = true;
    }

    pop_path();
}

void PredicateChecker::handle_float(const std::string &key, const json::float_t value)
{
    push_path(key);

    if(mode() == predicate_mode::NORMAL)
    {
        Document view(m_document, path_string(m_path));

        if(view.empty() || view.get_type() != ObjectType::Float)
        {
            m_matched = false;
        }
        else
        {
            json::float_t other = view.as_float();
            if(other != value)
                m_matched = false;
        }
    }
    else
    {
        if(m_in_type == ObjectType::Float && m_in_value_float == value)
            m_in_found = true;
    }

    pop_path();
}

void PredicateChecker::handle_map_start(const std::string &key)
{
    push_path(key);
}

void PredicateChecker::handle_boolean(const std::string &key, const bool value)
{
    push_path(key);

    Document view(m_document, path_string(m_path));

    if(view.empty() || (view.get_type() != ObjectType::True && view.get_type() != ObjectType::False))
    {
        m_matched = false;
    }
    else
    {
        bool other = view.as_boolean();
        if(other != value)
            m_matched = false;
    }

    pop_path();
}

void PredicateChecker::handle_null(const std::string &key)
{
    //FIXME
}

void PredicateChecker::handle_datetime(const std::string &key, const tm& val)
{
    //FIXME
}

void PredicateChecker::handle_map_end()
{
    pop_path();
}

void PredicateChecker::handle_array_start(const std::string &key)
{
    push_path(key);
}

void PredicateChecker::handle_array_end()
{
    pop_path();
}

PredicateChecker::predicate_mode PredicateChecker::mode() const
{
    for(auto &m : m_mode)
        if(m == predicate_mode::IN)
            return m;

    return predicate_mode::NORMAL;
}

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
        throw std::runtime_error("Unknown parse mode");
}

void Printer::handle_string(const std::string &key, const std::string &str)
{
    handle_key(key);
    result += '"' + str + '"';
}

void Printer::handle_integer(const std::string &key, const integer_t i)
{
    handle_key(key);
    result += std::to_string(i);
}

void Printer::handle_float(const std::string &key, const json::float_t d)
{
    handle_key(key);
    result += std::to_string(d);
}

void Printer::handle_map_start(const std::string &key)
{
    handle_key(key);
    mode.push(FIRST_IN_MAP);
    result += "{";
}

void Printer::handle_binary(const std::string &key, const uint8_t *data, uint32_t len)
{
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
        result += keyword(TRUE);
    else
        result += keyword(FALSE);
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
    parse_next("");
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
        throw std::runtime_error("Unknown object type");
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

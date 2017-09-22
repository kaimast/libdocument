#include "json.h"
#include "Iterator.h"

#include "defines.h"
using std::to_string;

namespace json
{

PredicateChecker::PredicateChecker(const json::Document &document)
    : m_pred_matches(false), m_pred_values(), m_document(document), m_matched(true)
{
}

bool PredicateChecker::get_result() const
{
    return m_matched;
}

void PredicateChecker::push_path(const std::string &path)
{
    if(path == "")
    {
        assert(m_path.size() == 0);
        return;
    }

    size_t pos = 0;
    size_t last_pos = 0;
    size_t count = 1;

    // Split up condensed paths...
    while((pos = path.find_first_of('.', last_pos)) != std::string::npos)
    {
        const std::string key = path.substr(last_pos, pos-last_pos);
        last_pos = pos+1;

        push_key(key);
        count++;
    }

    const std::string last_key = path.substr(last_pos, std::string::npos);
    push_key(last_key);

    m_path_sizes.push(count);
}

void PredicateChecker::push_key(const std::string &key)
{
    assert(key != "");

    if(key == keyword(IN))
    {
        m_mode.push_back(predicate_mode::IN);

        m_pred_matches = false;
        m_pred_values.clear();

        for(auto &path: path_strings(m_path, m_document))
        {
            json::Document view(m_document, path, false);

            pred_value val;
            val.type = view.get_type();

            switch(val.type)
            {
            case ObjectType::Integer:
                val.integer = view.as_integer();
                break;
            case ObjectType::String:
                val.str = view.as_string();
                break;
            case ObjectType::Float:
                val.floating = view.as_float();
                break;
            default:
                val.type = ObjectType::Null;
                break;
            }

            m_pred_values.push_back(val);
        }
    }
    else if(key == keyword(GREATER_THAN_EQUAL) || key == keyword(LESS_THAN))
    {
        if(key == keyword(LESS_THAN))
            m_mode.push_back(predicate_mode::LESS_THAN);
        else
            m_mode.push_back(predicate_mode::GREATER_THAN_EQUAL);

        m_pred_values.clear();
        m_pred_matches = false;

        for(auto &path: path_strings(m_path, m_document))
        {
            json::Document view(m_document, path, false);

            pred_value val;
            val.type = view.get_type();

            switch(val.type)
            {
            case ObjectType::Integer:
                val.integer = view.as_integer();
                break;
            case ObjectType::Float:
                val.floating = view.as_float();
                break;
            case ObjectType::String:
                val.str = view.as_string();
            default:
                val.type = ObjectType::Null;
                break;
            }

            m_pred_values.push_back(val);
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
    if(m_path_sizes.empty())
        return;

    size_t count = m_path_sizes.top();
    m_path_sizes.pop();

    if(m_path.size() < count || m_path.size() != m_mode.size())
        throw std::runtime_error("invalid state");

    for(size_t i = 0; i < count; ++i)
    {
        if(m_mode.back() != predicate_mode::NORMAL)
        {
            if(!m_pred_matches)
                m_matched = false;
        }

        m_path.pop_back();
        m_mode.pop_back();
    }
}

void PredicateChecker::handle_binary(const std::string &key, const uint8_t *data, uint32_t len)
{

}

void PredicateChecker::handle_string(const std::string &key, const std::string &value)
{
    push_path(key);

    if(mode() == predicate_mode::NORMAL)
    {
        bool found = false;

        for(auto &path: path_strings(m_path, m_document))
        {
            Document view(m_document, path, false);

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
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::String && val.str == value)
                m_pred_matches = true;
        }
    }

    pop_path();
}

void PredicateChecker::handle_integer(const std::string &key, const integer_t value)
{
    push_path(key);

    if(mode() == predicate_mode::NORMAL)
    {
        bool found = false;

        for(auto &path : path_strings(m_path, m_document))
        {
            Document view(m_document, path, false);

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
    else if(mode() == predicate_mode::IN)
    {
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::Integer && val.integer == value)
                m_pred_matches = true;
        }
    }
    else if(mode() == predicate_mode::LESS_THAN)
    {
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::Float && val.floating < value)
                m_pred_matches = true;
            else if(val.type == ObjectType::Integer && val.integer < value)
                m_pred_matches = true;
        }
    }
    else if(mode() == predicate_mode::GREATER_THAN_EQUAL)
    {
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::Float && val.floating >= value)
                m_pred_matches = true;
            else if(val.type == ObjectType::Integer && val.integer >= value)
                m_pred_matches = true;
        }
    }

    pop_path();
}

void PredicateChecker::handle_float(const std::string &key, const json::float_t value)
{
    push_path(key);

    if(mode() == predicate_mode::NORMAL)
    {
        Document view(m_document, path_string(m_path), false);

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
    else if(mode() == predicate_mode::IN)
    {
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::Float && val.floating == value)
                m_pred_matches = true;
            else if(val.type == ObjectType::Integer && val.integer == value)
                m_pred_matches = true;
        }
    }
    else if(mode() == predicate_mode::LESS_THAN)
    {
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::Float && val.floating < value)
                m_pred_matches = true;
            else if(val.type == ObjectType::Integer && val.integer < value)
                m_pred_matches = true;
        }
    }
    else if(mode() == predicate_mode::GREATER_THAN_EQUAL)
    {
        for(auto &val: m_pred_values)
        {
            if(val.type == ObjectType::Float && val.floating >= value)
                m_pred_matches = true;
            else if(val.type == ObjectType::Integer && val.integer >= value)
                m_pred_matches = true;
        }
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

    Document view(m_document, path_string(m_path), false);

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
    {
        if(m != predicate_mode::NORMAL)
            return m;
    }

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
    if(view.size() > 0)
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
        throw std::runtime_error("Document Iteration failed: unknown object type");
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

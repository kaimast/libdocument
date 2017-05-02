#include "Parser.h"
#include "Iterator.h"
#include "json.h"

#include <cctype>
#include <time.h>
#include <list>
#include <stack>

using std::to_string;

namespace json
{

String::String(const std::string &str)
    : Document()
{
    uint32_t length = str.size();
    m_content << ObjectType::String;
    m_content << length;
    m_content.write_raw_data(reinterpret_cast<const uint8_t*>(str.c_str()), length);
    m_content.move_to(0);
}

Integer::Integer(const integer_t i)
    : Document()
{
    m_content << ObjectType::Integer;
    m_content << i;
    m_content.move_to(0);
}

class DocumentTraversal
{
protected:
    void skip_next(ObjectType type, BitStream &view)
    {
        switch(type)
        {
        case ObjectType::Binary:
        {
            uint32_t size = 0;
            view >> size;
            view.move_by(size);
            break;
        }
        case ObjectType::Integer:
        {
            view.move_by(sizeof(json::integer_t));
            break;
        }
        case ObjectType::Float:
        {
            view.move_by(sizeof(json::float_t));
            break;
        }
        case ObjectType::String:
        case ObjectType::Map:
        case ObjectType::Array:
        {
            uint32_t byte_size;
            view >> byte_size;
            view.move_by(byte_size);
            break;
        }
        case ObjectType::Datetime:
        {
            view.move_by(sizeof(tm));
            break;
        }
        case ObjectType::True:
        case ObjectType::False:
        case ObjectType::Null:
            break;
        default:
            throw std::runtime_error("Document traversal failed: Unknown object type");
        }
    }
};

class DocumentDiffs : public DocumentTraversal
{
public:
    DocumentDiffs(const BitStream &data1, const BitStream &data2)
    {
        view1.assign(const_cast<uint8_t*>(data1.data()), data1.size(), true);
        view2.assign(const_cast<uint8_t*>(data2.data()), data2.size(), true);
    }

    void create_diffs(std::vector<json::Diff*> &diffs)
    {
        parse_next(diffs, false);
    }

private:
    void parse_next(std::vector<json::Diff*> &diffs, bool inside_diff)
    {
        assert(path1 == path2);

        uint32_t start = view2.pos();

        ObjectType type1, type2;
        view1 >> type1;
        view2 >> type2;

        if(type1 != type2)
        {
            skip_next(type1, view1);
            skip_next(type2, view2);
            uint32_t end = view2.pos();

            auto diff = new Diff(DiffType::Modified, path_string(path1), &view2.data()[start], end-start);
            diffs.push_back(diff);
        }
        else
        {
            assert(type1 == type2);
            switch(type1)
            {
            case ObjectType::String:
            {
                std::string str1, str2;
                view1 >> str1;
                view2 >> str2;

                uint32_t end = view2.pos();

                if(!inside_diff && str1 != str2)
                {
                    auto diff = new Diff(DiffType::Modified, path_string(path1), &view2.data()[start], end-start);
                    diffs.push_back(diff);
                }
                break;
            }
            case ObjectType::Integer:
            {
                integer_t i1, i2;
                view1 >> i1;
                view2 >> i2;

                uint32_t end = view2.pos();

                if(!inside_diff && i1 != i2)
                {
                    auto diff = new Diff(DiffType::Modified, path_string(path1), &view2.data()[start], end-start);
                    diffs.push_back(diff);
                }
                break;
            }
            case ObjectType::Float:
            {
                json::float_t d1, d2;
                view1 >> d1;
                view2 >> d2;

                uint32_t end = view2.pos();

                if(!inside_diff && d1 != d2)
                {
                    auto diff = new Diff(DiffType::Modified, path_string(path1), &view2.data()[start], end-start);
                    diffs.push_back(diff);
                }
                break;
            }
            case ObjectType::Map:
                parse_map(diffs, inside_diff);
                break;
            case ObjectType::Array:
                parse_array(diffs, inside_diff);
                break;
            case ObjectType::True:
            case ObjectType::False:
            case ObjectType::Null:
                break;
            default:
                throw std::runtime_error("Document diff failed: unknown object type");
            }
        }
    }

private:
    BitStream view1, view2;
    std::vector<std::string> path1;
    std::vector<std::string> path2;

    void parse_map(std::vector<Diff*> &diffs, bool inside_diff)
    {
        uint32_t byte_size1 = 0, byte_size2 = 0;
        view1 >> byte_size1;
        view2 >> byte_size2;

        uint32_t size1 = 0, size2 = 0;
        view1 >> size1;
        view2 >> size2;

        uint32_t i = 0, j = 0;

        while(i < size1 || j < size2)
        {
            std::string key1, key2;

            if(i < size1)
            {
                view1 >> key1;
                path1.push_back(key1);
            }

            if(j < size2)
            {
                view2 >> key2;
                path2.push_back(key2);
            }

            if(key1 == key2)
            {
                assert(key1 != "");
                parse_next(diffs, inside_diff);
            }
            else
            {
                //FIXME what if entries moved around?

                if(i < size1)
                {
                    uint32_t start = view1.pos();
                    ObjectType type;
                    view1 >> type;

                    skip_next(type, view1);
                    uint32_t end = view1.pos();

                    auto diff = new Diff(DiffType::Deleted, path_string(path1), &view1.data()[start], end-start);
                    diffs.push_back(diff);
                }

                if(j < size2)
                {
                    uint32_t start = view2.pos();
                    ObjectType type;
                    view2 >> type;

                    skip_next(type, view2);
                    uint32_t end = view2.pos();

                    auto diff = new Diff(DiffType::Added, path_string(path2), &view2.data()[start], end-start);
                    diffs.push_back(diff);
                }
            }

            if(i < size1)
            {
                path1.pop_back();
                ++i;
            }
            if(j < size2)
            {
                path2.pop_back();
                ++j;
            }
        }
    }

    void parse_array(std::vector<Diff*> &diffs, bool inside_diff)
    {
        uint32_t byte_size1 = 0, byte_size2 = 0;
        view1 >> byte_size1;
        view2 >> byte_size2;

        uint32_t size1 = 0, size2 = 0;
        view1 >> size1;
        view2 >> size2;

        uint32_t i = 0, j = 0;

        while(i < size1 || j < size2)
        {
            bool has_first = false, has_second = false;

            if(i < size1)
            {
                has_first = true;
                path1.push_back(to_string(i));
            }

            if(j < size2)
            {
                has_second = true;
                path2.push_back(to_string(j));
            }

            if(i == j && has_first && has_second)
            {
                parse_next(diffs, inside_diff);
            }
            else
            {
                if(has_first)
                {
                    uint32_t start = view1.pos();
                    ObjectType type;
                    view1 >> type;

                    skip_next(type, view1);
                    uint32_t end = view1.pos();

                    auto diff = new Diff(DiffType::Deleted, path_string(path1), &view1.data()[start], end-start);
                    diffs.push_back(diff);
                }

                if(has_second)
                {
                    uint32_t start = view2.pos();
                    ObjectType type;
                    view2 >> type;

                    skip_next(type, view2);
                    uint32_t end = view2.pos();

                    auto diff = new Diff(DiffType::Added, path_string(path2), &view2.data()[start], end-start);
                    diffs.push_back(diff);
                }
            }

            if(has_first)
            {
                path1.pop_back();
                ++i;
            }
            if(has_second)
            {
                path2.pop_back();
                ++j;
            }
        }
    }
};

class DocumentMerger : public DocumentTraversal
{
public:
    DocumentMerger(BitStream &doc, std::string full_path, const BitStream &other)
        : m_doc(doc), m_other(other), m_success(false)
    {
        size_t it;
        path.push_back("");

        while((it = full_path.find('.')) != std::string::npos)
        {
            std::string next = full_path.substr(0, it);
            full_path = full_path.substr(it+1, std::string::npos);

            path.push_back(next);
        }

        path.push_back(full_path);
    }

    bool do_merge()
    {
        parse_next("");
        m_doc.move_to(0);
        return m_success;
    }

    /// Return value indicates that element was found
    /// Check m_success for successful completion of update
    bool parse_next(const std::string &key)
    {
        ObjectType type;
        m_doc >> type;

        if(path.size() > 0 && path.front() == key)
        {
            path.pop_front();
        }
        else
        {
            skip_next(type, m_doc);
            return false;
        }

        bool is_target = path.size() == 1;

        if(is_target)
        {
            if(type == ObjectType::Map)
            {
                uint32_t map_start = m_doc.pos();
                uint32_t byte_size, size;
                m_doc >> byte_size >> size;

                auto &key = path.front();
                bool found = false;

                for(uint32_t i = 0; i < size; ++i)
                {
                    uint32_t start = m_doc.pos();

                    std::string ckey;
                    m_doc >> ckey;
                    ObjectType type;
                    m_doc >> type;

                    if(ckey == key)
                    {
                        // remove field first
                        skip_next(type, m_doc);
                        uint32_t end = m_doc.pos();
                        m_doc.move_to(start);
                        m_doc.remove_space(end-start);
                        found = true;
                    }
                    else
                        skip_next(type, m_doc);
                }

                if(found)
                {
                    //update sizes
                    uint32_t map_end = m_doc.pos();
                    byte_size = map_end - map_start - sizeof(byte_size);
                    size -= 1;

                    m_doc.move_to(map_start);
                    m_doc << byte_size << size;
                    m_doc.move_to(map_end);
                }

                // Make sure field does not exist yet
                if(!is_valid_key(key))
                {
                    return true;
                }

                insert_into_map(key, m_other, map_start);
                m_success = true;
                return true;
            }
            else if(type == ObjectType::Array)
            {
                uint32_t array_start = m_doc.pos();
                uint32_t byte_size = 0, size = 0;
                m_doc >> byte_size >> size;

                auto &key = path.front();

                // One can only append to an array for now
                if(!is_array_insertion(key))
                {
                    m_doc.move_by(byte_size - sizeof(size));
                    return true;
                }

                uint32_t pos = 0;
                for(; pos < size; ++pos)
                {
                    ObjectType type;
                    m_doc >> type;
                    skip_next(type, m_doc);
                }

                uint32_t elem_start = m_doc.pos();

                m_doc.make_space(m_other.size());
                m_doc.write_raw_data(m_other.data(), m_other.size());

                byte_size += m_doc.pos() - elem_start;
                size += 1;

                m_doc.move_to(array_start);
                m_doc << byte_size << size;

                m_doc.move_by(byte_size - sizeof(size));
                m_success = true;
                return true;
            }
            else
                return true;
        }
        else
        {
            switch(type)
            {
            case ObjectType::Map:
                return parse_map();
                break;
            case ObjectType::Array:
                return parse_array();
                break;
            default:
                throw std::runtime_error("we shouldn't be here");
            }
        }
    }

private:
    void insert_into_map(const std::string& key, const BitStream &data, uint32_t start)
    {
        m_doc.make_space(data.size() + sizeof(uint32_t) + key.size());
        m_doc << key;
        m_doc.write_raw_data(data.data(), data.size());

        uint32_t end = m_doc.pos();
        uint32_t byte_size = end - start - sizeof(byte_size); // this is the start of the map not the element

        m_doc.move_to(start);

        uint32_t _, size;
        m_doc >> _ >> size;
        size += 1;

        m_doc.move_to(start);
        m_doc << byte_size << size;
        m_doc.move_by(byte_size - sizeof(size));
    }

    bool parse_map()
    {
        uint32_t start = m_doc.pos();

        uint32_t byte_size = 0;
        m_doc >> byte_size;

        uint32_t size = 0;
        m_doc >> size;

        bool found = false;

        for(uint32_t i = 0; i < size; ++i)
        {
            std::string key;
            m_doc >> key;

            if(parse_next(key))
                found = true;
        }

        if(found && m_success)
        {
            uint32_t end = m_doc.pos();
            byte_size = end-start-sizeof(byte_size);
            m_doc.move_to(start);
            m_doc << byte_size;
            m_doc.move_to(end);
        }
        else if(!found)
        {
            assert(path.size() >= 2);

            BitStream data;
            json::Writer writer(data);

            auto it = path.begin();
            ++it;

            if(is_array_insertion(*it))
            {
                writer.start_array("");
                writer.end_array();
            }
            else
            {
                writer.start_map("");
                writer.end_map();
            }

            insert_into_map(path.front(), data, start);

            // try again...
            m_doc.move_to(start);
            parse_map();
        }

        return true;
    }

    bool is_array_insertion(const std::string &str)
    {
        return str == "+";
    }

    bool parse_array()
    {
        uint32_t array_start = m_doc.pos();

        uint32_t byte_size = 0;
        m_doc >> byte_size;

        uint32_t size = 0;
        m_doc >> size;

        bool found = false;

        for(uint32_t i = 0; i < size; ++i)
        {
            if(parse_next(to_string(i)))
                found = true;
        }

        if(found && m_success)
        {
            uint32_t end = m_doc.pos();
            byte_size = end - array_start - sizeof(byte_size);

            m_doc.move_to(array_start);
            m_doc << byte_size;
            m_doc.move_to(end);
        }

        //if(!found)
        //FIXME

        return found;
    }

    std::list<std::string> path;

    BitStream &m_doc;
    const BitStream &m_other;
    bool m_success;
};

class DocumentAdd : public DocumentTraversal
{
public:
    DocumentAdd(BitStream &data, const std::string &path, const json::Document &value)
        : m_view(data), m_target_path(path), m_value(value)
    {
        m_view.move_to(0);
    }

    void do_add()
    {
        auto t = m_value.get_type();

        if(t != ObjectType::Integer && t != ObjectType::Float && t != ObjectType::String)
            throw std::runtime_error("Add not defined on type type!");

        parse_next();
    }

private:
    void parse_next()
    {
        auto current = path_string(m_current_path);
        bool on_path = false;
        bool on_target = false;

        auto len = std::min(current.size(), m_target_path.size());
        if(m_target_path.compare(0, len, current) == 0)
        {
            on_path = true;
            if(current.size() >= m_target_path.size())
                on_target = true;
        }

        std::string key = "";
        if(m_current_path.size() > 0)
            key = m_current_path.back();

        ObjectType type;
        m_view >> type;

        if(!on_path)
        {
            skip_next(type, m_view);
            return;
        }

        if(on_target)
        {
            if(type != m_value.get_type())
                throw std::runtime_error("Incompatible types!");

            if(type == ObjectType::Integer)
            {
                json::integer_t value;
                m_view >> value;

                value += m_value.as_integer();
                m_view.move_by(-1*static_cast<int32_t>(sizeof(value)));
                m_view << value;
            }
            else if(type == ObjectType::Float)
            {
                json::float_t value;
                m_view >> value;

                value += m_value.as_float();
                m_view.move_by(-1*static_cast<int32_t>(sizeof(value)));
                m_view << value;
            }
            else
                throw std::runtime_error("Incompatible types");
        }
        else
        {
            switch(type)
            {
            case ObjectType::String:
            case ObjectType::Integer:
            case ObjectType::Float:
                throw std::runtime_error("Invalid path");
                break;
            case ObjectType::Map:
                parse_map();
                break;
            case ObjectType::Array:
                parse_array();
                break;
            case ObjectType::True:
            case ObjectType::False:
            case ObjectType::Null:
                break;
            default:
                throw std::runtime_error("Document add failed: unknown object type");
            }
        }
    }

private:
    BitStream& m_view;
    const std::string m_target_path;
    std::vector<std::string> m_current_path;
    const json::Document &m_value;

    void parse_map()
    {
        uint32_t byte_size = 0;
        m_view >> byte_size;

        uint32_t size = 0;
        m_view >> size;

        for(uint32_t i = 0; i < size; ++i)
        {
            std::string key;
            m_view >> key;

            m_current_path.push_back(key);
            parse_next();
            m_current_path.pop_back();
        }
    }

    void parse_array()
    {
        uint32_t byte_size = 0;
        m_view >> byte_size;

        uint32_t size = 0;
        m_view >> size;

        for(uint32_t i = 0; i < size; ++i)
        {
            m_current_path.push_back(to_string(i));
            parse_next();
            m_current_path.pop_back();
        }
    }
};

class DocumentSearch : public DocumentTraversal
{
public:
    DocumentSearch(const Document &document, const std::vector<std::string> &paths, bool write_path)
        : m_document(document), m_write_path(write_path), m_found_count(0)
    {
        m_view.assign(m_document.data().data(), m_document.data().size(), true);

        for(auto &path: paths)
        {
            size_t pos = 0, last_pos = 0;
            std::vector<std::string> split_path;
            while((pos = path.find_first_of('.', last_pos)) != std::string::npos)
            {
                split_path.push_back(path.substr(last_pos, pos-last_pos));
                last_pos = pos+1;
            }

            split_path.push_back(path.substr(last_pos, pos-last_pos));
            m_target_paths.push_back(split_path);
        }
    }

    uint32_t do_search(BitStream &result)
    {
        json::Writer writer(result);
        parse_next(writer);

        return m_found_count;
    }

private:
    void parse_next(json::Writer &writer)
    {
        auto current = path_string(m_current_path);
        bool on_path = false;
        bool on_target = false;

        for(auto &path: m_target_paths)
        {
            const auto full_paths = path_strings(path, m_document);

            for(auto &target_path: full_paths)
            {
                auto len = std::min(current.size(), target_path.size());

                if(len == 0)
                {
                    on_path = true;
                }
                else if(target_path.compare(0, len, current) == 0)
                {
                    if(target_path.size() == len)
                    {
                        on_path = on_target = true;
                    }
                    else if(target_path[len] == '.')
                    {
                        on_path = true;
                    }
                }
            }
        }

        std::string key = "";
        if(m_current_path.size() > 0)
            key = m_current_path.back();

        uint32_t start = m_view.pos();

        ObjectType type;
        m_view >> type;

        if(!on_path)
        {
            skip_next(type, m_view);
            return;
        }

        if(on_target)
        {
            skip_next(type, m_view);

            uint32_t end = m_view.pos();

            std::string nkey = m_write_path ? key : "";
            writer.write_raw_data(nkey, &m_view.data()[start], end-start);

            m_found_count += 1;
            return;
        }

        switch(type)
        {
        case ObjectType::String:
        {
            std::string str;
            m_view >> str;
            break;
        }
        case ObjectType::Integer:
        {
            json::integer_t i;
            m_view >> i;
            break;
        }
        case ObjectType::Float:
        {
            json::float_t d;
            m_view >> d;
            break;
        }
        case ObjectType::Map:
            parse_map(writer);
            break;
        case ObjectType::Array:
            parse_array(writer);
            break;
        case ObjectType::True:
        case ObjectType::False:
        case ObjectType::Null:
            break;
        default:
            throw std::runtime_error("Document search failed: Unknown object type");
        }
    }

private:
    const json::Document &m_document;

    BitStream m_view;
    std::vector<std::vector<std::string>> m_target_paths; //FIXME preserve order
    std::vector<std::string> m_current_path;
    const bool m_write_path;
    uint32_t m_found_count;

    void parse_map(json::Writer &writer)
    {
        uint32_t byte_size = 0;
        m_view >> byte_size;

        uint32_t size = 0;
        m_view >> size;

        std::string key = m_current_path.size() > 0 ? m_current_path.back() : "";

        if(m_write_path)
            writer.start_map(key);

        for(uint32_t i = 0; i < size; ++i)
        {
            std::string key;
            m_view >> key;

            m_current_path.push_back(key);
            parse_next(writer);
            m_current_path.pop_back();
        }

        if(m_write_path)
            writer.end_map();
    }

    void parse_array(json::Writer &writer)
    {
        uint32_t byte_size = 0;
        m_view >> byte_size;

        uint32_t size = 0;
        m_view >> size;

        std::string key = m_current_path.size() > 0 ? m_current_path.back() : "";

        if(m_write_path)
            writer.start_array(key);

        for(uint32_t i = 0; i < size; ++i)
        {
            m_current_path.push_back(to_string(i));
            parse_next(writer);
            m_current_path.pop_back();
        }

        if(m_write_path)
            writer.end_array();
    }
};


Document::~Document()
{
}

Document::Document(const std::string &str)
{
    Parser parser(str, m_content);
    parser.do_parse();
    m_content.move_to(0);
}

Document::Document(Document &&other)
    : m_content(std::move(other.m_content))
{
}

Document::Document(BitStream &data)
{
    uint32_t size = 0;
    data >> size;
    m_content.write_raw_data(data.current(), size);
    data.move_by(size);
    m_content.move_to(0);
}

bool Document::matches_predicates(const json::Document &predicates) const
{
    if(predicates.empty())
        return true;

    json::PredicateChecker checker(*this);
    json::IterationEngine engine(predicates.data(), checker);
    engine.run();

    return checker.get_result();
}

bool Document::insert(const std::string &path, const Document &doc)
{
    DocumentMerger merger(m_content, path, doc.m_content);
    return merger.do_merge();
}

int64_t Document::hash() const
{
    return m_content.hash();
}

Document Document::duplicate() const
{
    return Document(m_content.data(), m_content.size(), DocumentMode::Copy);
}

void Document::copy(const Document &other)
{
    m_content.resize(0);
    m_content.move_to(0);
    m_content.write_raw_data(other.m_content.data(), other.m_content.size());
    m_content.move_to(0);
}

void Document::compress(BitStream &bstream) const
{
    uint32_t size = m_content.size();
    bstream << size;
    bstream.write_raw_data(m_content.data(), m_content.size());
}

void Document::iterate(json::Iterator &iterator) const
{
    json::IterationEngine engine(data(), iterator);
    engine.run();
}

std::string Document::str() const
{
    json::Printer printer;
    iterate(printer);
    return printer.get_result();
}

Document::Document(const Document& parent, const std::vector<std::string> &paths, bool force)
{
    DocumentSearch search(parent, paths, true);
    uint32_t num_found = search.do_search(m_content);

    if(num_found != paths.size() && force)
        throw std::runtime_error("Not all paths were found");
}

Document::Document(const Document& parent, const std::string &path, bool force)
{
    std::vector<std::string> paths;
    paths.push_back(path);

    DocumentSearch search(parent, paths, false);
    uint32_t num_found = search.do_search(m_content);

    if(num_found == 0 && force)
        throw std::runtime_error("Path was not found");
}

Document::Document(uint8_t *data, uint32_t length, DocumentMode mode)
{
    if(mode == DocumentMode::ReadOnly)
        m_content.assign(data, length, true);
    else if(mode == DocumentMode::ReadWrite)
        m_content.assign(data, length, false);
    else if(mode == DocumentMode::Copy)
        m_content.write_raw_data(data, length);
    else
        throw std::runtime_error("Unknown Doucment mode");

    m_content.move_to(0);
}

Document::Document(const uint8_t *data, uint32_t length, DocumentMode mode)
{
    if(mode == DocumentMode::ReadWrite)
        throw std::runtime_error("Cannot modify read-only data");
    else if(mode == DocumentMode::ReadOnly)
        m_content.assign(data, length, true);
    else if(mode == DocumentMode::Copy)
        m_content.write_raw_data(data, length);
    else
        throw std::runtime_error("Unknown Doucment mode");

    m_content.move_to(0);
}

Diff::Diff(DiffType type, const std::string &path, const uint8_t *value, uint32_t length)
{
    m_content << type << path << length;
    m_content.write_raw_data(value, length);
}

bool Document::add(const std::string &path, const json::Document &value)
{
    DocumentAdd adder(m_content, path, value);

    try
    {
        adder.do_add();
        m_content.move_to(0);
        return true;
    }
    catch(std::runtime_error& e)
    {
        m_content.move_to(0);
        return false;
    }
}

std::vector<std::string> Document::get_keys() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    std::vector<std::string> result;

    ObjectType type;
    view >> type;

    if(type != ObjectType::Map)
        throw std::runtime_error("Document is not a map");

    uint32_t byte_size, size;
    view >> byte_size >> size;

    for(uint32_t i = 0; i < size; ++i)
    {
        std::string key;
        view >> key;
        result.push_back(key);

        ObjectType ctype;
        view >> ctype;

        switch(ctype)
        {
        case ObjectType::Binary:
        case ObjectType::Map:
        case ObjectType::Array:
        case ObjectType::String:
        {
            uint32_t byte_size;
            view >> byte_size;
            view.move_by(byte_size);
            break;
        }
        case ObjectType::Null:
        case ObjectType::True:
        case ObjectType::False:
            break;
        case ObjectType::Integer:
            view.move_by(sizeof(json::integer_t));
            break;
        case ObjectType::Float:
            view.move_by(sizeof(json::float_t));
            break;
        default:
            throw std::runtime_error("Unknown document type!");
        }
    }

    return result;
}


uint32_t Document::get_size() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    switch(type)
    {
    case ObjectType::Binary:
    {
        uint32_t byte_size;
        view >> byte_size;
        return byte_size;
    }
    case ObjectType::Null:
        return 0;
    case ObjectType::Datetime:
    case ObjectType::Float:
    case ObjectType::Integer:
    case ObjectType::String:
    case ObjectType::True:
    case ObjectType::False:
        return 1;
    case ObjectType::Map:
    case ObjectType::Array:
    {
        uint32_t byte_size, size;
        view >> byte_size >> size;
        return size;
    }
    default:
        throw std::runtime_error("Unknown document type!");
    }
}

ObjectType Document::get_type() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    if(view.at_end())
        return ObjectType::Null;

    ObjectType type;
    view >> type;

    return type;
}

BitStream Document::as_bitstream() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    if(type != ObjectType::Binary)
        throw std::runtime_error("Not a binary object");

    uint32_t size;
    view >> size;

    BitStream result;
    result.assign(view.current(), size, true);
    return result;
}
/*
const uint8_t* Document::as_binary() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    if(type != ObjectType::Binary)
        throw std::runtime_error("Not a binary object");

    return view.current();
}*/

json::integer_t Document::as_integer() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    if(type != ObjectType::Integer)
        throw std::runtime_error("Not an integer!");

    json::integer_t i;
    view >> i;
    return i;
}

json::float_t Document::as_float() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    if(type != ObjectType::Float)
        throw std::runtime_error("Not a float!");

    json::float_t f;
    view >> f;
    return f;
}

bool Document::as_boolean() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    if(type == ObjectType::True)
        return true;
    else if(type == ObjectType::False)
        return false;
    else
        throw std::runtime_error("Not a boolean!");
}

std::string Document::as_string() const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    ObjectType type;
    view >> type;

    if(type != ObjectType::String)
        throw std::runtime_error("Not a string!");

    std::string str;
    view >> str;
    return str;
}

std::vector<json::Diff*> Document::diff(const Document &other) const
{
    std::vector<json::Diff*> diffs;
    DocumentDiffs runner(m_content, other.m_content);
    runner.create_diffs(diffs);
    return diffs;
}

#ifndef IS_ENCLAVE
json::Document Diff::as_document() const
{
    BitStream bstream;
    compress(bstream, false);

    uint8_t *data = nullptr;
    uint32_t len = 0;

    bstream.detach(data, len);

    return json::Document(data, len, json::DocumentMode::ReadWrite);
}
#endif

void Diff::compress(BitStream &bstream, bool write_size) const
{
    BitStream view;
    view.assign(m_content.data(), m_content.size(), true);

    uint32_t size = 0;
    auto size_pos = bstream.pos();

    if(write_size)
        bstream << size;

    Writer writer(bstream);

    writer.start_map("");
    DiffType type;
    view >> type;

    if(type == DiffType::Modified)
        writer.write_string("type", "modified");
    else if(type == DiffType::Deleted)
        writer.write_string("type", "deleted");
    else if(type == DiffType::Added)
        writer.write_string("type", "added");
    else
        throw std::runtime_error("Unknown diff type");

    std::string path;
    view >> path;
    writer.write_string("path", path);

    if(type == DiffType::Modified || type == DiffType::Added)
    {
        auto key = "value";
        if(type == DiffType::Modified)
            key = "new_value";

        uint32_t len = 0;
        view >> len;

        Document doc(view.current(), len, DocumentMode::ReadOnly);
        writer.write_document(key, doc);
    }

    writer.end_map();

    if(write_size)
    {
        auto end = bstream.pos();
        bstream.move_to(size_pos);
        size = end - (size_pos + sizeof(size));
        bstream << size;
        bstream.move_to(end);
    }
}
}

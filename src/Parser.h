#pragma once

#include <string>
#include "json/json.h"

namespace json
{

class Parser
{
public:
    Parser(const std::string &input, BitStream &result);

    void do_parse();

private:
    void parse(const std::string &key);

    void skip_whitespace();

    void parse_array(const std::string &key);
    void parse_null(const std::string &key);
    void parse_string(const std::string &key);
    void parse_number(const std::string &key);
    void parse_map(const std::string &key);
    void parse_true(const std::string &key);
    void parse_false(const std::string &key);
    void parse_datetime(const std::string &key);

    std::string read_string();

    bool check_string(const char *s);

    const std::string &str;
    std::string::const_iterator it;

    Writer writer;
};

}

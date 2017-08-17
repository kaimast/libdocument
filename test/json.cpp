#include "peerdb/json.h"
#include <gtest/gtest.h>

using namespace json;

class JsonTest : public testing::Test
{
};

TEST(JsonTest, compress_two)
{
    Document input1("{\"a\":1}");
    Document input2("{\"a\":2}");

    BitStream bstream;
    input1.compress(bstream);
    input2.compress(bstream);

    bstream.move_to(0);
    Document output1(bstream);
    Document output2(bstream);

    EXPECT_EQ(input1.str(), output1.str());
    EXPECT_EQ(input2.str(), output2.str());
}

TEST(JsonTest, binary)
{
    constexpr uint32_t length = 1235;
    uint8_t value[length];

    json::Binary bin(value, length);

    json::Document doc("{\"a\":[]}");
    doc.insert("a.+", bin);

    json::Document view(doc, "a.0");

    EXPECT_EQ(memcmp(view.as_bitstream().data(), value, length), 0);
}

TEST(JsonTest, binary2)
{
    BitStream bs;
    bs << "This is only a test";

    json::Binary bin(bs);

    json::Document doc("{\"a\":[]}");
    doc.insert("a.+", bin);

    json::Document view(doc, "a.0");

    EXPECT_EQ(view.as_bitstream(), bs);
}

TEST(JsonTest, datetime)
{
    std::string str = "d\"1955-11-05 12:00:00\"";
    Document input(str);

    BitStream bstream;
    input.compress(bstream);

    bstream.move_to(0);

    Document output(bstream);
    EXPECT_EQ(input.str(), output.str());
    EXPECT_EQ(input.str(), str);
}

TEST(JsonTest, boolean)
{
    Document input("true");
    EXPECT_EQ(input.str(), "true");
}

TEST(JsonTest, map)
{
    Document doc("{\"a\":\"b\", \"c\":1}");

    BitStream bstream;
    doc.compress(bstream);

    bstream.move_to(0);
    Document doc2(bstream);

    EXPECT_EQ(doc.str(), "{\"a\":\"b\",\"c\":1}");
    EXPECT_EQ(doc2.str(), "{\"a\":\"b\",\"c\":1}");
}

TEST(JsonTest, array)
{
    Document doc("[1,2,3]");

    BitStream bstream;
    doc.compress(bstream);

    bstream.move_to(0);
    Document doc2(bstream);

    EXPECT_EQ(doc2.str(), doc.str());
}

TEST(JsonTest, nested_map)
{
    Document doc("{\"a\":{\"b\":42}}");

    BitStream bstream;
    doc.compress(bstream);

    bstream.move_to(0);
    Document doc2(bstream);

    EXPECT_EQ(doc2.str(), doc.str());
}

TEST(JsonTest, array_in_map)
{
    Document doc("{\"a\":[\"b\",42]}");

    BitStream bstream;
    doc.compress(bstream);

    bstream.move_to(0);
    Document doc2(bstream);

    EXPECT_EQ(doc2.str(), doc.str());
}

TEST(JsonTest, cannot_append_to_non_array)
{
    Document doc("{\"a\":[4,3,2], \"b\":{}}");
    bool result = doc.insert("b.+", json::Document("23"));

    EXPECT_FALSE(result);
    EXPECT_EQ(doc.str(), "{\"a\":[4,3,2],\"b\":{}}");
}

TEST(JsonTest, array_nested_multi_append)
{
    Document doc("{\"b\":\"xyz\",\"a\":{\"foo\":[4,3,2]}, \"bar\": 1337}");

    for(auto i = 0; i < 5; ++i)
        doc.insert("a.foo.+", json::Document("23"));

    EXPECT_EQ(doc.str(), "{\"b\":\"xyz\",\"a\":{\"foo\":[4,3,2,23,23,23,23,23]},\"bar\":1337}");
}

TEST(JsonTest, array_nested_pppend)
{
    Document doc("{\"b\":\"xyz\",\"a\":{\"foo\":[4,3,2]}, \"bar\": 1337}");
    bool result = doc.insert("a.foo.+", json::Document("23"));

    EXPECT_TRUE(result);
    EXPECT_EQ(doc.str(), "{\"b\":\"xyz\",\"a\":{\"foo\":[4,3,2,23]},\"bar\":1337}");
}
TEST(JsonTest, array_append)
{
    Document doc("{\"b\":\"xyz\",\"a\":[4,3,2]}");
    bool result = doc.insert("a.+", json::Document("23"));

    EXPECT_TRUE(result);
    EXPECT_EQ(doc.str(), "{\"b\":\"xyz\",\"a\":[4,3,2,23]}");
}

TEST(JsonTest, update)
{
    Document doc("{\"a\":42}");
    doc.insert("a", json::Document("23"));
    EXPECT_EQ(doc.str(), "{\"a\":23}");
}

TEST(JsonTest, array_predicate)
{
    Document doc("{\"a\":[5,4,{\"c\":3}]}");

    Document predicate1("{\"a.1\":5}");
    Document predicate2("{\"a.0\":5}");
    Document predicate3("{\"a.2.c\":3}");

    EXPECT_FALSE(doc.matches_predicates(predicate1));
    EXPECT_TRUE(doc.matches_predicates(predicate2));
    EXPECT_TRUE(doc.matches_predicates(predicate3));
}

TEST(JsonTest, set_predicate)
{
    Document doc1("{\"id\":42}");
    Document doc2("{\"id\":\"whatever\"}");
    Document doc3("{\"id\":1337.0}");

    Document predicate("{\"id\":{\"$in\": [\"whoever\", 1337.0, \"whatever\", \"however\"]}}");

    EXPECT_FALSE(doc1.matches_predicates(predicate));
    EXPECT_TRUE(doc2.matches_predicates(predicate));
    EXPECT_TRUE(doc3.matches_predicates(predicate));
}

TEST(JsonTest, wildcard_predicate)
{
    Document doc1("{\"a\":[1,3,4]}");
    Document doc2("{\"a\":[2,5,{\"b\":42}]}");

    Document predicate1("{\"a\":{\"*\":3}}");
    Document predicate2("{\"a.*\":3}");
    Document predicate3("{\"a.*.b\":42}");

    EXPECT_FALSE(doc2.matches_predicates(predicate1));
    EXPECT_TRUE(doc1.matches_predicates(predicate1));
    EXPECT_TRUE(doc1.matches_predicates(predicate2));
    EXPECT_TRUE(doc2.matches_predicates(predicate3));
}

TEST(JsonTest, filter)
{
    Document doc("{\"a\":41, \"b\":42, \"c\": 42}");

    Document filtered(doc, std::vector<std::string>{"b"});

    EXPECT_EQ(filtered.str(), "{\"b\":42}");
}

TEST(JsonTest, wildcard_filter)
{
    Document doc("{\"a\":[{\"b\":41,\"c\":42},{\"b\":43,\"c\":42}]}");

    Document filtered(doc, std::vector<std::string>{"a.*.b"});

    EXPECT_EQ(filtered.str(), "{\"a\":[{\"b\":41},{\"b\":43}]}");
}

TEST(JsonTest, wildcard_filter_nested)
{
    Document doc("{\"a\":[{\"b\":{\"c\":42}}]}");

    Document filtered(doc, std::vector<std::string>{"a.*.b.c"});

    EXPECT_EQ(filtered.str(), "{\"a\":[{\"b\":{\"c\":42}}]}");
}

TEST(JsonTest, add)
{
    Document doc("{\"a\":42}");
    Document to_add("5");

    bool result = doc.add("a", to_add);

    EXPECT_TRUE(result);
    EXPECT_EQ(doc.str(), "{\"a\":47}");
}

TEST(JsonTest, insert)
{
    Document doc("{\"a\":42}");
    bool result = doc.insert("b", json::Document("23"));

    EXPECT_TRUE(result);
    EXPECT_EQ(doc.str(), "{\"a\":42,\"b\":23}");
}

TEST(JsonTest, view)
{
    Document doc("{\"a\":{\"b\":[1,2,4,5]}}");
    Document view(doc, "a.b");

    EXPECT_EQ(view.str(), "[1,2,4,5]");
}

TEST(JsonTest, array_view)
{
    Document doc("{\"a\":[1,2,{\"b\":[42,23]},5]}}");
    Document view(doc, "a.2.b");

    EXPECT_EQ(view.str(), "[42,23]");
}

TEST(JsonTest, view2)
{
    Document doc("{\"a\":{\"b\":[1,2,4,5]},\"c\":42}");

    Document view(doc, "c");
    EXPECT_EQ(view.str(), "42");
}

TEST(JsonTest, no_diffs)
{
    Document doc1("{\"a\":42}");
    Document doc2("{\"a\":42}");

    auto diffs = doc1.diff(doc2);
    EXPECT_EQ(diffs.size(), 0);
}

TEST(JsonTest, diffs)
{
    Document doc1("{\"a\":1}");
    Document doc2("{\"a\":2}");

    auto diffs = doc1.diff(doc2);

    EXPECT_EQ(diffs.size(), 1);
    EXPECT_EQ(diffs.begin()->as_document(), json::Document("{\"type\":\"modified\",\"path\":\"a\",\"new_value\":2}"));
}

TEST(JsonTest, compress_diffs)
{
    Document doc1("{\"a\":1}");
    Document doc2("{\"a\":2}");

    auto diffs = doc1.diff(doc2);

    BitStream bstream;
    diffs.begin()->compress(bstream, true);

    bstream.move_to(0);

    json::Document doc(bstream);
    EXPECT_EQ(doc, json::Document("{\"type\":\"modified\",\"path\":\"a\",\"new_value\":2}"));
}

TEST(JsonTest, diffs_str)
{
    Document doc1("{\"a\":\"here\"}");
    Document doc2("{\"a\":\"we go\"}");

    auto diffs = doc1.diff(doc2);

    EXPECT_EQ(diffs.size(), 1);
    EXPECT_EQ(diffs.begin()->as_document(), json::Document("{\"type\":\"modified\",\"path\":\"a\",\"new_value\":\"we go\"}"));
}

TEST(JsonTest, diff_deleted)
{
    Document doc1("{\"a\":\"here\"}");
    Document doc2("{}");

    auto diffs = doc1.diff(doc2);

    EXPECT_EQ(diffs.size(), 1);
    EXPECT_EQ(diffs.begin()->as_document(), json::Document("{\"type\":\"deleted\",\"path\":\"a\"}"));
}

TEST(JsonTest, diff_added)
{
    Document doc1("{\"a\":\"here\"}");
    Document doc2("{\"a\":\"here\",\"b\":\"we go\"}");

    auto diffs = doc1.diff(doc2);

    EXPECT_EQ(diffs.size(), 1);
    EXPECT_EQ(diffs.begin()->as_document(), json::Document("{\"type\":\"added\",\"path\":\"b\",\"value\":\"we go\"}"));
}

TEST(JsonTest, diff_added_array)
{
    Document doc1("[\"here\"]");
    Document doc2("[\"here\",\"we go\"]");

    auto diffs = doc1.diff(doc2);

    EXPECT_EQ(diffs.size(), 1);
    EXPECT_EQ(diffs.begin()->as_document(), json::Document("{\"type\":\"added\",\"path\":\"1\",\"value\":\"we go\"}"));
}

TEST(JsonTest, cant_put_path)
{
    json::String doc("str");

    bool res = doc.insert("foo", json::Integer(42));

    EXPECT_FALSE(res);
    EXPECT_EQ(doc.str(), "\"str\"");
}

TEST(JsonTest, match_predicates)
{
    json::Document doc("{\"NO_D_ID\":1,\"NO_W_ID\":4,\"NO_O_ID\":300}");
    json::Document predicates("{\"NO_D_ID\":1,\"NO_W_ID\":4}");

    EXPECT_TRUE(doc.matches_predicates(predicates));
}

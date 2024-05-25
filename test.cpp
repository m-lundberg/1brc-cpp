#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "trie.hpp"

struct Data {
    int a = 0;
    int b = 0;
};

TEST_CASE("Trie insertion")
{
    TrieNode<Data, 256> root;

    root.insert("abc", {2, 3});
    root.insert("abcd", {4, 5});
}

TEST_CASE("Trie try_insert")
{
    TrieNode<Data, 256> root;

    auto val = root.try_insert("abc", {2, 3});
    CHECK(root.find("abc") != nullptr);
    CHECK(root.find("abc")->a == 2);
    CHECK(val->a == root.find("abc")->a);

    val = root.try_insert("abc", {5, 6});
    CHECK(root.find("abc") != nullptr);
    CHECK(root.find("abc")->a == 2);
    CHECK(val->a == root.find("abc")->a);
}

TEST_CASE("Trie searching")
{
    TrieNode<Data, 256> root;

    root.insert("abc", {2, 3});
    root.insert("abcd", {4, 5});

    CHECK(root.find("abc") != nullptr);
    CHECK(root.find("abc")->a == 2);
    CHECK(root.find("abc")->b == 3);

    CHECK(root.find("abcd") != nullptr);
    CHECK(root.find("abcd")->a == 4);
    CHECK(root.find("abcd")->b == 5);

    CHECK(root.find("") == nullptr);
    CHECK(root.find("asdf") == nullptr);
}

TEST_CASE("Trie erasing")
{
    TrieNode<Data, 256> root;
    root.insert("abc", {2, 3});
    root.insert("abcd", {4, 5});

    root.erase("abc");
    CHECK(root.find("abc") == nullptr);
    CHECK(root.find("abcd")->a == 4);
}

TEST_CASE("Trie get_all")
{
    TrieNode<Data, 256> root;
    root.insert("abc", {2, 3});
    root.insert("abcd", {4, 5});

    auto a = root.get_all();
    for (const auto& b : a) {
        std::cerr << b.first << ", " << b.second->a << std::endl;
    }
}

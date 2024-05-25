#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


template<typename T, size_t alphabet_size>
class TrieNode {
public:
    TrieNode() = default;
    TrieNode(const TrieNode&) = delete;
    TrieNode& operator=(const TrieNode&) = delete;

    const T* find(std::string_view key) const {
        if (key.empty()) {
            return leaf ? &data.value() : nullptr;
        }

        auto& next = children[key.front()];
        if (next == nullptr) {
            return nullptr;
        }
        return next->find(key.substr(1));
    }

    void insert(std::string_view key, const T& value) {
        if (key.empty()) {
            data = value;
            leaf = true;
            return;
        }

        auto& next = children[key.front()];
        if (next == nullptr) {
            next = std::make_unique<TrieNode<T, alphabet_size>>();
        }
        next->insert(key.substr(1), value);
    }

    // Insert element if key doesn't already exist, otherwise to nothing
    T* try_insert(std::string_view key, const T& value) {
        if (key.empty()) {
            if (!leaf) {
                data = value;
                leaf = true;
            }
            return &data.value();
        }

        auto& next = children[key.front()];
        if (next == nullptr) {
            next = std::make_unique<TrieNode<T, alphabet_size>>();
        }
        return next->try_insert(key.substr(1), value);
    }
    T* try_insert(std::basic_string_view<unsigned char> key, const T& value) {
        if (key.empty()) {
            if (!leaf) {
                data = value;
                leaf = true;
            }
            return &data.value();
        }

        auto& next = children[key.front()];
        if (next == nullptr) {
            next = std::make_unique<TrieNode<T, alphabet_size>>();
        }
        return next->try_insert(key.substr(1), value);
    }

    void erase(std::string_view key) {
        if (key.empty()) {
            if (leaf) {
                leaf = false;
                data = std::nullopt;
            }
            return;
        }
        
        auto& next = children[key.front()];
        if (next == nullptr) {
            // Key doesn't exist, just ignore
            return;
        }
        next->erase(key.substr(1));
    }

    std::vector<std::pair<std::string, const T*>> get_all(const std::string& key = "") const {
        std::vector<std::pair<std::string, const T*>> result;
        if (leaf) {
            result.emplace_back(std::make_pair(key, &data.value()));
        }
        for (size_t i = 0; i < alphabet_size; ++i) {
            auto& next = children[i];
            if (next == nullptr) {
                continue;
            }
            auto c = next->get_all(key + (char)i);
            result.insert(result.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
        }
        return result;
    }

    std::array<std::unique_ptr<TrieNode<T, alphabet_size>>, alphabet_size> children{};
    bool leaf = false; // TODO replace with data.has_value()
    std::optional<T> data;
};

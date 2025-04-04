#pragma once
#include <string>
#include <vector>
#include <set>
#include <functional>


namespace tools{
    std::string generate_pack_id();
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    template<typename T>
    std::vector<T> filterSet(const std::set<T>& original,
                                    std::function<bool(const T&)> predicate);
}

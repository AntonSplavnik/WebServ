// src/helpers/helpers.tpp
#pragma once
#include <vector>
#include <algorithm>

// Helper to add unique elements from src (string) to dest (vector)
template<typename T>
void addUnique(std::vector<T>& dest, const T& value) {
    if (std::find(dest.begin(), dest.end(), value) == dest.end()) {
        dest.push_back(value);
    }
}
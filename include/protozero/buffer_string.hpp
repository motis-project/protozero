#ifndef PROTOZERO_BUFFER_STRING_HPP
#define PROTOZERO_BUFFER_STRING_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file buffer_string.hpp
 *
 * @brief Contains the customization points for buffer implementations
 */

#include <protozero/buffer.hpp>

#include <cstddef>
#include <string>

namespace protozero {

// Implementation of buffer customizations points for std::string

template <>
inline void buffer_append_zeros<std::string>(std::string* buffer, std::size_t count) {
    buffer->append(count, '\0');
}

template <>
inline void buffer_reserve_additional<std::string>(std::string* buffer, std::size_t size) {
    buffer->reserve(buffer->size() + size);
}

template <>
inline void buffer_erase_range<std::string>(std::string* buffer, std::size_t from, std::size_t to) {
    buffer->erase(buffer->begin() + from, buffer->begin() + to);
}

template <>
inline char* buffer_at_pos<std::string>(std::string* buffer, std::size_t pos) {
    return &*(buffer->begin() + pos);
}

} // namespace protozero

#endif // PROTOZERO_BUFFER_STRING_HPP

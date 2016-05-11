#ifndef PROTOZERO_TYPES_HPP
#define PROTOZERO_TYPES_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file types.hpp
 *
 * @brief Contains the declaration of low-level types used in the pbf format.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>

#include <protozero/iterators.hpp>

namespace protozero {

/**
 * The type used for field tags (field numbers).
 */
using pbf_tag_type = uint32_t;

/**
 * The type used to encode type information.
 * See the table on
 *    https://developers.google.com/protocol-buffers/docs/encoding
 */
enum class pbf_wire_type : uint32_t {
    varint           = 0, // int32/64, uint32/64, sint32/64, bool, enum
    fixed64          = 1, // fixed64, sfixed64, double
    length_delimited = 2, // string, bytes, embedded messages,
                            // packed repeated fields
    fixed32          = 5, // fixed32, sfixed32, float
    unknown          = 99 // used for default setting in this library
};

/**
 * The type used for length values, such as the length of a field.
 */
using pbf_length_type = uint32_t;

#ifdef PROTOZERO_USE_VIEW
using data_view = PROTOZERO_USE_VIEW;
#else

/**
 * Holds a pointer to some data and a length.
 *
 * This class is supposed to be compatible with the std::string_view
 * that will be available in C++17.
 */
class data_view {

    const char* m_data;
    std::size_t m_size;

public:

    constexpr data_view(const char* data, std::size_t size) noexcept
        : m_data(data),
          m_size(size) {
    }

    constexpr data_view(const std::string& str) noexcept
        : m_data(str.data()),
          m_size(str.size()) {
    }

    data_view(const char* data) noexcept
        : m_data(data),
          m_size(std::strlen(data)) {
    }

    constexpr const char* data() const noexcept {
        return m_data;
    }
    constexpr std::size_t size() const noexcept {
        return m_size;
    }

    std::string to_string() const {
        return std::string{m_data, m_size};
    }

    explicit operator std::string() const {
        return std::string{m_data, m_size};
    }

}; // class data_view

#endif

namespace detail {

    template <typename T> class packed_field_varint;
    template <typename T> class packed_field_svarint;
    template <typename T> class packed_field_fixed;
    template <typename T, int N> class packed_field_varint_tag;
    template <typename T, int N> class packed_field_svarint_tag;
    template <typename T, int N> class packed_field_fixed_tag;

} // end namespace detail

struct varint_tag {};
struct bool_tag : public varint_tag {};
struct svarint_tag {};
struct fixed_tag {};
struct length_delimited_tag {};

template <typename T, typename C>
struct fixed_length {
    static constexpr const bool value = std::is_same<T, fixed_tag>::value && std::is_base_of<std::forward_iterator_tag, C>::value;
};

template <typename T>
class message;

namespace types {

    template <typename T>
    struct packed {
        using scalar_type = T;
        using tag = typename T::tag;
        using type = typename T::type;
        static constexpr const auto wire_type = T::wire_type;
    };

    template <typename T>
    struct enum_wrap {
        using tag = varint_tag;
        using type = T;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    template <typename T>
    struct message_wrap {
        using tag = length_delimited_tag;
        using type = message<T>;
        static constexpr const auto wire_type = pbf_wire_type::length_delimited;
    };

    struct _bool {
        using tag = bool_tag;
        using type = int32_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _enum {
        using tag = varint_tag;
        using type = int32_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _int32 {
        using tag = varint_tag;
        using type = int32_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _sint32 {
        using tag = svarint_tag;
        using type = int32_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _uint32 {
        using tag = varint_tag;
        using type = uint32_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _int64 {
        using tag = varint_tag;
        using type = int64_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _sint64 {
        using tag = svarint_tag;
        using type = int64_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _uint64 {
        using tag = varint_tag;
        using type = uint64_t;
        static constexpr const auto wire_type = pbf_wire_type::varint;
    };

    struct _fixed32 {
        using tag = fixed_tag;
        using type = uint32_t;
        static constexpr const auto wire_type = pbf_wire_type::fixed32;
    };

    struct _sfixed32 {
        using tag = fixed_tag;
        using type = int32_t;
        static constexpr const auto wire_type = pbf_wire_type::fixed32;
    };

    struct _fixed64 {
        using tag = fixed_tag;
        using type = uint64_t;
        static constexpr const auto wire_type = pbf_wire_type::fixed64;
    };

    struct _sfixed64 {
        using tag = fixed_tag;
        using type = int64_t;
        static constexpr const auto wire_type = pbf_wire_type::fixed64;
    };

    struct _float {
        using tag = fixed_tag;
        using type = float;
        static constexpr const auto wire_type = pbf_wire_type::fixed32;
    };

    struct _double {
        using tag = fixed_tag;
        using type = double;
        static constexpr const auto wire_type = pbf_wire_type::fixed64;
    };

    struct _bytes {
        using tag = length_delimited_tag;
        using type = data_view;
        static constexpr const auto wire_type = pbf_wire_type::length_delimited;
    };

    struct _string {
        using tag = length_delimited_tag;
        using type = data_view;
        static constexpr const auto wire_type = pbf_wire_type::length_delimited;
    };

    struct _message {
        using tag = length_delimited_tag;
        using type = data_view;
        static constexpr const auto wire_type = pbf_wire_type::length_delimited;
    };

} // end namespace types

namespace detail {

    template <typename T>
    struct iterator_type;

    template <>
    struct iterator_type<varint_tag> {
        template <typename U> using type = const_varint_iterator<U>;
    };

    template <>
    struct iterator_type<bool_tag> {
        template <typename U> using type = const_varint_iterator<U>;
    };

    template <>
    struct iterator_type<svarint_tag> {
        template <typename U> using type = const_svarint_iterator<U>;
    };

    template <>
    struct iterator_type<fixed_tag> {
        template <typename U> using type = const_fixed_iterator<typename U::type>;
    };



    template <typename T>
    struct packed_type;

    template <>
    struct packed_type<varint_tag> {
        template <typename U, int N> using type = packed_field_varint_tag<U, N>;
    };

    template <>
    struct packed_type<bool_tag> {
        template <typename U, int N> using type = packed_field_varint_tag<U, N>;
    };

    template <>
    struct packed_type<svarint_tag> {
        template <typename U, int N> using type = packed_field_svarint_tag<U, N>;
    };

    template <>
    struct packed_type<fixed_tag> {
        template <typename U, int N> using type = packed_field_fixed_tag<U, N>;
    };



    struct is_scalar{};
    struct is_packed{};

    template <typename T>
    struct traits {
        using tag = is_scalar;
        using get_result_type = typename T::type;
    };

    template <typename T>
    struct traits<types::packed<T>> {
        using tag = is_packed;
        using iterator = typename iterator_type<typename T::tag>::template type<T>;
        using get_result_type = iterator_range<iterator>;
    };

    template <typename T>
    struct traits<types::enum_wrap<T>> {
        using tag = is_scalar;
        using get_result_type = T;
    };

} // end namespace detail

template <typename T>
using iterator_range_over = iterator_range<typename detail::traits<types::packed<T>>::iterator>;

} // end namespace protozero

#endif // PROTOZERO_TYPES_HPP

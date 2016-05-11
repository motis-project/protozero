#ifndef PROTOZERO_PBF_READER_HPP
#define PROTOZERO_PBF_READER_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file pbf_reader.hpp
 *
 * @brief Contains the pbf_reader class.
 */

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <protozero/config.hpp>
#include <protozero/exception.hpp>
#include <protozero/iterators.hpp>
#include <protozero/types.hpp>
#include <protozero/varint.hpp>

#if PROTOZERO_BYTE_ORDER != PROTOZERO_LITTLE_ENDIAN
# include <protozero/byteswap.hpp>
#endif

namespace protozero {

/**
 * This class represents a protobuf message. Either a top-level message or
 * a nested sub-message. Top-level messages can be created from any buffer
 * with a pointer and length:
 *
 * @code
 *    std::string buffer;
 *    // fill buffer...
 *    pbf_reader message(buffer.data(), buffer.size());
 * @endcode
 *
 * Sub-messages are created using get_message():
 *
 * @code
 *    pbf_reader message(...);
 *    message.next();
 *    pbf_reader submessage = message.get_message();
 * @endcode
 *
 * All methods of the pbf_reader class except get_bytes() and get_string()
 * provide the strong exception guarantee, ie they either succeed or do not
 * change the pbf_reader object they are called on. Use the get_data() method
 * instead of get_bytes() or get_string(), if you need this guarantee.
 */
class pbf_reader {

    // A pointer to the next unread data.
    const char* m_data = nullptr;

    // A pointer to one past the end of data.
    const char* m_end = nullptr;

    // The wire type of the current field.
    pbf_wire_type m_wire_type = pbf_wire_type::unknown;

    // The tag of the current field.
    pbf_tag_type m_tag = 0;

    pbf_length_type get_length() {
        return pbf_length_type(decode_varint(&m_data, m_end));
    }

    void skip_bytes(pbf_length_type len) {
        if (m_data + len > m_end) {
            throw end_of_buffer_exception();
        }
        m_data += len;

    // In debug builds reset the tag to zero so that we can detect (some)
    // wrong code.
#ifndef NDEBUG
        m_tag = 0;
#endif
    }

    pbf_length_type get_len_and_skip() {
        const auto len = get_length();
        skip_bytes(len);
        return len;
    }

    template <typename T, typename R>
    R get_scalar(varint_tag) {
        return R(decode_varint(&m_data, m_end));
    }

    template <typename T, typename R>
    R get_scalar(bool_tag) {
        protozero_assert((*m_data & 0x80) == 0 && "not a 1 byte varint");
        skip_bytes(1);
        return m_data[-1] != 0; // -1 okay because we incremented m_data the line before
    }

    template <typename T, typename R>
    R get_scalar(svarint_tag) {
        return R(decode_zigzag64(decode_varint(&m_data, m_end)));
    }

    template <typename T, typename R>
    R get_scalar(fixed_tag) {
        constexpr const std::size_t size = sizeof(R);
        R result;
        skip_bytes(size);
        detail::copy_or_byteswap<size>(m_data - size, &result);
        return result;
    }

    template <typename T, typename R>
    R get_scalar(length_delimited_tag) {
        const auto len = get_len_and_skip();
        return data_view{m_data-len, len};
    }

    template <typename T, typename R>
    R get_impl(detail::is_scalar) {
        protozero_assert(has_wire_type(T::wire_type));
        return get_scalar<T, R>(typename T::tag{});
    }

    template <typename T, typename R>
    R get_impl(detail::is_packed) {
        const auto len = get_len_and_skip();
        return iterator_range_creator<typename detail::traits<T>::iterator>::create(m_data-len, m_data);
    }

public:

    /**
     * Construct a pbf_reader message from a data_view. The pointer from the
     * data_view will be stored inside the pbf_reader object, no data is
     * copied. So you must* make sure the view stays valid as long as the
     * pbf_reader object is used.
     *
     * The buffer must contain a complete protobuf message.
     *
     * @post There is no current field.
     */
    explicit pbf_reader(const data_view& view) noexcept
        : m_data(view.data()),
          m_end(view.data() + view.size()),
          m_wire_type(pbf_wire_type::unknown),
          m_tag(0) {
    }

    /**
     * Construct a pbf_reader message from a data pointer and a length. The pointer
     * will be stored inside the pbf_reader object, no data is copied. So you must
     * make sure the buffer stays valid as long as the pbf_reader object is used.
     *
     * The buffer must contain a complete protobuf message.
     *
     * @post There is no current field.
     */
    pbf_reader(const char* data, std::size_t length) noexcept
        : m_data(data),
          m_end(data + length),
          m_wire_type(pbf_wire_type::unknown),
          m_tag(0) {
    }

    /**
     * Construct a pbf_reader message from a std::string. A pointer to the string
     * internals will be stored inside the pbf_reader object, no data is copied.
     * So you must make sure the string is unchanged as long as the pbf_reader
     * object is used.
     *
     * The string must contain a complete protobuf message.
     *
     * @post There is no current field.
     */
    pbf_reader(const std::string& data) noexcept
        : m_data(data.data()),
          m_end(data.data() + data.size()),
          m_wire_type(pbf_wire_type::unknown),
          m_tag(0) {
    }

    /**
     * pbf_reader can be default constructed and behaves like it has an empty
     * buffer.
     */
    pbf_reader() noexcept = default;

    /// pbf_reader messages can be copied trivially.
    pbf_reader(const pbf_reader&) noexcept = default;

    /// pbf_reader messages can be moved trivially.
    pbf_reader(pbf_reader&&) noexcept = default;

    /// pbf_reader messages can be copied trivially.
    pbf_reader& operator=(const pbf_reader& other) noexcept = default;

    /// pbf_reader messages can be moved trivially.
    pbf_reader& operator=(pbf_reader&& other) noexcept = default;

    ~pbf_reader() = default;

    /**
     * In a boolean context the pbf_reader class evaluates to `true` if there are
     * still fields available and to `false` if the last field has been read.
     */
    operator bool() const noexcept {
        return m_data < m_end;
    }

    /**
     * Return the length in bytes of the current message. If you have
     * already called next() and/or any of the get_*() functions, this will
     * return the remaining length.
     *
     * This can, for instance, be used to estimate the space needed for a
     * buffer. Of course you have to know reasonably well what data to expect
     * and how it is encoded for this number to have any meaning.
     */
    std::size_t length() const noexcept {
        return std::size_t(m_end - m_data);
    }

    /**
     * Set next field in the message as the current field. This is usually
     * called in a while loop:
     *
     * @code
     *    pbf_reader message(...);
     *    while (message.next()) {
     *        // handle field
     *    }
     * @endcode
     *
     * @returns `true` if there is a next field, `false` if not.
     * @pre There must be no current field.
     * @post If it returns `true` there is a current field now.
     */
    bool next() {
        if (m_data == m_end) {
            return false;
        }

        const auto value = decode_varint(&m_data, m_end);
        m_tag = pbf_tag_type(value >> 3);

        // tags 0 and 19000 to 19999 are not allowed as per
        // https://developers.google.com/protocol-buffers/docs/proto
        protozero_assert(((m_tag > 0 && m_tag < 19000) || (m_tag > 19999 && m_tag <= ((1 << 29) - 1))) && "tag out of range");

        m_wire_type = pbf_wire_type(value & 0x07);
        switch (m_wire_type) {
            case pbf_wire_type::varint:
            case pbf_wire_type::fixed64:
            case pbf_wire_type::length_delimited:
            case pbf_wire_type::fixed32:
                break;
            default:
                throw unknown_pbf_wire_type_exception();
        }

        return true;
    }

    /**
     * Set next field with given tag in the message as the current field.
     * Fields with other tags are skipped. This is usually called in a while
     * loop for repeated fields:
     *
     * @code
     *    pbf_reader message(...);
     *    while (message.next(17)) {
     *        // handle field
     *    }
     * @endcode
     *
     * or you can call it just once to get the one field with this tag:
     *
     * @code
     *    pbf_reader message(...);
     *    if (message.next(17)) {
     *        // handle field
     *    }
     * @endcode
     *
     * @returns `true` if there is a next field with this tag.
     * @pre There must be no current field.
     * @post If it returns `true` there is a current field now with the given tag.
     */
    bool next(pbf_tag_type tag) {
        while (next()) {
            if (m_tag == tag) {
                return true;
            } else {
                skip();
            }
        }
        return false;
    }

    /**
     * The tag of the current field. The tag is the field number from the
     * description in the .proto file.
     *
     * Call next() before calling this function to set the current field.
     *
     * @returns tag of the current field.
     * @pre There must be a current field (ie. next() must have returned `true`).
     */
    pbf_tag_type tag() const noexcept {
        return m_tag;
    }

    /**
     * Get the wire type of the current field. The wire types are:
     *
     * * 0 - varint
     * * 1 - 64 bit
     * * 2 - length-delimited
     * * 5 - 32 bit
     *
     * All other types are illegal.
     *
     * Call next() before calling this function to set the current field.
     *
     * @returns wire type of the current field.
     * @pre There must be a current field (ie. next() must have returned `true`).
     */
    pbf_wire_type wire_type() const noexcept {
        return m_wire_type;
    }

    /**
     * Check the wire type of the current field.
     *
     * @returns `true` if the current field has the given wire type.
     * @pre There must be a current field (ie. next() must have returned `true`).
     */
    bool has_wire_type(pbf_wire_type type) const noexcept {
        return wire_type() == type;
    }

    /**
     * Consume the current field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @post The current field was consumed and there is no current field now.
     */
    void skip() {
        protozero_assert(tag() != 0 && "call next() before calling skip()");
        switch (wire_type()) {
            case pbf_wire_type::varint:
                (void)get_uint32(); // called for the side-effect of skipping value
                break;
            case pbf_wire_type::fixed64:
                skip_bytes(8);
                break;
            case pbf_wire_type::length_delimited:
                skip_bytes(get_length());
                break;
            case pbf_wire_type::fixed32:
                skip_bytes(4);
                break;
            default:
                protozero_assert(false && "can not be here because next() should have thrown already");
        }
    }

    template <typename T, typename R = typename detail::traits<T>::get_result_type>
    R get() {
        protozero_assert(tag() != 0 && "call next() before accessing field value");
        using tag_type = typename detail::traits<T>::tag;
        return get_impl<T, R>(tag_type{});
    }

    ///@{
    /**
     * @name Scalar field accessor functions
     */

    /**
     * Consume and return value of current "bool" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bool".
     * @post The current field was consumed and there is no current field now.
     */
    bool get_bool() {
        return get<types::_bool>();
    }

    /**
     * Consume and return value of current "enum" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "enum".
     * @post The current field was consumed and there is no current field now.
     */
    int32_t get_enum() {
        return get<types::_enum>();
    }

    /**
     * Consume and return value of current "int32" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "int32".
     * @post The current field was consumed and there is no current field now.
     */
    int32_t get_int32() {
        return get<types::_int32>();
    }

    /**
     * Consume and return value of current "sint32" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sint32".
     * @post The current field was consumed and there is no current field now.
     */
    int32_t get_sint32() {
        return get<types::_sint32>();
    }

    /**
     * Consume and return value of current "uint32" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "uint32".
     * @post The current field was consumed and there is no current field now.
     */
    uint32_t get_uint32() {
        return get<types::_uint32>();
    }

    /**
     * Consume and return value of current "int64" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "int64".
     * @post The current field was consumed and there is no current field now.
     */
    int64_t get_int64() {
        return get<types::_int64>();
    }

    /**
     * Consume and return value of current "sint64" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sint64".
     * @post The current field was consumed and there is no current field now.
     */
    int64_t get_sint64() {
        return get<types::_sint64>();
    }

    /**
     * Consume and return value of current "uint64" varint field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "uint64".
     * @post The current field was consumed and there is no current field now.
     */
    uint64_t get_uint64() {
        return get<types::_uint64>();
    }

    /**
     * Consume and return value of current "fixed32" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "fixed32".
     * @post The current field was consumed and there is no current field now.
     */
    uint32_t get_fixed32() {
        return get<types::_fixed32>();
    }

    /**
     * Consume and return value of current "sfixed32" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sfixed32".
     * @post The current field was consumed and there is no current field now.
     */
    int32_t get_sfixed32() {
        return get<types::_sfixed32>();
    }

    /**
     * Consume and return value of current "fixed64" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "fixed64".
     * @post The current field was consumed and there is no current field now.
     */
    uint64_t get_fixed64() {
        return get<types::_fixed64>();
    }

    /**
     * Consume and return value of current "sfixed64" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "sfixed64".
     * @post The current field was consumed and there is no current field now.
     */
    int64_t get_sfixed64() {
        return get<types::_sfixed64>();
    }

    /**
     * Consume and return value of current "float" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "float".
     * @post The current field was consumed and there is no current field now.
     */
    float get_float() {
        return get<types::_float>();
    }

    /**
     * Consume and return value of current "double" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "double".
     * @post The current field was consumed and there is no current field now.
     */
    double get_double() {
        return get<types::_double>();
    }

    /**
     * Consume and return value of current "bytes", "string", or "message"
     * field.
     *
     * @returns A data_view object.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bytes", "string", or "message".
     * @post The current field was consumed and there is no current field now.
     */
    data_view get_view() {
        return get<types::_string>();
    }

    /**
     * Consume and return value of current "bytes" or "string" field.
     *
     * @returns A data_view object.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bytes" or "string".
     * @post The current field was consumed and there is no current field now.
     */
    data_view get_data() {
        return get<types::_string>();
    }

    /**
     * Consume and return value of current "bytes" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "bytes".
     * @post The current field was consumed and there is no current field now.
     */
    std::string get_bytes() {
        return std::string(get<types::_string>());
    }

    /**
     * Consume and return value of current "string" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "string".
     * @post The current field was consumed and there is no current field now.
     */
    std::string get_string() {
        return get_bytes();
    }

    /**
     * Consume and return value of current "message" field.
     *
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "message".
     * @post The current field was consumed and there is no current field now.
     */
    pbf_reader get_message() {
        return pbf_reader(get_data());
    }

    ///@}

    ///@{
    /**
     * @name Repeated packed field accessor functions
     */

    /**
     * Consume current "repeated packed bool" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed bool".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_bool> get_packed_bool() {
        return get<types::packed<types::_bool>>();
    }

    /**
     * Consume current "repeated packed enum" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed enum".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_enum> get_packed_enum() {
        return get<types::packed<types::_enum>>();
    }

    /**
     * Consume current "repeated packed int32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed int32".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_int32> get_packed_int32() {
        return get<types::packed<types::_int32>>();
    }

    /**
     * Consume current "repeated packed sint32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sint32".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_sint32> get_packed_sint32() {
        return get<types::packed<types::_sint32>>();
    }

    /**
     * Consume current "repeated packed uint32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed uint32".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_uint32> get_packed_uint32() {
        return get<types::packed<types::_uint32>>();
    }

    /**
     * Consume current "repeated packed int64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed int64".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_int64> get_packed_int64() {
        return get<types::packed<types::_int64>>();
    }

    /**
     * Consume current "repeated packed sint64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sint64".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_sint64> get_packed_sint64() {
        return get<types::packed<types::_sint64>>();
    }

    /**
     * Consume current "repeated packed uint64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed uint64".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_uint64> get_packed_uint64() {
        return get<types::packed<types::_uint64>>();
    }

    /**
     * Consume current "repeated packed fixed32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed fixed32".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_fixed32> get_packed_fixed32() {
        return get<types::packed<types::_fixed32>>();
    }

    /**
     * Consume current "repeated packed sfixed32" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sfixed32".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_sfixed32> get_packed_sfixed32() {
        return get<types::packed<types::_sfixed32>>();
    }

    /**
     * Consume current "repeated packed fixed64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed fixed64".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_fixed64> get_packed_fixed64() {
        return get<types::packed<types::_fixed64>>();
    }

    /**
     * Consume current "repeated packed sfixed64" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed sfixed64".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_sfixed64> get_packed_sfixed64() {
        return get<types::packed<types::_sfixed64>>();
    }

    /**
     * Consume current "repeated packed float" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed float".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_float> get_packed_float() {
        return get<types::packed<types::_float>>();
    }

    /**
     * Consume current "repeated packed double" field.
     *
     * @returns a pair of iterators to the beginning and one past the end of
     *          the data.
     * @pre There must be a current field (ie. next() must have returned `true`).
     * @pre The current field must be of type "repeated packed double".
     * @post The current field was consumed and there is no current field now.
     */
    iterator_range_over<types::_double> get_packed_double() {
        return get<types::packed<types::_double>>();
    }

    ///@}

}; // class pbf_reader

} // end namespace protozero

#endif // PROTOZERO_PBF_READER_HPP

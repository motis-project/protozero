#ifndef PROTOZERO_PBF_WRITER_HPP
#define PROTOZERO_PBF_WRITER_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

/**
 * @file pbf_writer.hpp
 *
 * @brief Contains the pbf_writer class.
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>

#include <protozero/config.hpp>
#include <protozero/types.hpp>
#include <protozero/varint.hpp>

#if PROTOZERO_BYTE_ORDER != PROTOZERO_LITTLE_ENDIAN
# include <protozero/byteswap.hpp>
#endif

namespace protozero {

/**
 * The pbf_writer is used to write PBF formatted messages into a buffer.
 *
 * Almost all methods in this class can throw an std::bad_alloc exception if
 * the std::string used as a buffer wants to resize.
 */
class pbf_writer {

    // A pointer to a string buffer holding the data already written to the
    // PBF message. For default constructed writers or writers that have been
    // rolled back, this is a nullptr.
    std::string* m_data;

    // A pointer to a parent writer object if this is a submessage. If this
    // is a top-level writer, it is a nullptr.
    pbf_writer* m_parent_writer;

    // This is usually 0. If there is an open submessage, this is set in the
    // parent to the rollback position, ie. the last position before the
    // submessage was started. This is the position where the header of the
    // submessage starts.
    std::size_t m_rollback_pos = 0;

    // This is usually 0. If there is an open submessage, this is set in the
    // parent to the position where the data of the submessage is written to.
    std::size_t m_pos = 0;

    void add_varint(uint64_t value) {
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
        write_varint(std::back_inserter(*m_data), value);
    }

    void add_field(pbf_tag_type tag, pbf_wire_type type) {
        protozero_assert(((tag > 0 && tag < 19000) || (tag > 19999 && tag <= ((1 << 29) - 1))) && "tag out of range");
        const uint32_t b = (tag << 3) | uint32_t(type);
        add_varint(b);
    }

    template <typename T>
    void add_fixed(T value) {
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
#if PROTOZERO_BYTE_ORDER == PROTOZERO_LITTLE_ENDIAN
        m_data->append(reinterpret_cast<const char*>(&value), sizeof(T));
#else
        const auto size = m_data->size();
        m_data->resize(size + sizeof(T));
        byteswap<sizeof(T)>(reinterpret_cast<const char*>(&value), const_cast<char*>(m_data->data() + size));
#endif
    }

    // The number of bytes to reserve for the varint holding the length of
    // a length-delimited field. The length has to fit into pbf_length_type,
    // and a varint needs 8 bit for every 7 bit.
    static const int reserve_bytes = sizeof(pbf_length_type) * 8 / 7 + 1;

    // If m_rollpack_pos is set to this special value, it means that when
    // the submessage is closed, nothing needs to be done, because the length
    // of the submessage has already been written correctly.
    static const std::size_t size_is_known = std::numeric_limits<std::size_t>::max();

    void open_submessage(pbf_tag_type tag, std::size_t size) {
        protozero_assert(m_pos == 0);
        protozero_assert(m_data);
        if (size == 0) {
            m_rollback_pos = m_data->size();
            add_field(tag, pbf_wire_type::length_delimited);
            m_data->append(std::size_t(reserve_bytes), '\0');
        } else {
            m_rollback_pos = size_is_known;
            add_length_varint(tag, pbf_length_type(size));
            reserve(size);
        }
        m_pos = m_data->size();
    }

    void rollback_submessage() {
        protozero_assert(m_pos != 0);
        protozero_assert(m_rollback_pos != size_is_known);
        protozero_assert(m_data);
        m_data->resize(m_rollback_pos);
        m_pos = 0;
    }

    void commit_submessage() {
        protozero_assert(m_pos != 0);
        protozero_assert(m_rollback_pos != size_is_known);
        protozero_assert(m_data);
        const auto length = pbf_length_type(m_data->size() - m_pos);

        protozero_assert(m_data->size() >= m_pos - reserve_bytes);
        const auto n = write_varint(m_data->begin() + long(m_pos) - reserve_bytes, length);

        m_data->erase(m_data->begin() + long(m_pos) - reserve_bytes + n, m_data->begin() + long(m_pos));
        m_pos = 0;
    }

    void close_submessage() {
        protozero_assert(m_data);
        if (m_pos == 0 || m_rollback_pos == size_is_known) {
            return;
        }
        if (m_data->size() - m_pos == 0) {
            rollback_submessage();
        } else {
            commit_submessage();
        }
    }

    void add_length_varint(pbf_tag_type tag, pbf_length_type length) {
        add_field(tag, pbf_wire_type::length_delimited);
        add_varint(length);
    }

    template <typename T>
    void add_impl(T value, varint_tag) {
        write_varint(std::back_inserter(*m_data), uint64_t(value));
    }

    template <typename T>
    void add_impl(T value, bool_tag) {
        m_data->append(1, value);
    }

    template <typename T>
    void add_impl(T value, svarint_tag) {
        write_varint(std::back_inserter(*m_data), uint64_t(encode_zigzag64(value)));
    }

    template <typename T>
    void add_impl(T value, fixed_tag) {
        add_fixed<T>(value);
    }

    template <typename T>
    void add_impl(const data_view& value, length_delimited_tag) {
        protozero_assert(value.size() <= std::numeric_limits<pbf_length_type>::max());
        add_varint(pbf_length_type(value.size()));
        m_data->append(value.data(), value.size());
    }

    template <typename T, typename I, typename G, typename C, typename Enable = void>
    struct add_packed_impl {
    };

    template <typename T, typename I, typename G, typename C>
    struct add_packed_impl<T, I, G, C, typename std::enable_if<!fixed_length<G, C>::value>::type> {
        static void apply(pbf_writer& writer, pbf_tag_type tag, I first, I last) {
            pbf_writer sw(writer, tag);

            while (first != last) {
                sw.add_impl<typename T::type>(*first++, typename T::tag{});
            }
        }
    };

    template <typename T, typename I, typename G, typename C>
    struct add_packed_impl<T, I, G, C, typename std::enable_if<fixed_length<G, C>::value>::type> {
        static void apply(pbf_writer& writer, pbf_tag_type tag, I first, I last) {
            const auto length = std::distance(first, last);
            writer.add_length_varint(tag, sizeof(typename T::type) * pbf_length_type(length));
            writer.reserve(sizeof(typename T::type) * std::size_t(length));

            while (first != last) {
                writer.add_impl<typename T::type>(*first++, typename T::tag{});
            }
        }
    };

public:

    /**
     * Create a writer using the given string as a data store. The pbf_writer
     * stores a reference to that string and adds all data to it. The string
     * doesn't have to be empty. The pbf_writer will just append data.
     */
    explicit pbf_writer(std::string& data) noexcept :
        m_data(&data),
        m_parent_writer(nullptr),
        m_pos(0) {
    }

    /**
     * Create a writer without a data store. In this form the writer can not
     * be used!
     */
    pbf_writer() noexcept :
        m_data(nullptr),
        m_parent_writer(nullptr),
        m_pos(0) {
    }

    /**
     * Construct a pbf_writer for a submessage from the pbf_writer of the
     * parent message.
     *
     * @param parent_writer The pbf_writer
     * @param tag Tag (field number) of the field that will be written
     * @param size Optional size of the submessage in bytes (use 0 for unknown).
     *        Setting this allows some optimizations but is only possible in
     *        a few very specific cases.
     */
    pbf_writer(pbf_writer& parent_writer, pbf_tag_type tag, std::size_t size=0) :
        m_data(parent_writer.m_data),
        m_parent_writer(&parent_writer),
        m_pos(0) {
        m_parent_writer->open_submessage(tag, size);
    }

    /// A pbf_writer object can be copied
    pbf_writer(const pbf_writer&) noexcept = default;

    /// A pbf_writer object can be copied
    pbf_writer& operator=(const pbf_writer&) noexcept = default;

    /// A pbf_writer object can be moved
    pbf_writer(pbf_writer&&) noexcept = default;

    /// A pbf_writer object can be moved
    pbf_writer& operator=(pbf_writer&&) noexcept = default;

    ~pbf_writer() {
        if (m_parent_writer) {
            m_parent_writer->close_submessage();
        }
    }

    /**
     * Reserve size bytes in the underlying message store in addition to
     * whatever the message store already holds. So unlike
     * the `std::string::reserve()` method this is not an absolute size,
     * but additional memory that should be reserved.
     *
     * @param size Number of bytes to reserve in underlying message store.
     */
    void reserve(std::size_t size) {
        protozero_assert(m_data);
        m_data->reserve(m_data->size() + size);
    }

    void rollback() {
        protozero_assert(m_parent_writer && "you can't call rollback() on a pbf_writer without a parent");
        protozero_assert(m_pos == 0 && "you can't call rollback() on a pbf_writer that has an open nested submessage");
        m_parent_writer->rollback_submessage();
        m_data = nullptr;
    }

    // parameters are two iterators -> write to packed
    template <typename T, typename I>
    void add(pbf_tag_type tag, I&& first, I&& last) {
        using category = typename std::iterator_traits<typename std::remove_reference<I>::type>::iterator_category;

        if (first == last) {
            return;
        }
        add_packed_impl<typename T::scalar_type, I, typename T::tag, category>::apply(*this, tag, std::forward<I>(first), std::forward<I>(last));
    }

    // parameter is scalar or enum type -> write to scalar
    template <typename T, typename V, typename std::enable_if<std::is_same<typename detail::traits<T>::tag, detail::is_scalar>::value>::type* = nullptr>
    void add(pbf_tag_type tag, V&& value) {
        add_field(tag, T::wire_type);
        protozero_assert(m_pos == 0 && "you can't add fields to a parent pbf_writer if there is an existing pbf_writer for a submessage");
        protozero_assert(m_data);
        add_impl<typename T::type>(std::forward<V>(value), typename T::tag{});
    }

    // parameter is container type -> write to packed
    template <typename T, typename C, typename V = typename C::value_type, typename std::enable_if<std::is_same<typename T::type, V>::value>::type* = nullptr>
    void add(pbf_tag_type tag, const C& value) {
        add<T>(tag, std::begin(value), std::end(value));
    }

    ///@{
    /**
     * @name Scalar field writer functions
     */

    /**
     * Add "bool" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_bool(pbf_tag_type tag, bool value) {
        add<types::_bool>(tag, value);
    }

    /**
     * Add "enum" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_enum(pbf_tag_type tag, int32_t value) {
        add<types::_enum>(tag, value);
    }

    /**
     * Add "int32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_int32(pbf_tag_type tag, int32_t value) {
        add<types::_int32>(tag, value);
    }

    /**
     * Add "sint32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_sint32(pbf_tag_type tag, int32_t value) {
        add<types::_sint32>(tag, value);
    }

    /**
     * Add "uint32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_uint32(pbf_tag_type tag, uint32_t value) {
        add<types::_uint32>(tag, value);
    }

    /**
     * Add "int64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_int64(pbf_tag_type tag, int64_t value) {
        add<types::_int64>(tag, value);
    }

    /**
     * Add "sint64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_sint64(pbf_tag_type tag, int64_t value) {
        add<types::_sint64>(tag, value);
    }

    /**
     * Add "uint64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_uint64(pbf_tag_type tag, uint64_t value) {
        add<types::_uint64>(tag, value);
    }

    /**
     * Add "fixed32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_fixed32(pbf_tag_type tag, uint32_t value) {
        add<types::_fixed32>(tag, value);
    }

    /**
     * Add "sfixed32" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_sfixed32(pbf_tag_type tag, int32_t value) {
        add<types::_sfixed32>(tag, value);
    }

    /**
     * Add "fixed64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_fixed64(pbf_tag_type tag, uint64_t value) {
        add<types::_fixed64>(tag, value);
    }

    /**
     * Add "sfixed64" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_sfixed64(pbf_tag_type tag, int64_t value) {
        add<types::_sfixed64>(tag, value);
    }

    /**
     * Add "float" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_float(pbf_tag_type tag, float value) {
        add<types::_float>(tag, value);
    }

    /**
     * Add "double" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_double(pbf_tag_type tag, double value) {
        add<types::_double>(tag, value);
    }

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to value to be written
     * @param size Number of bytes to be written
     */
    void add_bytes(pbf_tag_type tag, const char* value, std::size_t size) {
        add<types::_bytes>(tag, data_view{value, size});
    }

    /**
     * Add "bytes" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_bytes(pbf_tag_type tag, const std::string& value) {
        add<types::_bytes>(tag, data_view{value.data(), value.size()});
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to value to be written
     * @param size Number of bytes to be written
     */
    void add_string(pbf_tag_type tag, const char* value, std::size_t size) {
        add<types::_string>(tag, data_view{value, size});
    }

    /**
     * Add "string" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written
     */
    void add_string(pbf_tag_type tag, const std::string& value) {
        add<types::_string>(tag, data_view{value.data(), value.size()});
    }

    /**
     * Add "string" field to data. Bytes from the value are written until
     * a null byte is encountered. The null byte is not added.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to value to be written
     */
    void add_string(pbf_tag_type tag, const char* value) {
        add<types::_string>(tag, data_view{value, std::strlen(value)});
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Pointer to message to be written
     * @param size Length of the message
     */
    void add_message(pbf_tag_type tag, const char* value, std::size_t size) {
        add<types::_message>(tag, data_view{value, size});
    }

    /**
     * Add "message" field to data.
     *
     * @param tag Tag (field number) of the field
     * @param value Value to be written. The value must be a complete message.
     */
    void add_message(pbf_tag_type tag, const std::string& value) {
        add<types::_message>(tag, data_view{value.data(), value.size()});
    }

    ///@}

    ///@{
    /**
     * @name Repeated packed field writer functions
     */

    /**
     * Add "repeated packed bool" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to bool.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_bool(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_bool>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed enum" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_enum(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_enum>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed int32" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_int32(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_int32>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed sint32" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_sint32(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_sint32>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed uint32" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_uint32(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_uint32>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed int64" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_int64(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_int64>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed sint64" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_sint64(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_sint64>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed uint64" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_uint64(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_uint64>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed fixed32" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_fixed32(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_fixed32>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed sfixed32" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int32_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_sfixed32(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_sfixed32>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed fixed64" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to uint64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_fixed64(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_fixed64>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed sfixed64" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to int64_t.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_sfixed64(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_sfixed64>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed float" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to float.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_float(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_float>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    /**
     * Add "repeated packed double" field to data.
     *
     * @tparam InputIterator A type satisfying the InputIterator concept.
     *         Dereferencing the iterator must yield a type assignable to double.
     * @param tag Tag (field number) of the field
     * @param first Iterator pointing to the beginning of the data
     * @param last Iterator pointing one past the end of data
     */
    template <typename InputIterator>
    void add_packed_double(pbf_tag_type tag, InputIterator&& first, InputIterator&& last) {
        add<types::packed<types::_double>>(tag, std::forward<InputIterator>(first), std::forward<InputIterator>(last));
    }

    ///@}

    template <typename T> friend class detail::packed_field_varint;
    template <typename T> friend class detail::packed_field_svarint;
    template <typename T> friend class detail::packed_field_fixed;
    template <typename T, int N> friend class detail::packed_field_varint_tag;
    template <typename T, int N> friend class detail::packed_field_svarint_tag;
    template <typename T, int N> friend class detail::packed_field_fixed_tag;

}; // class pbf_writer

namespace detail {

    class packed_field {

    protected:

        pbf_writer m_writer;

    public:

        packed_field(pbf_writer& parent_writer, pbf_tag_type tag) :
            m_writer(parent_writer, tag) {
        }

        packed_field(pbf_writer& parent_writer, pbf_tag_type tag, std::size_t size) :
            m_writer(parent_writer, tag, size) {
        }

        void rollback() {
            m_writer.rollback();
        }

    }; // class packed_field

    template <typename T>
    class packed_field_fixed : public packed_field {

    public:

        packed_field_fixed(pbf_writer& parent_writer, pbf_tag_type tag) :
            packed_field(parent_writer, tag) {
        }

        packed_field_fixed(pbf_writer& parent_writer, pbf_tag_type tag, std::size_t size) :
            packed_field(parent_writer, tag, size * sizeof(T)) {
        }

        void add_element(T value) {
            m_writer.add_fixed<T>(value);
        }

    }; // class packed_field_fixed

    template <typename T, int N>
    class packed_field_fixed_tag : public packed_field {

    public:

        packed_field_fixed_tag(pbf_writer& parent_writer) :
            packed_field(parent_writer, N) {
        }

        packed_field_fixed_tag(pbf_writer& parent_writer, std::size_t size) :
            packed_field(parent_writer, N, size * sizeof(T)) {
        }

        void add_element(T value) {
            m_writer.add_fixed<T>(value);
        }

    }; // class packed_field_fixed_tag

    template <typename T>
    class packed_field_varint : public packed_field {

    public:

        packed_field_varint(pbf_writer& parent_writer, pbf_tag_type tag) :
            packed_field(parent_writer, tag) {
        }

        void add_element(T value) {
            m_writer.add_varint(uint64_t(value));
        }

    }; // class packed_field_varint

    template <typename T, int N>
    class packed_field_varint_tag : public packed_field {

    public:

        packed_field_varint_tag(pbf_writer& parent_writer) :
            packed_field(parent_writer, N) {
        }

        packed_field_varint_tag(pbf_writer& parent_writer, std::size_t) :
            packed_field(parent_writer, N) {
        }

        void add_element(T value) {
            m_writer.add_varint(uint64_t(value));
        }

    }; // class packed_field_varint_tag

    template <typename T>
    class packed_field_svarint : public packed_field {

    public:

        packed_field_svarint(pbf_writer& parent_writer, pbf_tag_type tag) :
            packed_field(parent_writer, tag) {
        }

        void add_element(T value) {
            m_writer.add_varint(encode_zigzag64(value));
        }

    }; // class packed_field_svarint

    template <typename T, int N>
    class packed_field_svarint_tag : public packed_field {

    public:

        packed_field_svarint_tag(pbf_writer& parent_writer) :
            packed_field(parent_writer, N) {
        }

        packed_field_svarint_tag(pbf_writer& parent_writer, std::size_t) :
            packed_field(parent_writer, N) {
        }

        void add_element(T value) {
            m_writer.add_varint(encode_zigzag64(value));
        }

    }; // class packed_field_svarint_tag

} // end namespace detail

using packed_field_bool     = detail::packed_field_varint<bool>;
using packed_field_enum     = detail::packed_field_varint<int32_t>;
using packed_field_int32    = detail::packed_field_varint<int32_t>;
using packed_field_sint32   = detail::packed_field_svarint<int32_t>;
using packed_field_uint32   = detail::packed_field_varint<uint32_t>;
using packed_field_int64    = detail::packed_field_varint<int64_t>;
using packed_field_sint64   = detail::packed_field_svarint<int64_t>;
using packed_field_uint64   = detail::packed_field_varint<uint64_t>;
using packed_field_fixed32  = detail::packed_field_fixed<uint32_t>;
using packed_field_sfixed32 = detail::packed_field_fixed<int32_t>;
using packed_field_fixed64  = detail::packed_field_fixed<uint64_t>;
using packed_field_sfixed64 = detail::packed_field_fixed<int64_t>;
using packed_field_float    = detail::packed_field_fixed<float>;
using packed_field_double   = detail::packed_field_fixed<double>;

} // end namespace protozero

#endif // PROTOZERO_PBF_WRITER_HPP

#ifndef PROTOZERO_MESSAGE_HPP
#define PROTOZERO_MESSAGE_HPP

/*****************************************************************************

protozero - Minimalistic protocol buffer decoder and encoder in C++.

This file is from https://github.com/mapbox/protozero where you can find more
documentation.

*****************************************************************************/

#include <cstdint>
#include <utility>
#include <string>
#include <type_traits>

#include <protozero/types.hpp>
#include <protozero/pbf_reader.hpp>
#include <protozero/pbf_writer.hpp>

namespace protozero {

template <typename T>
class message : public pbf_reader {

public:

    using type = T;

    template <typename... Args>
    message(Args&&... args) noexcept :
        pbf_reader(std::forward<Args>(args)...) {
    }

}; // end class message

template <typename T>
class builder : public pbf_writer {

public:

    using type = T;

    builder(std::string& data) noexcept :
        pbf_writer(data) {
    }

    builder(pbf_writer& parent_writer, pbf_tag_type tag) noexcept :
        pbf_writer(parent_writer, tag) {
    }

}; // end class builder

} // end namespace protozero

#define PROTOZERO_CONCAT2(x, y)                       x##y
#define PROTOZERO_CONCAT(x, y)                        PROTOZERO_CONCAT2(x, y)
#define PROTOZERO_TYPE_NAME(_type)                    PROTOZERO_CONCAT(::protozero::types::_, _type)

#define PROTOZERO_MESSAGE(_name) struct _name
#define PROTOZERO_FIELD(_type, _name, _tag)           PROTOZERO_TYPE_NAME(_type) _name[_tag]
#define PROTOZERO_PACKED_FIELD(_type, _name, _tag)    ::protozero::types::packed<PROTOZERO_TYPE_NAME(_type)> _name[_tag]
#define PROTOZERO_MESSAGE_FIELD(_type, _name, _tag)   ::protozero::types::message_wrap<_type> _name[_tag]
#define PROTOZERO_ENUM_FIELD(_type, _name, _tag)      ::protozero::types::enum_wrap<_type> _name[_tag]

#define PROTOZERO_FIELD_TYPE_AND_TAG(_message, _name) decltype(decltype(_message)::type::_name)
#define PROTOZERO_FIELD_TYPE(_message, _name)         std::remove_extent<PROTOZERO_FIELD_TYPE_AND_TAG(_message, _name)>::type

#define FIELD_TAG(_message, _name)                    (std::extent<PROTOZERO_FIELD_TYPE_AND_TAG(_message, _name)>::value)
#define FIELD_VALUE(_message, _name)                  (_message.get<PROTOZERO_FIELD_TYPE(_message, _name)>())

#define ADD_FIELD(_message, _name, ...)               (_message.add<PROTOZERO_FIELD_TYPE(_message, _name)>(FIELD_TAG(_message, _name), __VA_ARGS__))

#define PACKED_FIELD_TYPE(_message, _name)            protozero::detail::packed_type<PROTOZERO_FIELD_TYPE(_message, _name)::tag>::type<PROTOZERO_FIELD_TYPE(_message, _name)::type, FIELD_TAG(_message, _name)>

#endif // PROTOZERO_MESSAGE_HPP

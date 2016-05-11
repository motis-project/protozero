
#define PBF_TYPE_NAME PROTOZERO_TEST_STRING(PBF_TYPE)
#define GET_TYPE PROTOZERO_TEST_CONCAT(get_, PBF_TYPE)
#define ADD_TYPE PROTOZERO_TEST_CONCAT(add_, PBF_TYPE)

#include <protozero/types.hpp>

namespace TestScalar {

    PROTOZERO_MESSAGE(Test) {
        PROTOZERO_FIELD(PBF_TYPE, field, 1);
    };

}

TEST_CASE("read field using raw reader: " PBF_TYPE_NAME) {

    SECTION("zero") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-zero");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.GET_TYPE() == 0);
        REQUIRE(!item.next());
    }

    SECTION("positive") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-pos");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.GET_TYPE() == 1);
        REQUIRE(!item.next());
    }

    SECTION("pos200") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-pos200");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.GET_TYPE() == 200);
        REQUIRE(!item.next());
    }

    SECTION("max") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-max");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.GET_TYPE() == std::numeric_limits<cpp_type>::max());
        REQUIRE(!item.next());
    }

#if PBF_TYPE_IS_SIGNED
    SECTION("negative") {
        if (std::is_signed<cpp_type>::value) {
            const std::string buffer = load_data(PBF_TYPE_NAME "/data-neg");

            protozero::pbf_reader item(buffer);

            REQUIRE(item.next());
            REQUIRE(item.GET_TYPE() == -1);
            REQUIRE(!item.next());
        }
    }

    SECTION("neg200") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-neg200");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.GET_TYPE() == -200);
        REQUIRE(!item.next());
    }

    SECTION("min") {
        if (std::is_signed<cpp_type>::value) {
            const std::string buffer = load_data(PBF_TYPE_NAME "/data-min");

            protozero::pbf_reader item(buffer);

            REQUIRE(item.next());
            REQUIRE(item.GET_TYPE() == std::numeric_limits<cpp_type>::min());
            REQUIRE(!item.next());
        }
    }
#endif

    SECTION("end_of_buffer") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-max");

        for (std::string::size_type i = 1; i < buffer.size(); ++i) {
            protozero::pbf_reader item(buffer.data(), i);
            REQUIRE(item.next());
            REQUIRE_THROWS_AS(item.GET_TYPE(), protozero::end_of_buffer_exception);
        }
    }

}

TEST_CASE("read field using message: " PBF_TYPE_NAME) {

    SECTION("zero") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-zero");

        protozero::message<TestScalar::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, field) == 0);
        REQUIRE(!item.next());
    }

    SECTION("positive") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-pos");

        protozero::message<TestScalar::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, field) == 1);
        REQUIRE(!item.next());
    }

    SECTION("pos200") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-pos200");

        protozero::message<TestScalar::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, field) == 200);
        REQUIRE(!item.next());
    }

    SECTION("max") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-max");

        protozero::message<TestScalar::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, field) == std::numeric_limits<cpp_type>::max());
        REQUIRE(!item.next());
    }

#if PBF_TYPE_IS_SIGNED
    SECTION("negative") {
        if (std::is_signed<cpp_type>::value) {
            const std::string buffer = load_data(PBF_TYPE_NAME "/data-neg");

            protozero::message<TestScalar::Test> item(buffer);

            REQUIRE(item.next());
            REQUIRE(FIELD_VALUE(item, field) == -1);
            REQUIRE(!item.next());
        }
    }

    SECTION("neg200") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-neg200");

        protozero::message<TestScalar::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, field) == -200);
        REQUIRE(!item.next());
    }

    SECTION("min") {
        if (std::is_signed<cpp_type>::value) {
            const std::string buffer = load_data(PBF_TYPE_NAME "/data-min");

            protozero::message<TestScalar::Test> item(buffer);

            REQUIRE(item.next());
            REQUIRE(FIELD_VALUE(item, field) == std::numeric_limits<cpp_type>::min());
            REQUIRE(!item.next());
        }
    }
#endif

    SECTION("end_of_buffer") {
        const std::string buffer = load_data(PBF_TYPE_NAME "/data-max");

        for (std::string::size_type i = 1; i < buffer.size(); ++i) {
            protozero::message<TestScalar::Test> item(buffer.data(), i);
            REQUIRE(item.next());
            REQUIRE_THROWS_AS(FIELD_VALUE(item, field), protozero::end_of_buffer_exception);
        }
    }

}

TEST_CASE("write field using raw writer: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::pbf_writer pw(buffer);

    SECTION("zero") {
        pw.ADD_TYPE(1, 0);
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-zero"));
    }

    SECTION("positive") {
        pw.ADD_TYPE(1, 1);
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-pos"));
    }

    SECTION("max") {
        pw.ADD_TYPE(1, std::numeric_limits<cpp_type>::max());
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-max"));
    }

#if PBF_TYPE_IS_SIGNED
    SECTION("negative") {
        pw.ADD_TYPE(1, -1);
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-neg"));
    }

    SECTION("min") {
        if (std::is_signed<cpp_type>::value) {
            pw.ADD_TYPE(1, std::numeric_limits<cpp_type>::min());
            REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-min"));
        }
    }
#endif

}

TEST_CASE("write field using builder: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::builder<TestScalar::Test> builder(buffer);

    SECTION("zero") {
        ADD_FIELD(builder, field, cpp_type(0));
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-zero"));
    }

    SECTION("positive") {
        ADD_FIELD(builder, field, cpp_type(1));
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-pos"));
    }

    SECTION("pos200") {
        ADD_FIELD(builder, field, cpp_type(200));
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-pos200"));
    }

    SECTION("max") {
        ADD_FIELD(builder, field, std::numeric_limits<cpp_type>::max());
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-max"));
    }

#if PBF_TYPE_IS_SIGNED
    SECTION("negative") {
        ADD_FIELD(builder, field, cpp_type(-1));
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-neg"));
    }

    SECTION("neg200") {
        ADD_FIELD(builder, field, cpp_type(-200));
        REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-neg200"));
    }

    SECTION("min") {
        if (std::is_signed<cpp_type>::value) {
            ADD_FIELD(builder, field, std::numeric_limits<cpp_type>::min());
            REQUIRE(buffer == load_data(PBF_TYPE_NAME "/data-min"));
        }
    }
#endif

}


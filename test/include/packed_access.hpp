
#define PBF_TYPE_NAME PROTOZERO_TEST_STRING(PBF_TYPE)
#define GET_TYPE PROTOZERO_TEST_CONCAT(get_packed_, PBF_TYPE)
#define ADD_TYPE PROTOZERO_TEST_CONCAT(add_packed_, PBF_TYPE)

namespace TestRepeatedPacked {
    PROTOZERO_MESSAGE(Test) {
        PROTOZERO_PACKED_FIELD(PBF_TYPE, i, 1);
    };
}

using packed_field_type = PROTOZERO_TEST_CONCAT(protozero::packed_field_, PBF_TYPE);

TEST_CASE("read repeated packed field: " PBF_TYPE_NAME) {

    // Run these tests twice, the second time we basically move the data
    // one byte down in the buffer. It doesn't matter how the data or buffer
    // is aligned before that, in at least one of these cases the ints will
    // not be aligned properly. So we test that even in that case the ints
    // will be extracted properly.

    for (std::string::size_type n = 0; n < 2; ++n) {

        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("empty") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));

            protozero::pbf_reader item(abuffer.data() + n, abuffer.size() - n);

            REQUIRE(!item.next());
        }

        SECTION("one") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));

            protozero::pbf_reader item(abuffer.data() + n, abuffer.size() - n);

            REQUIRE(item.next());
            const auto it_range = item.GET_TYPE();
            REQUIRE(!item.next());

            REQUIRE(it_range.begin() != it_range.end());
            REQUIRE(*it_range.begin() == 17);
            REQUIRE(std::next(it_range.begin()) == it_range.end());
        }

        SECTION("many") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            protozero::pbf_reader item(abuffer.data() + n, abuffer.size() - n);

            REQUIRE(item.next());
            const auto it_range = item.GET_TYPE();
            REQUIRE(!item.next());

            auto it = it_range.begin();
            REQUIRE(it != it_range.end());
            REQUIRE(*it++ ==   17);
            REQUIRE(*it++ ==  200);
            REQUIRE(*it++ ==    0);
            REQUIRE(*it++ ==    1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            REQUIRE(*it++ == -200);
            REQUIRE(*it++ ==   -1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::min());
#endif
            REQUIRE(it == it_range.end());
        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::pbf_reader item(abuffer.data() + n, i);
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(item.GET_TYPE(), protozero::end_of_buffer_exception);
            }
        }

    }

}

TEST_CASE("read repeated packed field using message: " PBF_TYPE_NAME) {

    // Run these tests twice, the second time we basically move the data
    // one byte down in the buffer. It doesn't matter how the data or buffer
    // is aligned before that, in at least one of these cases the ints will
    // not be aligned properly. So we test that even in that case the ints
    // will be extracted properly.

    for (std::string::size_type n = 0; n < 2; ++n) {

        std::string abuffer;
        abuffer.reserve(1000);
        abuffer.append(n, '\0');

        SECTION("empty") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));

            protozero::message<TestRepeatedPacked::Test> item(abuffer.data() + n, abuffer.size() - n);

            REQUIRE(!item.next());
        }

        SECTION("one") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));

            protozero::message<TestRepeatedPacked::Test> item(abuffer.data() + n, abuffer.size() - n);

            REQUIRE(item.next());
            const auto it_range = FIELD_VALUE(item, i);
            REQUIRE(!item.next());

            REQUIRE(it_range.begin() != it_range.end());
            REQUIRE(*it_range.begin() == 17);
            REQUIRE(std::next(it_range.begin()) == it_range.end());
        }

        SECTION("many") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            protozero::message<TestRepeatedPacked::Test> item(abuffer.data() + n, abuffer.size() - n);

            REQUIRE(item.next());
            const auto it_range = FIELD_VALUE(item, i);
            REQUIRE(!item.next());

            auto it = it_range.begin();
            REQUIRE(it != it_range.end());
            REQUIRE(*it++ ==   17);
            REQUIRE(*it++ ==  200);
            REQUIRE(*it++ ==    0);
            REQUIRE(*it++ ==    1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            REQUIRE(*it++ == -200);
            REQUIRE(*it++ ==   -1);
            REQUIRE(*it++ == std::numeric_limits<cpp_type>::min());
#endif
            REQUIRE(it == it_range.end());
        }

        SECTION("end_of_buffer") {
            abuffer.append(load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));

            for (std::string::size_type i = 1; i < abuffer.size() - n; ++i) {
                protozero::message<TestRepeatedPacked::Test> item(abuffer.data() + n, i);
                REQUIRE(item.next());
                REQUIRE_THROWS_AS(FIELD_VALUE(item, i), protozero::end_of_buffer_exception);
            }
        }

    }

}

TEST_CASE("write repeated packed field: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::pbf_writer pw(buffer);

    SECTION("empty") {
        cpp_type data[] = { 17 };
        pw.ADD_TYPE(1, std::begin(data), std::begin(data) /* !!!! */);

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));
    }

    SECTION("one") {
        cpp_type data[] = { 17 };
        pw.ADD_TYPE(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("many") {
        cpp_type data[] = {
               17
            , 200
            ,   0
            ,   1
            ,std::numeric_limits<cpp_type>::max()
#if PBF_TYPE_IS_SIGNED
            ,-200
            ,  -1
            ,std::numeric_limits<cpp_type>::min()
#endif
        };
        pw.ADD_TYPE(1, std::begin(data), std::end(data));

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

}

TEST_CASE("write repeated packed field using packed field: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::pbf_writer pw(buffer);

    SECTION("empty - should do rollback") {
        {
            packed_field_type field{pw, 1};
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));
    }

    SECTION("one") {
        {
            packed_field_type field{pw, 1};
            field.add_element(cpp_type(17));
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("many") {
        {
            packed_field_type field{pw, 1};
            field.add_element(cpp_type(  17));
            field.add_element(cpp_type( 200));
            field.add_element(cpp_type(   0));
            field.add_element(cpp_type(   1));
            field.add_element(std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            field.add_element(cpp_type(-200));
            field.add_element(cpp_type(  -1));
            field.add_element(std::numeric_limits<cpp_type>::min());
#endif
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

    SECTION("failure when not closing properly after zero elements") {
        packed_field_type field{pw, 1};
        REQUIRE_THROWS_AS({
            pw.add_fixed32(2, 1234); // dummy values
        }, assert_error);
    }

    SECTION("failure when not closing properly after one element") {
        packed_field_type field{pw, 1};
        field.add_element(cpp_type(17));
        REQUIRE_THROWS_AS({
            pw.add_fixed32(2, 1234); // dummy values
        }, assert_error);
    }

}

TEST_CASE("write repeated packed field using builder: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::builder<TestRepeatedPacked::Test> builder(buffer);

    SECTION("one with container") {
        {
            std::vector<cpp_type> v = { 17 };
            ADD_FIELD(builder, i, v);
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("one with iterator") {
        {
            std::vector<cpp_type> v = { 17 };
            ADD_FIELD(builder, i, v.cbegin(), v.cend());
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("many with container") {
        {
            std::vector<cpp_type> v = {
                   17
                , 200
                ,   0
                ,   1
                ,std::numeric_limits<cpp_type>::max()
#if PBF_TYPE_IS_SIGNED
                ,-200
                ,  -1
                ,std::numeric_limits<cpp_type>::min()
#endif
            };
            ADD_FIELD(builder, i, v);
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

    SECTION("many with iterator") {
        {
            std::vector<cpp_type> v = {
                   17
                , 200
                ,   0
                ,   1
                ,std::numeric_limits<cpp_type>::max()
#if PBF_TYPE_IS_SIGNED
                ,-200
                ,  -1
                ,std::numeric_limits<cpp_type>::min()
#endif
            };
            ADD_FIELD(builder, i, v.cbegin(), v.cend());
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

}

TEST_CASE("write repeated packed field using PACKED_FIELD and builder: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::builder<TestRepeatedPacked::Test> builder(buffer);

    SECTION("empty - should do rollback") {
        {
            PACKED_FIELD_TYPE(builder, i) field{builder};
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-empty"));
    }

    SECTION("one") {
        {
            PACKED_FIELD_TYPE(builder, i) field{builder};
            field.add_element(cpp_type(17));
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("one with predefined size") {
        {
            PACKED_FIELD_TYPE(builder, i) field{builder, 1};
            field.add_element(cpp_type(17));
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-one"));
    }

    SECTION("many") {
        {
            PACKED_FIELD_TYPE(builder, i) field{builder};
            field.add_element(cpp_type( 17));
            field.add_element(cpp_type(200));
            field.add_element(cpp_type(  0));
            field.add_element(cpp_type(  1));
            field.add_element(std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            field.add_element(cpp_type(-200));
            field.add_element(cpp_type(  -1));
            field.add_element(std::numeric_limits<cpp_type>::min());
#endif
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

    SECTION("many with predefined size") {
        {
            PACKED_FIELD_TYPE(builder, i) field{builder,
#if PBF_TYPE_IS_SIGNED
                8
#else
                5
#endif
            };
            field.add_element(cpp_type( 17));
            field.add_element(cpp_type(200));
            field.add_element(cpp_type(  0));
            field.add_element(cpp_type(  1));
            field.add_element(std::numeric_limits<cpp_type>::max());
#if PBF_TYPE_IS_SIGNED
            field.add_element(cpp_type(-200));
            field.add_element(cpp_type(  -1));
            field.add_element(std::numeric_limits<cpp_type>::min());
#endif
        }

        REQUIRE(buffer == load_data("repeated_packed_" PBF_TYPE_NAME "/data-many"));
    }

    SECTION("failure when not closing properly after zero elements") {
        PACKED_FIELD_TYPE(builder, i) field{builder};
        REQUIRE_THROWS_AS({
            builder.add_fixed32(2, 1234); // dummy values
        }, assert_error);
    }

    SECTION("failure when not closing properly after one element") {
        PACKED_FIELD_TYPE(builder, i) field{builder};
        field.add_element(cpp_type(17));
        REQUIRE_THROWS_AS({
            builder.add_fixed32(2, 1234); // dummy values
        }, assert_error);
    }

}

TEST_CASE("write from different types of iterators: " PBF_TYPE_NAME) {

    std::string buffer;
    protozero::pbf_writer pw(buffer);

    SECTION("from uint16_t") {
#if PBF_TYPE_IS_SIGNED
        const  int16_t data[] = { 1, 4, 9, 16, 25 };
#else
        const uint16_t data[] = { 1, 4, 9, 16, 25 };
#endif

        pw.ADD_TYPE(1, std::begin(data), std::end(data));
    }

    SECTION("from string") {
        std::string data = "1 4 9 16 25";
        std::stringstream sdata(data);

#if PBF_TYPE_IS_SIGNED
        using test_type =  int32_t;
#else
        using test_type = uint32_t;
#endif

        std::istream_iterator<test_type> eod;
        std::istream_iterator<test_type> it(sdata);

        pw.ADD_TYPE(1, it, eod);
    }

    protozero::pbf_reader item(buffer);

    REQUIRE(item.next());
    const auto it_range = item.GET_TYPE();
    REQUIRE(!item.next());
    REQUIRE(std::distance(it_range.begin(), it_range.end()) == 5);

    auto it = it_range.begin();
    REQUIRE(*it++ ==  1);
    REQUIRE(*it++ ==  4);
    REQUIRE(*it++ ==  9);
    REQUIRE(*it++ == 16);
    REQUIRE(*it++ == 25);
    REQUIRE(it == it_range.end());
}


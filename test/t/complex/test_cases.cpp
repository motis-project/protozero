
#include <test.hpp>

namespace TestComplex {

PROTOZERO_MESSAGE(Sub) {
    PROTOZERO_FIELD(string, s, 1);
};

PROTOZERO_MESSAGE(Test) {
    PROTOZERO_FIELD(fixed32, f, 1);
    PROTOZERO_FIELD(int64, i, 2);
    PROTOZERO_FIELD(int64, j, 3);
    PROTOZERO_MESSAGE_FIELD(Sub, submessage, 5);
    PROTOZERO_FIELD(string, s, 8);
    PROTOZERO_FIELD(uint32, u, 4);
    PROTOZERO_PACKED_FIELD(sint32, d, 7);
};

} // end namespace TestComplex

TEST_CASE("read complex data using pbf_reader") {

    SECTION("minimal") {
        const std::string buffer = load_data("complex/data-minimal");

        protozero::pbf_reader item(buffer);

        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
    }

    SECTION("some") {
        const std::string buffer = load_data("complex/data-some");

        protozero::pbf_reader item(buffer);

        uint32_t sum_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 2: {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case 4: {
                    sum_of_u += item.get_uint32();
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(sum_of_u == 66);
    }

    SECTION("all") {
        const std::string buffer = load_data("complex/data-all");

        protozero::pbf_reader item(buffer);

        int number_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 2: {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case 3: {
                    REQUIRE(item.get_int64() == 555555555LL);
                    break;
                }
                case 4: {
                    item.skip();
                    ++number_of_u;
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                case 7: {
                    const auto pi = item.get_packed_sint32();
                    int32_t sum = 0;
                    for (auto val : pi) {
                        sum += val;
                    }
                    REQUIRE(sum == 5);
                    break;
                }
                case 8: {
                    REQUIRE(item.get_string() == "optionalstring");
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(number_of_u == 5);
    }

    SECTION("skip everything") {
        const std::string buffer = load_data("complex/data-all");

        protozero::pbf_reader item(buffer);

        while (item.next()) {
            switch (item.tag()) {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 7:
                case 8:
                    item.skip();
                    break;
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
    }

}

TEST_CASE("read complex data using message") {

    SECTION("minimal") {
        const std::string buffer = load_data("complex/data-minimal");

        protozero::message<TestComplex::Test> item(buffer);

        while (item.next()) {
            switch (item.tag()) {
                case FIELD_TAG(item, f): {
                    REQUIRE(FIELD_VALUE(item, f) == 12345678L);
                    break;
                }
                case FIELD_TAG(item, submessage): {
                    auto subitem = FIELD_VALUE(item, submessage);
                    REQUIRE(subitem.next());
                    REQUIRE(std::string(FIELD_VALUE(subitem, s)) == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
    }

    SECTION("some") {
        const std::string buffer = load_data("complex/data-some");

        protozero::message<TestComplex::Test> item(buffer);

        uint32_t sum_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case FIELD_TAG(item, f): {
                    REQUIRE(FIELD_VALUE(item, f) == 12345678L);
                    break;
                }
                case FIELD_TAG(item, i): {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case FIELD_TAG(item, u): {
                    sum_of_u += FIELD_VALUE(item, u);
                    break;
                }
                case FIELD_TAG(item, submessage): {
                    auto subitem = FIELD_VALUE(item, submessage);
                    REQUIRE(subitem.next());
                    REQUIRE(std::string(FIELD_VALUE(subitem, s)) == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(sum_of_u == 66);
    }

    SECTION("all") {
        const std::string buffer = load_data("complex/data-all");

        protozero::message<TestComplex::Test> item(buffer);

        int number_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case FIELD_TAG(item, f): {
                    REQUIRE(FIELD_VALUE(item, f) == 12345678L);
                    break;
                }
                case FIELD_TAG(item, i): {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case FIELD_TAG(item, j): {
                    REQUIRE(FIELD_VALUE(item, j) == 555555555LL);
                    break;
                }
                case FIELD_TAG(item, u): {
                    item.skip();
                    ++number_of_u;
                    break;
                }
                case FIELD_TAG(item, submessage): {
                    auto subitem = FIELD_VALUE(item, submessage);
                    REQUIRE(subitem.next());
                    REQUIRE(std::string(FIELD_VALUE(subitem, s)) == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                case FIELD_TAG(item, d): {
                    const auto pi = FIELD_VALUE(item, d);
                    int32_t sum = 0;
                    for (auto val : pi) {
                        sum += val;
                    }
                    REQUIRE(sum == 5);
                    break;
                }
                case FIELD_TAG(item, s): {
                    REQUIRE(std::string(FIELD_VALUE(item, s)) == "optionalstring");
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(number_of_u == 5);
    }

    SECTION("skip everything") {
        const std::string buffer = load_data("complex/data-all");

        protozero::message<TestComplex::Test> item(buffer);

        while (item.next()) {
            switch (item.tag()) {
                case FIELD_TAG(item, f):
                case FIELD_TAG(item, i):
                case FIELD_TAG(item, j):
                case FIELD_TAG(item, u):
                case FIELD_TAG(item, submessage):
                case FIELD_TAG(item, d):
                case FIELD_TAG(item, s):
                    item.skip();
                    break;
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
    }

}

TEST_CASE("write complex data using pbf_writer") {

    SECTION("minimal") {
        std::string buffer;
        protozero::pbf_writer pw(buffer);
        pw.add_fixed32(1, 12345678);

        std::string submessage;
        protozero::pbf_writer pws(submessage);
        pws.add_string(1, "foobar");

        pw.add_message(5, submessage);

        protozero::pbf_reader item(buffer);

        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
    }

    SECTION("some") {
        std::string buffer;
        protozero::pbf_writer pw(buffer);
        pw.add_fixed32(1, 12345678);

        std::string submessage;
        protozero::pbf_writer pws(submessage);
        pws.add_string(1, "foobar");

        pw.add_uint32(4, 22);
        pw.add_uint32(4, 44);
        pw.add_int64(2, -9876543);
        pw.add_message(5, submessage);

        protozero::pbf_reader item(buffer);

        uint32_t sum_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 2: {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case 4: {
                    sum_of_u += item.get_uint32();
                    break;
                }
                case 5: {
                    auto view = item.get_view();
                    protozero::pbf_reader subitem{view};
                    REQUIRE(subitem.next());
                    REQUIRE(std::string(subitem.get_view()) == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(sum_of_u == 66);
    }

    SECTION("all") {
        std::string buffer;
        protozero::pbf_writer pw(buffer);
        pw.add_fixed32(1, 12345678);

        std::string submessage;
        protozero::pbf_writer pws(submessage);
        pws.add_string(1, "foobar");
        pw.add_message(5, submessage);

        pw.add_uint32(4, 22);
        pw.add_uint32(4, 44);
        pw.add_int64(2, -9876543);
        pw.add_uint32(4, 44);
        pw.add_uint32(4, 66);
        pw.add_uint32(4, 66);

        int32_t d[] = { -17, 22 };
        pw.add_packed_sint32(7, std::begin(d), std::end(d));

        pw.add_int64(3, 555555555);

        protozero::pbf_reader item(buffer);

        int number_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 2: {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case 3: {
                    REQUIRE(item.get_int64() == 555555555LL);
                    break;
                }
                case 4: {
                    item.skip();
                    ++number_of_u;
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                case 7: {
                    const auto pi = item.get_packed_sint32();
                    int32_t sum = 0;
                    for (auto val : pi) {
                        sum += val;
                    }
                    REQUIRE(sum == 5);
                    break;
                }
                case 8: {
                    REQUIRE(item.get_string() == "optionalstring");
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(number_of_u == 5);
    }
}

TEST_CASE("write complex data using builder") {

    SECTION("minimal") {
        std::string buffer;
        protozero::builder<TestComplex::Test> builder(buffer);
        ADD_FIELD(builder, f, 12345678u);

        std::string sub_buffer;
        protozero::builder<TestComplex::Sub> sub_builder(sub_buffer);
        ADD_FIELD(sub_builder, s, "foobar");

        ADD_FIELD(builder, submessage, sub_buffer);

        protozero::pbf_reader item(buffer);

        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
    }

    SECTION("some") {
        std::string buffer;
        protozero::builder<TestComplex::Test> builder(buffer);
        ADD_FIELD(builder, f, 12345678u);

        std::string submessage;
        protozero::builder<TestComplex::Sub> sub_builder(submessage);
        ADD_FIELD(sub_builder, s, "foobar");

        ADD_FIELD(builder, u, 22u);
        ADD_FIELD(builder, u, 44u);
        ADD_FIELD(builder, i, -9876543);
        ADD_FIELD(builder, submessage, submessage);

        protozero::pbf_reader item(buffer);

        uint32_t sum_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 2: {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case 4: {
                    sum_of_u += item.get_uint32();
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(sum_of_u == 66);
    }

    SECTION("all") {
        std::string buffer;
        protozero::builder<TestComplex::Test> builder(buffer);
        ADD_FIELD(builder, f, 12345678u);

        std::string submessage;
        protozero::builder<TestComplex::Sub> sub_builder(submessage);
        ADD_FIELD(sub_builder, s, "foobar");
        ADD_FIELD(builder, submessage, submessage);

        ADD_FIELD(builder, u, 22u);
        ADD_FIELD(builder, u, 44u);
        ADD_FIELD(builder, i, -9876543);
        ADD_FIELD(builder, u, 44u);
        ADD_FIELD(builder, u, 66u);
        ADD_FIELD(builder, u, 66u);

        int32_t d[] = { -17, 22 };
        ADD_FIELD(builder, d, std::begin(d), std::end(d));
        ADD_FIELD(builder, j, 555555555);

        protozero::pbf_reader item(buffer);

        int number_of_u = 0;
        while (item.next()) {
            switch (item.tag()) {
                case 1: {
                    REQUIRE(item.get_fixed32() == 12345678L);
                    break;
                }
                case 2: {
                    REQUIRE(true);
                    item.skip();
                    break;
                }
                case 3: {
                    REQUIRE(item.get_int64() == 555555555LL);
                    break;
                }
                case 4: {
                    item.skip();
                    ++number_of_u;
                    break;
                }
                case 5: {
                    protozero::pbf_reader subitem = item.get_message();
                    REQUIRE(subitem.next());
                    REQUIRE(subitem.get_string() == "foobar");
                    REQUIRE(!subitem.next());
                    break;
                }
                case 7: {
                    const auto pi = item.get_packed_sint32();
                    int32_t sum = 0;
                    for (auto val : pi) {
                        sum += val;
                    }
                    REQUIRE(sum == 5);
                    break;
                }
                case 8: {
                    REQUIRE(item.get_string() == "optionalstring");
                    break;
                }
                default: {
                    REQUIRE(false); // should not be here
                    break;
                }
            }
        }
        REQUIRE(number_of_u == 5);
    }

}

static void check_message(const std::string& buffer) {
    protozero::pbf_reader item(buffer);

    while (item.next()) {
        switch (item.tag()) {
            case 1: {
                REQUIRE(item.get_fixed32() == 42L);
                break;
            }
            case 5: {
                protozero::pbf_reader subitem = item.get_message();
                REQUIRE(subitem.next());
                REQUIRE(subitem.get_string() == "foobar");
                REQUIRE(!subitem.next());
                break;
            }
            default: {
                REQUIRE(false); // should not be here
                break;
            }
        }
    }
}

TEST_CASE("write complex with subwriter using pbf_writer") {
    std::string buffer_test;
    protozero::pbf_writer pbf_test(buffer_test);
    pbf_test.add_fixed32(1, 42L);

    SECTION("message in message") {
        protozero::pbf_writer pbf_submessage(pbf_test, 5);
        pbf_submessage.add_string(1, "foobar");
    }

    check_message(buffer_test);
}

TEST_CASE("write complex with subwriter using builder") {
    std::string buffer_test;
    protozero::builder<TestComplex::Test> pbf_test(buffer_test);
    ADD_FIELD(pbf_test, f, 42u);

    SECTION("message in message") {
        protozero::builder<TestComplex::Sub> pbf_submessage(pbf_test, FIELD_TAG(pbf_test, submessage));
        ADD_FIELD(pbf_submessage, s, "foobar");
    }

    check_message(buffer_test);
}



#include <test.hpp>

namespace TestEnum {

    enum Color {
        BLACK = 0,
        RED   = 1,
        GREEN = 2,
        BLUE  = 3
    };

    PROTOZERO_MESSAGE(Test) {
        PROTOZERO_ENUM_FIELD(Color, color, 1);
    };

} // end namespace TestEnum

TEST_CASE("read enum field") {

    SECTION("zero") {
        const std::string buffer = load_data("enum/data-black");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.get_enum() == 0L);
        REQUIRE(!item.next());
    }

    SECTION("positive") {
        const std::string buffer = load_data("enum/data-blue");

        protozero::pbf_reader item(buffer);

        REQUIRE(item.next());
        REQUIRE(item.get_enum() == 3L);
        REQUIRE(!item.next());
    }

}

TEST_CASE("read enum field using message") {

    SECTION("zero") {
        const std::string buffer = load_data("enum/data-black");

        protozero::message<TestEnum::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, color) == TestEnum::Color::BLACK);
        REQUIRE(!item.next());
    }

    SECTION("positive") {
        const std::string buffer = load_data("enum/data-blue");

        protozero::message<TestEnum::Test> item(buffer);

        REQUIRE(item.next());
        REQUIRE(FIELD_VALUE(item, color) == TestEnum::Color::BLUE);
        REQUIRE(!item.next());
    }

}

TEST_CASE("write enum field") {

    std::string buffer;
    protozero::pbf_writer pw(buffer);

    SECTION("zero") {
        pw.add_enum(1, 0L);
        REQUIRE(buffer == load_data("enum/data-black"));
    }

    SECTION("positive") {
        pw.add_enum(1, 3L);
        REQUIRE(buffer == load_data("enum/data-blue"));
    }

}

TEST_CASE("write enum field using builder") {

    std::string buffer;
    protozero::builder<TestEnum::Test> builder(buffer);

    SECTION("zero") {
        ADD_FIELD(builder, color, TestEnum::Color::BLACK);
        REQUIRE(buffer == load_data("enum/data-black"));
    }

    SECTION("positive") {
        ADD_FIELD(builder, color, TestEnum::Color::BLUE);
        REQUIRE(buffer == load_data("enum/data-blue"));
    }

}


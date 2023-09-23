#include <edgelink/edgelink.hpp>
#include <edgelink/propex.hpp>

using namespace edgelink;


TEST_CASE("Test DynamicObjectVariant") {

    auto obj = DynamicObject(std::string("123"));

    REQUIRE(obj.kind() == DynamicObjectKind::STRING);
    REQUIRE(obj.as_string() == "123");
}


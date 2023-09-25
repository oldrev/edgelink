#include <cmath>
#include <edgelink/edgelink.hpp>
#include <edgelink/propex.hpp>
#include <edgelink/variant.hpp>

using namespace edgelink;

TEST_CASE("Variant Class Tests") {
    SECTION("Default constructor creates a null Variant") {
        Variant v;
        REQUIRE(v.is<std::nullptr_t>());
        REQUIRE(v.kind() == Variant::Kind::NULLPTR);
    }

    SECTION("Constructor with value sets the Variant") {
        Variant v(42.0);
        REQUIRE(v.is<double>());
        REQUIRE(fabs(v.get<double>() - 42.0) <= 0.000001);
    }

    SECTION("Constructor with simple object") {
        Variant vobj({{"x", 123LL}, {"y", 333LL}, {"z", 444LL}});
        REQUIRE(vobj.kind() == Variant::Kind::OBJECT);
        // REQUIRE(v.is<double>());
        // REQUIRE(v.is<double>());
        // REQUIRE(fabs(v.get<double>() - 42.0) <= 0.000001);
    }

    SECTION("Copy and move operations") {
        Variant original(42LL);

        SECTION("Copy constructor copies the Variant") {
            Variant copy(original);
            REQUIRE(copy.is<int64_t>());
            REQUIRE(copy.get<int64_t>() == 42);
        }

        SECTION("Move constructor moves the Variant") {
            Variant moved(std::move(original));
            REQUIRE(moved.is<int64_t>());
            REQUIRE(moved.get<int64_t>() == 42);
            REQUIRE(original.is<std::nullptr_t>());
        }

        SECTION("Copy assignment operator copies the Variant") {
            Variant copy;
            copy = original;
            REQUIRE(copy.is<int64_t>());
            REQUIRE(copy.get<int64_t>() == 42);
        }

        SECTION("Move assignment operator moves the Variant") {
            Variant moved;
            moved = std::move(original);
            REQUIRE(moved.is<int64_t>());
            REQUIRE(moved.get<int64_t>() == 42);
            REQUIRE(original.is<std::nullptr_t>());
        }
    }

    SECTION("Equality and inequality operators") {
        Variant v1(42LL);
        Variant v2(42LL);
        Variant v3("Hello");

        SECTION("Equality operator compares Variants correctly") {
            REQUIRE(v1 == v2);
            REQUIRE_FALSE(v1 == v3);
        }

        SECTION("Inequality operator compares Variants correctly") {
            REQUIRE_FALSE(v1 != v2);
            REQUIRE(v1 != v3);
        }
    }

    SECTION("Accessing VariantObject and VariantArray") {
        VariantObject obj = {
            {"key1", 42},
            {"key2", "Hello"},
            {"key3", VariantArray({
                         "AA",
                         "BB",
                         "CC",
                     })},
        };
        Variant v(obj);

        SECTION("Accessing VariantObject works as expected") {
            REQUIRE(v.is<VariantObject>());
            REQUIRE(v.as_object().size() == 3);
            REQUIRE(v.as_object()["key1"].is<int64_t>());
            REQUIRE(v.as_object()["key1"].get<int64_t>() == 42LL);
            REQUIRE(v.as_object()["key2"].is<std::string>());
            REQUIRE(v.as_object()["key2"].get<std::string>() == "Hello");
            REQUIRE(v.as_object()["key3"].is<VariantArray>());
            REQUIRE(v.as_object()["key3"].get<VariantArray>() == VariantArray({"AA", "BB", "CC"}));
        }

        SECTION("Accessing VariantObject via Propex works as expected") {
            REQUIRE(v.at_propex("key1").is<int64_t>());
            REQUIRE(v.at_propex("key1").get<int64_t>() == 42LL);

            REQUIRE(v.at_propex("key3[1]").is<std::string>());
            REQUIRE(v.at_propex("key3[1]").get<std::string>() == "BB");

            v.at_propex("key2") = "World";
            REQUIRE(v.at_propex("key2").get<std::string>() == "World");

            v.at_propex("key3") = std::move(std::string("Test1"));
            const std::string_view sv = v.at_propex("key3").get<std::string>();
            REQUIRE(sv == "Test1");
        }

        VariantArray arr = {1, 2, 3};
        v.set(arr);

        SECTION("Accessing VariantArray works as expected") {
            REQUIRE(arr.size() == 3);
            REQUIRE(v.is<VariantArray>());
            REQUIRE(v.as_array().size() == 3);
            REQUIRE(v.as_array().at(0).is<int64_t>());
            REQUIRE(v.as_array().at(0).get<int64_t>() == 1);
            REQUIRE(v.as_array().at(1).is<int64_t>());
            REQUIRE(v.as_array().at(1).get<int64_t>() == 2);
            REQUIRE(v.as_array().at(2).is<int64_t>());
            REQUIRE(v.as_array().at(2).get<int64_t>() == 3);
        }

        v = arr;
        auto json = v.to_json();
        SECTION("Export to JSON works as expected") {
            auto jarr = json.as_array();
            REQUIRE(jarr.at(0) == 1);
            REQUIRE(jarr.at(1) == 2);
            REQUIRE(jarr.at(2) == 3);
            REQUIRE(v.json_dump() == "[1,2,3]");
        }

    }
}

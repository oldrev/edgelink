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
        VariantObject obj = {{"key1", 42}, {"key2", "Hello"}};
        Variant v(obj);

        SECTION("Accessing VariantObject works as expected") {
            REQUIRE(v.is<VariantObject>());
            REQUIRE(v.get_object().size() == 2);
            REQUIRE(v.get_object()["key1"].is<int64_t>());
            REQUIRE(v.get_object()["key1"].get<int64_t>() == 42LL);
            REQUIRE(v.get_object()["key2"].is<std::string>());
            REQUIRE(v.get_object()["key2"].get<std::string>() == "Hello");
        }

        VariantArray arr = {1, 2, 3};
        v.set(arr);

        SECTION("Accessing VariantArray works as expected") {
            REQUIRE(v.is<VariantArray>());
            REQUIRE(v.get_array().size() == 3);
            REQUIRE(v.get_array()[0].is<int64_t>());
            REQUIRE(v.get_array()[0].get<int64_t>() == 1);
            REQUIRE(v.get_array()[1].is<int64_t>());
            REQUIRE(v.get_array()[1].get<int64_t>() == 2);
            REQUIRE(v.get_array()[2].is<int64_t>());
            REQUIRE(v.get_array()[2].get<int64_t>() == 3);
        }
    }
}

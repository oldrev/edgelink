#include <edgelink/edgelink.hpp>
#include <tao/pegtl.hpp>

using namespace edgelink;

namespace pegtl = tao::pegtl;

namespace pe {

const size_t PROPERTY_SEGMENT_MAX = 16;


struct HexDigit : pegtl::xdigit {};
struct UnicodeChar : pegtl::list<pegtl::seq<pegtl::one<'u'>, pegtl::rep<4, HexDigit>>, pegtl::one<'\\'>> {};
struct EscapedChar : pegtl::one<'"', '\\', '/', 'b', 'f', 'n', 'r', 't'> {};
struct Escaped : pegtl::sor<EscapedChar, UnicodeChar> {};
struct Unescaped : pegtl::utf8::range<0x20, 0x10FFFF> {};
struct Char_ : pegtl::if_then_else<pegtl::one<'\\'>, Escaped, Unescaped> {}; // NOLINT(readability-identifier-naming)

struct StringContent : pegtl::until<pegtl::at<pegtl::one<'"'>>, Char_> {};
struct String : pegtl::seq<pegtl::one<'"'>, StringContent, pegtl::any> {
    using content = StringContent;
};

struct KeyContent : pegtl::until<pegtl::at<pegtl::one<'"'>>, Char_> {};
struct Key : pegtl::seq<pegtl::one<'"'>, KeyContent, pegtl::any> {
    using content = KeyContent;
};

template <char TChar> struct StringContentWithout : pegtl::star<pegtl::not_one<TChar, 10, 13>> {};
struct DoubleQuotedString : pegtl::seq<pegtl::one<'"'>, StringContentWithout<'"'>, pegtl::one<'"'>> {};
struct SingleQuotedString : pegtl::seq<pegtl::one<'\''>, StringContentWithout<'\''>, pegtl::one<'\''>> {};
struct QuotedString : pegtl::sor<DoubleQuotedString, SingleQuotedString> {};

struct Identifier : pegtl::identifier {};
struct Dot : pegtl::one<'.'> {};
struct LBracket : pegtl::one<'['> {};
struct RBracket : pegtl::one<']'> {};
struct ArrayIndexInteger : pegtl::plus<pegtl::digit> {};
struct ArrayIndexExpr : pegtl::seq<LBracket, ArrayIndexInteger, RBracket> {};
struct StringKeyMapIndex : QuotedString {};
struct StringKeyMapIndexExpr : pegtl::seq<LBracket, StringKeyMapIndex, RBracket> {};
struct IndexExpr : pegtl::sor<ArrayIndexExpr, StringKeyMapIndexExpr> {};

struct PropertyExprSegment : pegtl::seq<Identifier, pegtl::star<IndexExpr>> {};

struct PropertyExpr : pegtl::list<PropertyExprSegment, pegtl::one<'.'>> {};

enum class PropertySegmentKindIndex : size_t {
    IDENTIFIER = 0,
    INT_INDEX,
};

using PropertySegment = std::variant<std::string_view, size_t>;
using PropertySegments = std::vector<PropertySegment>;
// boost::container::static_vector<PropertySegment, PROPERTY_SEGMENT_MAX>;

template <typename Rule> struct Action {};

template <> struct Action<Identifier> {
    template <typename ActionInput> static void apply(const ActionInput& in, std::vector<PropertySegment>& data) {
        auto seg_str = std::string_view(in.begin(), in.end());
        auto ps = PropertySegment(std::move(seg_str));
        data.emplace_back(ps);
    }
};

template <> struct Action<ArrayIndexInteger> {
    template <typename ActionInput> static void apply(const ActionInput& in, std::vector<PropertySegment>& data) {
        auto seg_str = std::string_view(in.begin(), in.end());
        auto i = boost::lexical_cast<size_t>(seg_str);
        auto ps = PropertySegment(std::move(i));
        data.emplace_back(ps);
    }
};

template <> struct Action<StringKeyMapIndex> {
    template <typename ActionInput> static void apply(const ActionInput& in, std::vector<PropertySegment>& data) {
        auto seg_str = std::string_view(in.begin() + 1, in.end() - 1); // trim the quotes
        auto ps = PropertySegment(std::move(seg_str));
        data.emplace_back(ps);
    }
};

template <typename TContainer>
constexpr bool is_valid_container =
    std::is_same_v<decltype(std::declval<TContainer>().emplace_back(std::declval<const PropertySegment&>())), void>;

template <typename TContainer = PropertySegments> const TContainer parse(const std::string_view input) {
    TContainer result;
    pegtl::memory_input mi(input, "");
    pegtl::parse<pe::PropertyExpr, pe::Action>(mi, result);
    return result;
}

}; // namespace pe

TEST_CASE("Test Property Expression") {

    SECTION("Can parse strings Property Expression") {
        std::string pe1 = "test1.hello.world['aaa'].name_of";
        auto result = pe::parse(pe1);
        REQUIRE(result.size() == 5);
        REQUIRE(result == pe::PropertySegments{"test1", "hello", "world", "aaa", "name_of"});
    }

    SECTION("Can parse hybird Property Expression") {
        std::string pe1 = "test1[100].hello['aaa'][42].world[99]['bbb'].name_of[100]";
        auto result = pe::parse<std::vector<pe::PropertySegment>>(pe1);

        REQUIRE(result.size() == 10);

        auto expected = pe::PropertySegments{
            pe::PropertySegment("test1"), pe::PropertySegment(100U),  pe::PropertySegment("hello"),
            pe::PropertySegment("aaa"),   pe::PropertySegment(42U),   pe::PropertySegment("world"),
            pe::PropertySegment(99U),     pe::PropertySegment("bbb"), pe::PropertySegment("name_of"),
            pe::PropertySegment(100U),
        };
        REQUIRE(result == expected);
    }
}

TEST_CASE("Test Msg class") {

    constexpr char MSG_JSON[] = R"(
        {
            "topic": "/test/test1",
            "payload": {
                "hostInfo": {
                    "cpuLoad": 0.9,
                    "memoryUsage": 300,
                    "memoryTotal": 2048
                },
                "version": "0.1.0"
            }
        }
    )";


    SECTION("Test adding edges and sorting") {
        auto MSG_JSON_VALUE = boost::json::parse(MSG_JSON).as_object();
        auto msg1 = Msg(MSG_JSON_VALUE);

        msg1.set_navigation_property_value("payload.hostInfo.memoryUsage", 100);
        REQUIRE(msg1.get_navigation_property_value("payload.hostInfo.memoryUsage") == 100);
    }
}
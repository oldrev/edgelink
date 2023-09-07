#pragma once

namespace duk {

template <> struct Type<boost::json::string> {
    static void push(Context& d, boost::json::string const& value) { duk_push_string(d, value.c_str()); }

    static void get(Context& d, boost::json::string& value, int index) {
        const char* cstr = duk_get_string(d, index);
        value = boost::json::string(cstr);
    }

    static constexpr bool isPrimitive() { return true; };
};

template <> struct Type<boost::json::value> {

    static void push(duk::Context& d, boost::json::value const& value) {

        switch (value.kind()) {

        case boost::json::kind::null: {
            duk_push_null(d);
        } break;

        case boost::json::kind::array: {
            duk_push_array(d);
            auto json_array = value.as_array();
            for (int i = 0; i < json_array.size(); ++i) {
                Type<boost::json::value>::push(d, json_array[i]);
                duk_put_prop_index(d, -2, i);
            }
        } break;

        case boost::json::kind::bool_: {
            Type<bool>::push(d, value.as_bool());
        } break;

        case boost::json::kind::int64: {
            Type<int>::push(d, value.to_number<int>());
        } break;

        case boost::json::kind::uint64: {
            Type<unsigned int>::push(d, value.to_number<int>());
        } break;

        case boost::json::kind::double_: {
            Type<double>::push(d, value.as_double());
        } break;

        case boost::json::kind::string: {
            Type<boost::json::string>::push(d, value.as_string());
        } break;

        default: {
            //
            throw std::runtime_error("Bad json or value???? TODO FIXME");
        }

        } // switch
    }

    static void get(Context& d, boost::json::value& value, int index) {

        switch (duk_get_type(d, index)) {

        case DUK_TYPE_NONE: {
            value = std::move(nullptr);
        } break;

        case DUK_TYPE_UNDEFINED: {
            value = std::move(nullptr);
        } break;

        case DUK_TYPE_NULL: {
            value = std::move(nullptr);
        } break;

        case DUK_TYPE_BOOLEAN: {
            bool boolean;
            Type<bool>::get(d, boolean, index);
            value = std::move(boolean);
        } break;

        case DUK_TYPE_NUMBER: {
            double number;
            Type<double>::get(d, number, index);
            value = std::move(number);
        } break;

        case DUK_TYPE_STRING: {
            boost::json::string str_value;
            Type<boost::json::string>::get(d, str_value, index);
            value = std::move(boost::json::string(str_value));
        } break;

        case DUK_TYPE_OBJECT: {
            throw ::edgelink::NotSupportedException("还不支持 Duk object 到 json");
        } break;

        case DUK_TYPE_BUFFER: {
            throw ::edgelink::NotSupportedException("还不支持 Duk object 到 json");
        } break;

        case DUK_TYPE_POINTER: {
            throw ::edgelink::NotSupportedException("还不支持 Duk object 到 json");
        } break;

        default: {
            throw ::edgelink::NotSupportedException("还不支持 Duk object 到 json");
            break;
        }

        } // switch
    }

    static constexpr bool isPrimitive() { return false; };
};

template <> struct Type<boost::json::array> {
    static void push(Context& d, boost::json::array const& value) {
        duk_push_array(d);
        for (int i = 0; i < value.size(); ++i) {
            Type<boost::json::value>::push(d, value[i]);
            duk_put_prop_index(d, -2, i);
        }
    }

    static void get(duk::Context& d, boost::json::array& value, int index) {
        duk_enum(d, index, DUK_ENUM_ARRAY_INDICES_ONLY);

        while (duk_next(d, -1, 1)) {
            boost::json::value val;
            Type<boost::json::value>::get(d, val, -1);
            value.push_back(val);
            duk_pop_2(d);
        }

        duk_pop(d);
    }

    static constexpr bool isPrimitive() { return true; };
};

}; // namespace duk

namespace edgelink::scripting {

/// @brief Duktape 执行保护器，通过 RAII 模式允许自动清除全局变量，这样就可以自动恢复 JS 解析器状态使用一个 JS
/// 解析器实例互不干扰地解析多段 JS 代码
class DuktapeStashingGuard {
  public:
    DuktapeStashingGuard(const DuktapeStashingGuard&&) = delete;
    DuktapeStashingGuard(const DuktapeStashingGuard&) = delete;

    DuktapeStashingGuard(duk_context* ctx) : _ctx(ctx) {
        // 保存 Duktape 状态到全局 stash
        duk_push_global_stash(_ctx);
        duk_dup(_ctx, 0); // 复制当前的 Duktape 状态到 stash
        duk_put_prop_string(_ctx, -2, "__savedContext");
        duk_pop(_ctx); // 弹出全局 stash
    }

    ~DuktapeStashingGuard() {
        // 恢复之前保存的 Duktape 状态
        duk_push_global_stash(_ctx);
        duk_get_prop_string(_ctx, -1, "__savedContext"); // 获取保存的状态
        duk_dup(_ctx, -1);                               // 复制 stash 中的状态到当前堆栈
        duk_pop_2(_ctx);                                 // 弹出全局 stash 和恢复的状态
    }

  private:
    duk_context* _ctx;
};

}; // namespace edgelink::scripting
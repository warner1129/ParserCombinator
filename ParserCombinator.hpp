#include <any>
#include <map>
#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace details {
    template<class... Ts>
    struct ProductTypes {
        using type = std::tuple<Ts...>;
        type data;
        ProductTypes() {}
        ProductTypes(type &&data_) : data(data_) {}
    };

    template<class>
    struct isProductTypes : std::false_type {};
    template<class... T>
    struct isProductTypes<ProductTypes<T...>> : std::true_type {};
    template<class T>
    inline constexpr bool isProductTypes_v = isProductTypes<T>::value;

    template<class... Ts>
    struct PushProductTypes : std::type_identity<ProductTypes<Ts...>> {};
    template<>
    struct PushProductTypes<std::monostate, std::monostate> : std::type_identity<std::monostate> {};
    template<class... T, class U>
    struct PushProductTypes<ProductTypes<T...>, U> : std::type_identity<ProductTypes<T..., U>> {};
    template<class T, class... U>
    struct PushProductTypes<T, ProductTypes<U...>> : std::type_identity<ProductTypes<T, U...>> {};
    template<class... T, class... U>
    struct PushProductTypes<ProductTypes<T...>, ProductTypes<U...>> : std::type_identity<ProductTypes<T..., U...>> {};
    template<class T, class U>
    using PushProductTypes_t = typename PushProductTypes<T, U>::type;

    std::monostate ResultCat(std::monostate, std::monostate) { return {}; };
    template<class T, class U>
    ProductTypes<T, U> ResultCat(T a, U b) { return std::make_tuple(a, b); };
    template<class T, class... U>
    ProductTypes<T, U...> ResultCat(T a, ProductTypes<U...> b) { return std::tuple_cat(std::make_tuple(a), b.data); };
    template<class... T, class U>
    ProductTypes<T..., U> ResultCat(ProductTypes<T...> a, U b) { return std::tuple_cat(a.data, std::make_tuple(b)); };
    template<class... T, class... U>
    ProductTypes<T..., U...> ResultCat(ProductTypes<T...> a, ProductTypes<U...> b) { return std::tuple_cat(a.data, b.data); };

    template <typename T, typename U, typename... Ts>
    struct is_one_of : std::conditional_t<std::is_same_v<T, U>, std::true_type, is_one_of<T, Ts...>> {};
    template <typename T, typename U>
    struct is_one_of<T, U> : std::is_same<T, U> {};

    template<class... T>
    struct AdditionTypes {
        using type = std::any;
        type data;
    };

    template<class T, class U>
    struct PushAdditionTypes :
        std::type_identity<typename std::conditional<std::is_same_v<T, U>, T, AdditionTypes<T, U>>::type> {};

    template<class... Ts, class U>
    struct PushAdditionTypes<AdditionTypes<Ts...>, U> : std::conditional<
        is_one_of<U, Ts...>::value, AdditionTypes<Ts...>, AdditionTypes<Ts..., U>> {};
    template<class T, class U>
    using PushAdditionTypes_t = typename PushAdditionTypes<T, U>::type;

    template<typename T> struct LambdaTraits;
    template<typename T, typename R, typename ...Args>
    struct LambdaTraits<R(T::*)(Args...) const> : std::type_identity<R> {};

    template<class T, class = std::void_t<>>
    struct callWhich : std::type_identity<decltype(&T::operator())> {};
    template<class T>
    struct callWhich<T, decltype(&T::template operator())> : std::type_identity<decltype(&T::template operator())> {};

    template<class T, class ...Args>
    struct ReturnType : LambdaTraits<decltype(&T::template operator()<Args...>)> {};
    template<class Ret, class... Args>
    struct ReturnType<Ret(Args...)> : std::type_identity<Ret> {};
    template<class Ret, class... Args>
    struct ReturnType<Ret(&)(Args...)> : std::type_identity<Ret> {};
    template<class T>
    struct ReturnType<T> : LambdaTraits<typename callWhich<T>::type> {};
    template<class T>
    using ReturnType_t = typename ReturnType<T>::type;

}

using ParserInput = std::string_view;

template<class T>
using ParserResult = std::optional<std::pair<T, ParserInput>>;

template<class T = std::monostate>
struct ParserCombinator {
    using type = T;
    using Parser = std::function<ParserResult<type>(ParserInput)>;
    std::map<ParserInput, size_t> counter;
    std::map<ParserInput, ParserResult<T>> result;
    Parser parser{};
    ParserCombinator() {}
    ParserCombinator(Parser &&p) : parser{p} {}
    void ResetMemory() {
        counter.clear();
        result.clear();
    }
    ParserResult<T> operator()(ParserInput in) {
        if (result.count(in)) {
            return result[in];
        }
        if (counter[in] > in.size()) {
            return std::nullopt;
        }
        ++counter[in];
        auto r = parser(in);
        if (!result.count(in) or (r and result[in]->second.size() > r->second.size())) {
            result[in] = r;
        } else {
            r = result[in];
        }
        if (--counter[in] == 0) {
            ResetMemory();
        }
        return r;
    }
    Parser lazy() {
        return [f=this](ParserInput in) {
            return (*f)(in);
        };
    };
};

template<class>
struct isParserCombinator : std::false_type {};
template<class T>
struct isParserCombinator<ParserCombinator<T>> : std::true_type {};
template<class T>
inline constexpr bool isParserCombinator_v = isParserCombinator<T>::value;

template<class P1, class P2,
    class = std::enable_if_t<isParserCombinator_v<std::decay_t<P1>> and isParserCombinator_v<std::decay_t<P2>>>,
    class R = details::PushProductTypes_t<typename std::decay_t<P1>::type, typename std::decay_t<P2>::type>>
ParserCombinator<R> operator+(P1 &&p1, P2 &&p2) {
    return {[
        t1=std::is_lvalue_reference_v<P1> ? p1.lazy() : p1.parser,
        t2=std::is_lvalue_reference_v<P2> ? p2.lazy() : p2.parser
    ](ParserInput in) -> ParserResult<R> {
        auto r1 = t1(in);
        if (!r1) return std::nullopt;
        auto r2 = t2(r1->second);
        if (!r2) return std::nullopt;
        return std::pair(details::ResultCat(r1->first, r2->first), r2->second);
    }};
}

template<class P1, class P2,
    class = std::enable_if_t<isParserCombinator_v<std::decay_t<P1>> and isParserCombinator_v<std::decay_t<P2>>>,
    class R = details::PushAdditionTypes_t<typename std::decay_t<P1>::type, typename std::decay_t<P1>::type>>
ParserCombinator<R> operator|(P1 &&p1, P2 &&p2) {
    return {[
        t1=std::is_lvalue_reference_v<P1> ? p1.lazy() : p1.parser,
        t2=std::is_lvalue_reference_v<P2> ? p2.lazy() : p2.parser
    ](ParserInput in) -> ParserResult<R> {
        auto r1 = t1(in);
        if (r1) return r1;
        auto r2 = t2(in);
        if (r2) return r2;
        return std::nullopt;
    }};
}

template<class T, class Func, class R = details::ReturnType_t<Func>>
ParserCombinator<R> operator>>(ParserCombinator<T> p, Func f) {
    return {[=](ParserInput in) -> ParserResult<R> {
        auto r = p.parser(in);
        if (!r) return std::nullopt;
        if constexpr (details::isProductTypes_v<T>) {
            return std::pair(apply(f, r->first.data), r->second);
        } else {
            return std::pair(f(r->first), r->second);
        }
    }};
};

template<class R = std::monostate>
ParserCombinator<R> Token(std::function<bool(char)> &&f) {
    return {[=](ParserInput in) -> ParserResult<R> {
        if (in.empty() or !f(in[0])) return std::nullopt;
        if constexpr (std::is_same_v<R, std::monostate>) {
            return std::pair(std::monostate{}, ParserInput(in.begin() + 1, in.end()));
        } else {
            return std::pair(in[0], ParserInput(in.begin() + 1, in.end()));
        }
    }};
}

template<class R = std::monostate>
ParserCombinator<R> Token(std::string S) {
    return {[=](ParserInput in) -> ParserResult<std::string> {
        if (in.starts_with(S)) {
            in.remove_prefix(S.length());
            if constexpr (std::is_same_v<R, std::monostate>) {
                return std::pair({}, in);
            } else {
                return std::pair(std::string(S), in);
            }
        }
        return std::nullopt;
    }};
}

ParserCombinator<std::monostate> Epsilon() {
    return {[](ParserInput in) -> ParserResult<std::monostate> {
        if (in.empty()) return std::pair(std::monostate{}, in);
        return std::nullopt;
    }};
}

ParserCombinator<std::monostate> Lambda() {
    return {[](ParserInput in) -> ParserResult<std::monostate> {
        return std::pair(std::monostate{}, in);
    }};
}

auto operator""_T(const char c) {
    return Token<std::monostate>([=](char in) { return in == c; });
}

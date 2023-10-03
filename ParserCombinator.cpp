#include <bits/stdc++.h>

using namespace std;

namespace details {
    template<class... Ts>
    struct ProductTypes {
        using type = tuple<Ts...>;
        type data;
        ProductTypes() {}
        ProductTypes(type &&data_) : data(data_) {}
    };

    template<class>
    struct isProductTypes : false_type {};
    template<class... T>
    struct isProductTypes<ProductTypes<T...>> : true_type {};
    template<class T>
    inline constexpr bool isProductTypes_v = isProductTypes<T>::value;

    template<class... Ts>
    struct PushProductTypes : type_identity<ProductTypes<Ts...>> {};
    template<>
    struct PushProductTypes<monostate, monostate> : type_identity<monostate> {};
    template<class... T, class U>
    struct PushProductTypes<ProductTypes<T...>, U> : type_identity<ProductTypes<T..., U>> {};
    template<class T, class... U>
    struct PushProductTypes<T, ProductTypes<U...>> : type_identity<ProductTypes<T, U...>> {};
    template<class... T, class... U>
    struct PushProductTypes<ProductTypes<T...>, ProductTypes<U...>> : type_identity<ProductTypes<T..., U...>> {};
    template<class T, class U>
    using PushProductTypes_t = typename PushProductTypes<T, U>::type;

    monostate ResultCat(monostate, monostate) { return {}; };
    template<class T, class U>
    ProductTypes<T, U> ResultCat(T a, U b) { return make_tuple(a, b); };
    template<class T, class... U>
    ProductTypes<T, U...> ResultCat(T a, ProductTypes<U...> b) { return tuple_cat(make_tuple(a), b.data); };
    template<class... T, class U>
    ProductTypes<T..., U> ResultCat(ProductTypes<T...> a, U b) { return tuple_cat(a.data, make_tuple(b)); };
    template<class... T, class... U>
    ProductTypes<T..., U...> ResultCat(ProductTypes<T...> a, ProductTypes<U...> b) { return tuple_cat(a.data, b.data); };

    template <typename T, typename U, typename... Ts>
    struct is_one_of : conditional_t<is_same_v<T, U>, true_type, is_one_of<T, Ts...>> {};
    template <typename T, typename U>
    struct is_one_of<T, U> : is_same<T, U> {};

    template<class... T>
    struct AdditionTypes {
        using type = std::any;
        type data;
    };

    template<class T, class U>
    struct PushAdditionTypes :
        type_identity<typename conditional<is_same_v<T, U>, T, AdditionTypes<T, U>>::type> {};

    template<class... Ts, class U>
    struct PushAdditionTypes<AdditionTypes<Ts...>, U> : conditional<
        is_one_of<U, Ts...>::value, AdditionTypes<Ts...>, AdditionTypes<Ts..., U>> {};
    template<class T, class U>
    using PushAdditionTypes_t = typename PushAdditionTypes<T, U>::type;

    template<typename T> struct LambdaTraits;
    template<typename T, typename R, typename ...Args>
    struct LambdaTraits<R(T::*)(Args...) const> : type_identity<R> {};

    template<class T, class = std::void_t<>>
    struct callWhich : type_identity<decltype(&T::operator())> {};
    template<class T>
    struct callWhich<T, decltype(&T::template operator())> : type_identity<decltype(&T::template operator())> {};

    template<class T, class ...Args>
    struct ReturnType : LambdaTraits<decltype(&T::template operator()<Args...>)> {};
    template<class Ret, class... Args>
    struct ReturnType<Ret(Args...)> : type_identity<Ret> {};
    template<class Ret, class... Args>
    struct ReturnType<Ret(&)(Args...)> : type_identity<Ret> {};
    template<class T>
    struct ReturnType<T> : LambdaTraits<typename callWhich<T>::type> {};
    template<class T>
    using ReturnType_t = typename ReturnType<T>::type;

}

using ParserInput = string_view;

template<class T>
using ParserResult = optional<pair<T, ParserInput>>;

template<class T = monostate>
struct ParserCombinator {
    using type = T;
    using Parser = function<ParserResult<type>(ParserInput)>;
    map<ParserInput, size_t> counter;
    map<ParserInput, ParserResult<T>> result;
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
            return nullopt;
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
struct isParserCombinator : false_type {};
template<class T>
struct isParserCombinator<ParserCombinator<T>> : true_type {};
template<class T>
inline constexpr bool isParserCombinator_v = isParserCombinator<T>::value;

template<class P1, class P2,
    class = enable_if_t<isParserCombinator_v<decay_t<P1>> and isParserCombinator_v<decay_t<P2>>>,
    class R = details::PushProductTypes_t<typename decay_t<P1>::type, typename decay_t<P2>::type>>
ParserCombinator<R> operator+(P1 &&p1, P2 &&p2) {
    return {[
        t1=is_lvalue_reference_v<P1> ? p1.lazy() : p1.parser,
        t2=is_lvalue_reference_v<P2> ? p2.lazy() : p2.parser
    ](ParserInput in) -> ParserResult<R> {
        auto r1 = t1(in);
        if (!r1) return nullopt;
        auto r2 = t2(r1->second);
        if (!r2) return nullopt;
        return pair(details::ResultCat(r1->first, r2->first), r2->second);
    }};
}

template<class P1, class P2,
    class = enable_if_t<isParserCombinator_v<decay_t<P1>> and isParserCombinator_v<decay_t<P2>>>,
    class R = details::PushAdditionTypes_t<typename decay_t<P1>::type, typename decay_t<P1>::type>>
ParserCombinator<R> operator|(P1 &&p1, P2 &&p2) {
    return {[
        t1=is_lvalue_reference_v<P1> ? p1.lazy() : p1.parser,
        t2=is_lvalue_reference_v<P2> ? p2.lazy() : p2.parser
    ](ParserInput in) -> ParserResult<R> {
        auto r1 = t1(in);
        if (r1) return r1;
        auto r2 = t2(in);
        if (r2) return r2;
        return nullopt;
    }};
}

template<class T, class Func, class R = details::ReturnType_t<Func>>
ParserCombinator<R> operator>>(ParserCombinator<T> p, Func f) {
    return {[=](ParserInput in) -> ParserResult<R> {
        auto r = p.parser(in);
        if (!r) return nullopt;
        if constexpr (details::isProductTypes_v<T>) {
            return pair(apply(f, r->first.data), r->second);
        } else {
            return pair(f(r->first), r->second);
        }
    }};
};

template<class R = char>
ParserCombinator<R> Token(function<bool(char)> &&f) {
    return {[=](ParserInput in) -> ParserResult<R> {
        if (in.empty() or !f(in[0])) return nullopt;
        if constexpr (is_same_v<R, monostate>) {
            return pair(monostate{}, ParserInput(in.begin() + 1, in.end()));
        } else {
            return pair(in[0], ParserInput(in.begin() + 1, in.end()));
        }
    }};
}

ParserCombinator<monostate> Epsilon() {
    return {[](ParserInput in) -> ParserResult<monostate> {
        if (in.empty()) return pair(monostate{}, in);
        return nullopt;
    }};
}

ParserCombinator<monostate> Lambda() {
    return {[](ParserInput in) -> ParserResult<monostate> {
        return pair(monostate{}, in);
    }};
}

ParserCombinator<string> Token(string_view S) {
    return {[=](ParserInput in) -> ParserResult<string> {
        if (in.starts_with(S)) {
            in.remove_prefix(S.length());
            return pair(string(S), in);
        }
        return nullopt;
    }};
}

auto operator""_T(const char c) {
    return Token<monostate>([=](char in) { return in == c; });
}

template<class T = monostate>
using Parser = ParserCombinator<T>;

void solve() {

    Parser<int> Decimal = Token(::isdigit) >> [](char c) { return c - '0'; };
    Parser<int> Number;
    Number = Number + Decimal >> [](int a, int b) { return a * 10 + b; } | Decimal;

    Parser<int> expr;
    expr = '('_T + expr + ','_T + expr + ','_T + expr + ')'_T >>
        [](monostate, int a, monostate, int b, monostate, int c, monostate) {
            return a * b % c;
    } | Number;

    string s;
    while (cin >> s) {
        auto r = expr(s);
        if (r) {
            cout << r->first << '\n';
        } else {
            cout << "fail\n";
        }
    }

}

signed main() {
    int T = 1;
    // cin >> T;
    while (T--) solve();
    return 0;
}



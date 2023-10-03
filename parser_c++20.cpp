#include <bits/stdc++.h>

using namespace std;

using ParserInput = string_view;
using ParserResult = optional<string_view>;
using Parser = function<ParserResult(ParserInput)>;

Parser lazy(const Parser &F) {
    return [f=&F](ParserInput s) {
        return (*f)(s);
    };
}

template<class P1, class P2>
Parser operator+(P1 &&p1, P2 &&p2) {
    return [
        t1=is_lvalue_reference<P1>::value ? lazy(p1) : p1,
        t2=is_lvalue_reference<P2>::value ? lazy(p2) : p2
    ](ParserInput s) -> ParserResult {
        auto r = t1(s);
        if (!r) return nullopt;
        return t2(*r);
    };
}

template<class P1, class P2>
Parser operator|(P1 &&p1, P2 &&p2) {
    return [
        t1=is_lvalue_reference<P1>::value ? lazy(p1) : p1,
        t2=is_lvalue_reference<P2>::value ? lazy(p2) : p2
    ](ParserInput s) -> ParserResult {
        auto r = t1(s);
        if (r) return r;
        return t2(s);
    };
}

Parser oneOf(string_view S) {
    return [=](ParserInput c) -> ParserResult {
        if (c.empty() or S.find(c[0]) == string::npos) return nullopt;
        return ParserInput(c.begin() + 1, c.end());
    };
}

Parser Token(string_view S) {
    return [=](ParserInput c) -> ParserResult {
        if (c.starts_with(S)) {
            c.remove_prefix(S.length());
            return c;
        }
        return {};
    };
}


void solve() {
    // productions
    Parser Decimal = oneOf("0123456789");
    Parser Alpha = oneOf("aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ");
    Parser Opts = oneOf("+-*/");
    Parser LBR = Token("(");
    Parser RBR = Token(")");
    Parser DOT = Token(".");
    Parser QUO = Token("\"");
    Parser e = Token("");
    Parser end = [&](ParserInput _) -> ParserResult {
        if (_.empty()) return _;
        return {};
    };

    Parser Program, stmts, stmt, primary, primary_tail, Number;
    Parser exprs, expr, exprT, ID, STRLIT;

    Number = Decimal + Number | Decimal;
    exprT = Number | LBR + expr + RBR;
    expr = exprT + Opts + expr | exprT;

    ID = Alpha + ID | Alpha;
    STRLIT = QUO + ID + QUO;
    Program = stmts + end;
    stmts = stmt + stmts | e;
    stmt = primary | STRLIT;
    Parser stmtT = stmt | e;
    primary = ID + primary_tail;
    primary_tail = DOT + ID + primary_tail | LBR + stmtT + RBR + primary_tail | e;

    string S;
    while (cin >> S) {
        auto a = Program(S);
        if (!a) {
            cout << "fail\n";
        } else {
            cout << "... " << *a << '\n';
        }
    }
}

signed main() {
    solve();
    return 0;
}

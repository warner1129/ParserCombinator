#include <bits/stdc++.h>

using namespace std;

using ParserInput = string;
using ParserResult = pair<bool, ParserInput>;
using Parser = function<ParserResult(ParserInput)>;

Parser lazy(const Parser &F) {
    auto f = &F;
    return [=](ParserInput s) {
        return (*f)(s);
    };
}

template<class P1, class P2>
Parser operator+(P1 &&p1, P2 &&p2) {
    auto t1 = is_lvalue_reference<P1>::value ? lazy(p1) : p1;
    auto t2 = is_lvalue_reference<P2>::value ? lazy(p2) : p2;
    return [=](ParserInput s) -> ParserResult {
        auto r = t1(s);
        if (!r.first) return ParserResult(false, "");
        return t2(r.second);
    };
}

template<class P1, class P2>
Parser operator|(P1 &&p1, P2 &&p2) {
    auto t1 = is_lvalue_reference<P1>::value ? lazy(p1) : p1;
    auto t2 = is_lvalue_reference<P2>::value ? lazy(p2) : p2;
    return [=](ParserInput s) -> ParserResult {
        auto r = t1(s);
        if (r.first) return r;
        return t2(s);
    };
}

Parser oneOf(string S) {
    return [=](ParserInput c) -> ParserResult {
        if (c.empty() or S.find(c[0]) == string::npos)
            return ParserResult(false, "");
        return ParserResult(true, c.substr(1));
    };
}

Parser Token(string S) {
    return [=](ParserInput c) -> ParserResult {
        for (int i = 0; i < S.size(); i++)
            if (i >= c.size() or c[i] != S[i])
                return ParserResult(false, "");
        return ParserResult(true, c.substr(S.size()));
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
        if (_.empty()) return ParserResult(true, "");
        return ParserResult(false, "");
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

    Parser Float = Number + DOT + Number | Number + DOT;

    Parser items, obj, item;
    obj = Number | ID;
    item = LBR + obj + DOT + obj + DOT + obj + RBR;
    items = item + items | item;

    string S;
    while (cin >> S) {
        auto a = (expr + end)(S);
        if (!a.first) {
            cout << "fail\n";
        } else {
            cout << "ok\n";
        }
    }
}

signed main() {
    solve();
    return 0;
}

#include "ParserCombinator.hpp"
#include <iostream>

template<class T = std::monostate>
using Parser = ParserCombinator<T>;

void test() {
    Parser Decimal = Token(isdigit);
    Parser Number;
    Number = Number + Decimal | Decimal;

    Parser Alpha = Token(isalpha);
    Parser ID;
    ID = ID + Alpha | Alpha;

    Parser item, obj, expr;
    item = Number | ID;
    obj = '('_T + item + ','_T + item + ')'_T;
    expr = expr + obj | obj;
    // (aabb,2233)(abg,433)(23,2231)....

    Parser expr_end = expr + Epsilon(); 

    std::string s;
    while (std::cin >> s) {
        auto r = expr_end(s);
        if (r) {
            std::cout << "match" << '\n';
        } else {
            std::cout << "fail\n";
        }
    }
    
}

void test2() {
    Parser<int> Add, Mul, Pri, Num, Dec;

    Dec = Token<char>(isdigit) >> [](char c) { return c - '0'; };
    Num = Num + Dec >> [](int a, int b) { return a * 10 + b; } | Dec;

    Add = Add + '+'_T + Mul >> [](int a, std::monostate, int b) { return a + b; } | 
          Add + '-'_T + Mul >> [](int a, std::monostate, int b) { return a - b; } | 
          Mul;

    Mul = Mul + '*'_T + Pri >> [](int a, std::monostate, int b) { return a * b; } |
          Mul + '/'_T + Pri >> [](int a, std::monostate, int b) { return a / b; } |
          Pri;

    Pri = '('_T + Add + ')'_T >> [](std::monostate, int a, std::monostate) { return a; } | Num;

    std::cout << Add("2+3*(2-3-(1+(2-3)-5)*3)/2")->first << '\n'; // =23

}

int main() {
    test2();
    return 0;
}
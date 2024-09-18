## Features

- **ParserCombinator 類**：
  - 能夠處理解析過程中的中間結果並記憶化。
  - 支持多種組合子運算，包括 `+`（串接）、`|`（選擇）、`>>`（映射），以及 `Token` 和 `Epsilon` 函數來定義基本的解析單位。

- **Parser Combinator 運算符**：
  - **`+`**：串接運算，將兩個解析器組合成一個解析器，依次處理兩個輸入部分。
  - **`|`**：選擇運算，在兩個解析器之間進行選擇，嘗試第一個解析器，若失敗則嘗試第二個解析器。
  - **`>>`**：映射運算，將解析結果映射到其他類型或進行進一步處理。

- **基本解析單位**：
  - **Token**：用於匹配單個字符或字符串，支持自定義匹配函數。
  - **Epsilon**：表示空解析，當輸入為空時返回成功。
  - **Lambda**：表示空解析，不消耗任何輸入。

## Sample

### Usage

```cpp
Parser Decimal = Token(isdigit);
Parser Number = Number + Decimal | Decimal;

Parser Alpha = Token(isalpha);
Parser ID = ID + Alpha | Alpha;

Parser item = Number | ID;
Parser obj = '('_T + item + ','_T + item + ')'_T;
Parser expr = expr + obj | obj;

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
```

### 表達式解析
```cpp
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
```
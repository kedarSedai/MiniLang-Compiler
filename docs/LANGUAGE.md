# MiniLang (draft)

A minimal imperative language for compiler + optimization research.

## Lexical grammar (informal)

- **Whitespace:** space, tab, newline (ignored except for line tracking)
- **Comments:** `//` to end of line; `/* ... */` nested not required
- **Identifiers:** `[A-Za-z_][A-Za-z0-9_]*`
- **Integers:** decimal `[0-9]+`
- **Keywords:** `int` `bool` `if` `else` `while` `return` `true` `false` `void`
- **Operators:** `+` `-` `*` `/` `%` `=` `==` `!=` `<` `<=` `>` `>=` `&&` `||` `!`
- **Delimiters:** `(` `)` `{` `}` `[` `]` `;` `,`

## Example

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

## Syntax (parser)

```
program     ::= function*
function    ::= type IDENT "(" [ parameters ] ")" block
parameters  ::= type IDENT ( "," type IDENT )*
block       ::= "{" statement* "}"
statement   ::= block
              | type IDENT "=" expression ";"
              | "if" "(" expression ")" statement [ "else" statement ]
              | "while" "(" expression ")" statement
              | "return" [ expression ] ";"
              | expression ";"
expression  ::= logical_or
logical_or  ::= logical_and ( "||" logical_and )*
logical_and ::= equality ( "&&" equality )*
equality    ::= comparison ( ( "==" | "!=" ) comparison )*
comparison  ::= term ( ( "<" | "<=" | ">" | ">=" ) term )*
term        ::= factor ( ( "+" | "-" ) factor )*
factor      ::= unary ( ( "*" | "/" | "%" ) unary )*
unary       ::= ( "!" | "-" ) unary | call
call        ::= primary
primary     ::= INT | "true" | "false" | IDENT [ "(" [ arguments ] ")" ] | "(" expression ")"
arguments   ::= expression ( "," expression )*
type        ::= "int" | "bool" | "void"
```

## Semantic rules

The semantic analyzer (`minilang_semantic`) checks:

- **Functions:** no duplicate definitions; calls match arity and parameter types.
- **Variables:** declared before use; no duplicate declarations in the same block scope.
- **Scopes:** nested blocks; inner scopes may shadow outer names.
- **Types:**
  - `int` literals and arithmetic (`+ - * / %`) use `int`
  - `true` / `false` and `&&` / `||` / `!` use `bool`
  - comparisons (`< <= > >=`) require `int`, produce `bool`
  - `==` / `!=` require matching `int` or `bool` operands, produce `bool`
  - `if` / `while` conditions must be `bool`
  - variable initializers and `return` values must match the declared / function type
  - `void` functions may use `return;` but not `return expr;`

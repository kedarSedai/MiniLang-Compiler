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

- Functions: no duplicates; calls match arity and parameter types.
- Variables: declare before use; block scopes with optional shadowing.
- Types: `int`/`bool` enforced on operators, conditions, returns, and initializers.

## HIR (high-level IR)

After semantic analysis, the AST is lowered to flat HIR per function:

- **Temporaries** (`%t0`, `%t1`, …) for expression results
- **Locals** via `load_local` / `store_local`
- **Control flow** via `br_cond`, `jump`, and `label`
- **Calls** and `ret`

Rule-based optimizations on HIR:

1. **Constant folding** — e.g. `2 + 3` → `5`
2. **Algebraic simplification** — e.g. `x * 0` → `0`
3. **Dead temp removal** — drop unused temporaries

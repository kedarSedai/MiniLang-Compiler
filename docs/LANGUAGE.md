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

(Syntax and semantics will be refined when the parser lands.)

# Tricky C/C++ constructs

- [Arbitrary-sized Integers in C23](https://blog.tal.bi/posts/c23-bitint/)
- [C23 template](https://www.reddit.com/r/C_Programming/comments/1cmqqgw/c23_makes_errors_awesome/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button)
- https://en.wikipedia.org/wiki/C23_(C_standard_revision)#Constants
    > Add `wb` and `uwb` integer literal suffixes for `_BitInt(N)` and `unsigned _BitInt(N)` types, such as `6uwb` yields an `unsigned _BitInt(3)`, and `-6wb` yields a signed `_BitInt(4)` which has three value bits and one sign bit.
- https://stackoverflow.com/a/71848927
    > As of c++20, you can now copy objects that have one or more const member objects by defining your own copy-assignment operator.
- [Growing Immutable Value](https://martin-moene.blogspot.com/2012/08/growing-immutable-value.html)


Hypothetically, can the lifetime of local variables in the body of a do-while loop be extended to the conditional expression?
```cpp
do {
    int x;
} while (x); // Looks good.

std::mutex m;
do {
    const std::lock_guard lock(m);
} while (true); // Looks fishy; should already be unlocked.
```

- highest bit set: for 32 bits, use [de Bruijn multiplication](https://stackoverflow.com/a/31718095).

- C++ string literal concatenate
    - https://stackoverflow.com/a/75866219
    - https://stackoverflow.com/a/72510788

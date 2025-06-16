int main() {
  struct tight {
    _BitInt(6) src_shift : 6;
    _BitInt(6) dst_shift : 6;
  };
  constexpr _BitInt(6) shift = 1;
  static_assert(_BitInt(6){-1} == int{-1});
  using Shift = unsigned _BitInt(6);
  static_assert(Shift{0} == Shift{0x3f});
  static_assert(Shift{0} - Shift{0x3f} == -63);
  static_assert(-1 / 8 == 0);
  static_assert(-2 / 8 == 0);
  static_assert(-8 / 8 == -1);
  static_assert(-1 % 8 == -1);
  static_assert(-2 % 8 == -2);
  static_assert(-8 % 8 == 0);
  Shift s = -1; // DANGER: should warn about implicit conversion. Is there a linter check for this?
  Shift s{-1};
}

// https://xn--rpa.cc/irl/term.html
// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// https://man.freebsd.org/cgi/man.cgi?query=screen&apropos=0&sektion=4&manpath=FreeBSD+5.4-RELEASE&format=html
module;
#include <charconv>
#include <string>
export module ansi;

// The escape character, '\033', followed by
// the control sequence introducer (CSI), '[', followed by
// parameters separated by ';', followed by
// a command character.
export namespace ansi {

// Rows and columns are 1-based.
constexpr std::string cursor_position(const uint8_t row, const uint8_t col) {
  char array[] = "\033[row;colH";
  auto first = std::begin(array) + 2;
  const auto last = std::end(array);
  first = std::to_chars(first, last, row).ptr;
  *first++ = ';';
  first = std::to_chars(first, last, col).ptr;
  *first++ = 'H';
  *first = '\0';
  return array;
}

namespace impl {

template <char command> constexpr std::string move_cursor(const uint8_t offset) {
  char array[] = "\033[nnnX";
  auto first = std::begin(array) + 2;
  const auto last = std::end(array);
  first = std::to_chars(first, last, offset).ptr;
  *first++ = command;
  *first = '\0';
  return array;
}

} // namespace impl

constexpr auto cursor_up(const uint8_t offset) { return impl::move_cursor<'A'>(offset); }
constexpr auto cursor_down(const uint8_t offset) { return impl::move_cursor<'B'>(offset); }
constexpr auto cursor_forward(const uint8_t offset) { return impl::move_cursor<'C'>(offset); }
constexpr auto cursor_back(const uint8_t offset) { return impl::move_cursor<'D'>(offset); }
constexpr auto cursor_column(const uint8_t offset) { return impl::move_cursor<'G'>(offset); }
constexpr auto cursor_hide{"\033[?25l"};
constexpr auto cursor_show{"\033[?25h"};
constexpr auto cursor_steady_block{"\033[0 q"};
constexpr auto cursor_blinking_block{"\033[1 q"};
constexpr auto cursor_reset{"\033[H"};

constexpr auto clear_screen{"\033[2J"};
constexpr auto hard_clear_screen{"\033[3J\033c"};
constexpr auto clear_line{"\033[2K"};

constexpr auto reset{"\033[m"};
// The control sequence `CSI n m`, named Select Graphic Rendition (SGR), sets display attributes.
#define SGR(n) ("\033[" n "m")
constexpr auto bold = SGR("1");
constexpr auto increased_intensity = bold;
constexpr auto faint = SGR("2");
constexpr auto decreased_intensity = faint;
constexpr auto italic = SGR("3");
constexpr auto underline = SGR("4");
constexpr auto slow_blink = SGR("5");
constexpr auto blink = slow_blink;
constexpr auto rapid_blink = SGR("6");
constexpr auto invert = SGR("7");
constexpr auto conceal = SGR("8");
constexpr auto crossed_out = SGR("9");
constexpr auto strike = crossed_out;
constexpr auto primary_font = SGR("10");
constexpr auto fraktur = SGR("20");
constexpr auto gothic = fraktur;
constexpr auto doubly_underlined = SGR("21");
constexpr auto normal_intensity = SGR("22");
constexpr auto not_italic_nor_blackletter = SGR("23");
// Neither singly nor doubly underlined
constexpr auto no_underlined = SGR("24");
constexpr auto not_blinking = SGR("25");
constexpr auto proportional_spacing = SGR("26");
constexpr auto not_reversed = SGR("27");
constexpr auto reveal = SGR("28");
constexpr auto not_concealed = reveal;
constexpr auto not_crossed_out = SGR("29");

#undef SGR

namespace impl {

// FG   BG  Name
// 30   40  Black  rgb(0, 0, 0)
// 31   41  Red    rgb(170, 0, 0)
// 32   42  Green  rgb(0, 170, 0)
// 33   43  Yellow rgb(170, 85, 0)
// 34   44  Blue   rgb(0, 0, 170)
// 35   45  Magenta  rgb(170, 0, 170)
// 36   46  Cyan   rgb(0, 170, 170)
// 37   47  White  rgb(170, 170, 170)
// 90  100  Bright Black (Gray) rgb(85, 85, 85)
// 91  101  Bright Red
// 92  102  Bright Green	rgb(85, 255, 85)
// 93  103  Bright Yellow  rgb(255, 255, 85)
// 94  104  Bright Blue  rgb(85, 85, 255)
// 95  105  Bright Magenta rgb(255, 85, 255)
// 96  106  Bright Cyan    rgb(85, 255, 255)
// 97  107  Bright White  rgb(255, 255, 255)
constexpr std::string color(const uint8_t c) {
  char array[] = "\033[NNNm";
  auto first = std::begin(array) + 2;
  const auto last = std::end(array);
  first = std::to_chars(first, last, c).ptr;
  *first++ = 'm';
  *first = '\0';
  return array;
}

} // namespace impl

enum class color { black, red, green, yellow, blue, magenta, cyan, white };

constexpr auto foreground(const color c, const bool bright) {
  return impl::color(std::to_underlying(c) + (bright ? 90 : 30));
}
constexpr auto foreground_dark(const color c) { return foreground(c, false); }
constexpr auto foreground_bright(const color c) { return foreground(c, true); }

constexpr auto background(const color c, const bool bright) {
  return impl::color(std::to_underlying(c) + (bright ? 40 : 100));
}
constexpr auto background_dark(const color c) { return background(c, false); }
constexpr auto background_bright(const color c) { return background(c, true); }

} // namespace ansi

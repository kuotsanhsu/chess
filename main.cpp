#include "ansi_escape_code.hpp"
#include "chess.hpp"
#include <csignal>
#include <iostream>
#include <termios.h>
#include <unistd.h>

struct colored_piece {
  piece piece;
  bool is_white;
};

// https://stackoverflow.com/a/8327034
std::ostream &operator<<(std::ostream &os, const colored_piece &cp) {
  constexpr std::array fgcolors{
      ansi::foreground_bright(ansi::color::green),
      ansi::foreground_bright(ansi::color::white),
  };
  const auto fgcolor = fgcolors[cp.is_white];
  switch (cp.piece) {
  case piece::empty:
    return os << fgcolor << "　";
  case piece::pawn:
    return os << fgcolor << (cp.is_white ? "Ｐ" : "ｐ");
  case piece::rook:
    return os << fgcolor << (cp.is_white ? "Ｒ" : "ｒ");
  case piece::knight:
    return os << fgcolor << (cp.is_white ? "Ｎ" : "ｎ");
  case piece::bishop:
    return os << fgcolor << (cp.is_white ? "Ｂ" : "ｂ");
  case piece::queen:
    return os << fgcolor << (cp.is_white ? "Ｑ" : "ｑ");
  case piece::king:
    return os << fgcolor << (cp.is_white ? "Ｋ" : "ｋ");
  }
}

// https://askubuntu.com/a/558422
std::ostream &operator<<(std::ostream &os, const configuration &config) {
  std::array<piece, 64> board;
  std::ranges::fill(board, piece::empty);
  for (const auto [piece, shift] : config.get_white()) {
    board[63 ^ shift] = piece;
  }
  for (const auto [piece, shift] : config.get_black()) {
    board[63 ^ shift] = piece;
  }
  constexpr auto file_hint{"　ａｂｃｄｅｆｇｈ　"};
  constexpr auto bgcolors = std::views::repeat(std::array{
                                ansi::background_bright(ansi::color::blue),
                                ansi::background_dark(ansi::color::blue),
                            }) |
                            std::views::join;
  auto bgcolor = std::ranges::begin(bgcolors);
  constexpr auto hint_color{"\033[0;49;90m"};
  os << hint_color << file_hint << '\n';
  auto square = board.cbegin();
  auto pos = uint64_t{1} << 63;
  for (const auto rank : {"８", "７", "６", "５", "４", "３", "２", "１"}) {
    os << rank;
    for (const auto _ : std::views::iota(0, 8)) {
      os << *bgcolor++ << colored_piece(*square++, pos & config.get_white().get_occupancy());
      pos >>= 1;
    }
    os << hint_color << rank << '\n';
    ++bgcolor;
  }
  return os << file_hint << ansi::reset;
}

// Note that tcsetattr() returns success if any of the requested changes could be successfully
// carried out. Therefore, when making multiple changes it may be necessary to follow this call with
// a further call to tcgetattr() to check that all changes have been performed successfully.
void assert_tcsetattr(const int fd, const int optional_actions, const termios &expected) {
  assert(tcsetattr(fd, optional_actions, &expected) == 0);
  termios actual{};
  assert(tcgetattr(fd, &actual) == 0);
  assert(actual.c_iflag == expected.c_iflag);
  assert(actual.c_oflag == expected.c_oflag);
  assert(actual.c_cflag == expected.c_cflag);
  assert(actual.c_lflag == expected.c_lflag);
}

void noecho() {
  termios t{};
  assert(tcgetattr(STDIN_FILENO, &t) == 0);
  static const auto initial_termios = t;
  std::atexit([] {
    std::cout << ansi::cursor_position(11, 1) << std::flush;
    assert_tcsetattr(STDIN_FILENO, TCSAFLUSH, initial_termios);
  });
  t.c_lflag &= ~(ECHO | ICANON);
  assert_tcsetattr(STDIN_FILENO, TCSAFLUSH, t);

  struct sigaction act{
      .sa_handler = [](int) { exit(1); },
      .sa_flags = 0,
  };
  assert(sigemptyset(&act.sa_mask) == 0);
  for (struct sigaction oldact{}; const int signum : {SIGINT, SIGHUP, SIGTERM}) {
    assert(sigaction(signum, nullptr, &oldact) == 0);
    if (oldact.sa_handler != SIG_IGN) {
      assert(sigaction(signum, &act, nullptr) == 0);
    }
  }
}

void loop() {
  int file = 0, rank = 0, col = 0;
  for (char ch; read(STDIN_FILENO, &ch, 1) == 1;) {
    if ('a' <= ch && ch <= 'h') {
      if (rank == 0) {
        file = ch - 'a' + 1;
        col = file * 2 + 1;
        std::cout << ansi::cursor_show << ansi::cursor_position(10, col) << std::flush;
      }
    } else if ('1' <= ch && ch <= '8') {
      if (file != 0) {
        rank = ch - '1' + 1;
        std::cout << ansi::cursor_position(10 - rank, col) << std::flush;
      }
    } else {
      switch (ch) {
      case '\n':
        break;
      case '\033':
        file = rank = 0;
        std::cout << ansi::cursor_hide << std::flush;
        break;
      }
    }
  }
}

int main() {
  std::cin.tie(nullptr)->sync_with_stdio(false);
  noecho();

  constexpr configuration config;
  std::cout << ansi::hard_clear_screen << ansi::cursor_hide << config << std::flush;
  loop();
}

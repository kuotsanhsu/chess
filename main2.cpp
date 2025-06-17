#include "ansi_escape_code.hpp"
#include <algorithm>
#include <cassert>
#include <csignal>
#include <iostream>
#include <ranges>
#include <termios.h>
#include <unistd.h>

using square = unsigned _BitInt(6);

enum class piece { empty, pawn, rook, knight, bishop, queen, king };

class side {
  static constexpr std::array initial_rank1{
      piece::rook, piece::knight, piece::bishop, piece::queen,
      piece::king, piece::bishop, piece::knight, piece::rook,
  };
  static constexpr std::ranges::repeat_view initial_rank2{piece::pawn, 8};
  static constexpr std::ranges::repeat_view initial_rank3{piece::empty, 48};

  static constexpr side initial(auto &&r1, auto &&r2, auto &&r3) noexcept {
    // assert(std::ranges::size(r1) + std::ranges::size(r2) + std::ranges::size(r3) == 64);
    constexpr auto populate = [](side side, auto &&pieces) -> class side {
      for (const piece piece : pieces) {
        side.occupancy <<= 1;
        if (piece != piece::empty) {
          side.occupancy ^= 1;
          side.pieces <<= 4;
          side.pieces ^= std::to_underlying(piece);
        }
      }
      return side;
    };
    side side;
    side = populate(side, r1);
    side = populate(side, r2);
    side = populate(side, r3);
    return side;
  }

public:
  static constexpr side initial_black() noexcept {
    return initial(side::initial_rank1, side::initial_rank2, side::initial_rank3);
  }
  static constexpr side initial_white() noexcept {
    return initial(side::initial_rank3, side::initial_rank2, side::initial_rank1);
  }

private:
  uint64_t occupancy{}, pieces{};
  constexpr side() noexcept = default;

public:
  [[nodiscard]] constexpr auto get_occupancy() const noexcept { return occupancy; }

  [[nodiscard]] constexpr auto get_king_square() const noexcept {
    for (const auto [piece, shift] : *this) {
      if (piece == piece::king) {
        return shift;
      }
    }
    assert(false); // FIXME: std::unreachable();
  }

  class iterator {
    uint64_t occupancy, pieces;

  public:
    using difference_type = std::ptrdiff_t;
    struct value_type {
      piece piece;
      square square;
    };

    constexpr iterator(const side &side) : occupancy(side.occupancy), pieces(side.pieces) {}

    constexpr value_type operator*() const noexcept {
      return value_type(static_cast<piece>(pieces & 0xF), std::countr_zero(occupancy));
    }

    constexpr iterator &operator++() {
      occupancy ^= occupancy & -occupancy; // Clear lowest set bit.
      pieces >>= 4;
      return *this;
    }

    constexpr iterator operator++(int) {
      auto old = *this;
      ++*this;
      return old;
    }

    constexpr bool operator==(std::default_sentinel_t) const noexcept { return occupancy == 0; }
  };

  [[nodiscard]] constexpr iterator begin() const noexcept { return *this; };
  [[nodiscard]] constexpr std::default_sentinel_t end() const noexcept { return {}; };
  [[nodiscard]] constexpr size_t size() const noexcept { return std::popcount(occupancy); };
};

static_assert(std::ranges::sized_range<side>);

class move {
  int src_square, dst_square;

  template <uint64_t L, uint64_t R = L>
  [[nodiscard]] constexpr uint64_t straight_path() const noexcept {
    if (src_square < dst_square) {
      return (L << src_square) ^ (L << dst_square);
    } else {
      return (R >> (63 ^ src_square)) ^ (R >> (63 ^ dst_square));
      // static_assert((63 - x) == (63 ^ x)); // forall 0 <= x < 64
    }
  }

public:
  constexpr move(const square src_square, const square dst_square) noexcept
      : src_square(src_square), dst_square(dst_square) {}

  [[nodiscard]] constexpr uint64_t src() const noexcept { return uint64_t{1} << src_square; }
  [[nodiscard]] constexpr uint64_t src(const uint64_t mask) const noexcept { return mask & src(); }
  [[nodiscard]] constexpr uint64_t src(const side side) const noexcept {
    return src(side.get_occupancy());
  }

  [[nodiscard]] constexpr uint64_t dst() const noexcept { return uint64_t{1} << dst_square; }
  [[nodiscard]] constexpr uint64_t dst(const uint64_t mask) const noexcept { return mask & dst(); }
  [[nodiscard]] constexpr uint64_t dst(const side side) const noexcept {
    return dst(side.get_occupancy());
  }

  [[nodiscard]] constexpr uint64_t exclude_dst_from(uint64_t mask) const noexcept {
    return mask & ~dst();
  }

  /** Returns rank-difference followed by file-difference. */
  [[nodiscard]] constexpr auto diff() const noexcept {
    return std::div(dst_square - src_square, 8);
  }

  /** The four cardinal directions: north, south, east, and west. */
  [[nodiscard]] constexpr std::optional<uint64_t> cardinal_path() const noexcept {
    const auto [rank_diff, file_diff] = diff();
    if (rank_diff == 0) { // same rank
      constexpr uint64_t full = 0xFFFF'FFFF'FFFF'FFFF;
      return straight_path<full>();
    }
    if (file_diff == 0) { // same file
      constexpr uint64_t rightmost_file = 0x0101'0101'0101'0101;
      constexpr uint64_t leftmost_file = 0x8080'8080'8080'8080;
      return straight_path<rightmost_file, leftmost_file>();
    }
    return std::nullopt;
  };

  /** The four ordinal directions: northeast, southeast, southwest, and northwest. */
  [[nodiscard]] constexpr std::optional<uint64_t> ordinal_path() const noexcept {
    const auto [rank_diff, file_diff] = diff();
    if (rank_diff == file_diff) { // northwest-southeast
      constexpr uint64_t major_diagonal = 0x8040'2010'0804'0201;
      return straight_path<major_diagonal>();
    }
    if (rank_diff == -file_diff) { // northeast-southwest
      constexpr uint64_t minor_diagonal_h1 = 0x0204'0810'2040'8001;
      constexpr uint64_t minor_diagonal_a8 = 0x8001'0204'0810'2040;
      return straight_path<minor_diagonal_h1, minor_diagonal_a8>();
    }
    return std::nullopt;
  };
};

class configuration {
  side white, black;

  constexpr configuration(const side white, const side black) : white(white), black(black) {
    // Pieces of different colors DO NOT share any square.
    assert((black.get_occupancy() & white.get_occupancy()) == 0);

    constexpr auto is_king = [](const side::iterator::value_type v) {
      return v.piece == piece::king;
    };
    // There is exactly 1 king for each side.
    assert(std::ranges::count_if(white, is_king) == 1);
    assert(std::ranges::count_if(black, is_king) == 1);
  }

  [[nodiscard]] constexpr bool empty(const uint64_t mask) const noexcept {
    return !(mask & (black.get_occupancy() ^ white.get_occupancy()));
  }

  [[nodiscard]] constexpr bool check(const bool is_white) const noexcept {
    const auto dst_square = (is_white ? white : black).get_king_square();
    const auto opponent = is_white ? black : white;
    for (const auto [piece, shift] : opponent) {
      if (test_move(piece, move(shift, dst_square))) {
        return true;
      }
    }
    return false;
  }

public:
  constexpr configuration() : configuration(side::initial_white(), side::initial_black()) {}

  /**
   * 1. Must not leave your king attacked after the move.
   * 2. Stalemate?
   */
  [[nodiscard]] constexpr std::optional<configuration> try_move(const piece p, const move m) const {
    // TODO: escape if in check; otherwise, checkmate.

    // src must not be empty.
    if (!m.src(black) && !m.src(white)) {
      return std::nullopt;
    }

    // src and dst are NOT of the same color; dst could be empty.
    if (m.src(black) && m.dst(black)) {
      return std::nullopt;
    }
    if (m.src(white) && m.dst(white)) {
      return std::nullopt;
    }

    // assert(src != dst); // implied by the previous conditions

    if (!test_move(p, m)) {
      return std::nullopt;
    }
    if (check(m.src(white))) {
      return std::nullopt;
    }

    const auto both_ends = m.src(m.dst());
    return configuration{white, black}; // FIXME: update white and black
  }

  [[nodiscard]] constexpr bool test_move(const piece p, const move m) const {
    switch (p) {
    case piece::pawn:
      // TODO: advancing 1 square
      // TODO: advancing 2 squares on first move
      // TODO: regular capturing
      // TODO: capturing en passant
      // TODO: promotion at last rank
      if (m.src(black)) {
        // TODO: move black pawn.
      } else { // Move white pawn.
        constexpr uint64_t last_rank = 0xFF00'0000'0000'0000;
        // There are NO pawns at the last rank. It must have been promoted.
        assert(!m.src(last_rank));
        const auto [rank_diff, file_diff] = m.diff();
        switch (rank_diff) {
        case 1: // Advancing 1 square.
          switch (file_diff) {
          case 0: // Moving on the same file without capture.
            assert(empty(m.dst()));
            if (m.dst(last_rank)) {
              // TODO: require promotion of the same color as an immediate next step.
            }
            break;
          case -1:
          case 1:
            if (empty(m.dst())) {
              // TODO: capture en passant.
            } else { // Capture regularly.
              // Guaranteed to be capturing opponent's piece.
            }
            break;
          default:
            assert(false);
          }
          break;
        case 2:                                 // Advancing 2 squares.
          assert(file_diff == 0);               // MUST move on the same file.
          assert(m.src(0x0000'0000'0000'FF00)); // MUST be first move.
          assert(empty(m.dst()));
          // TODO: signal potential capturing en passant for the immediate next step.
          break;
        default:
          assert(false);
        }
      }
      return true;
    case piece::king: {
      const auto [rank_diff, file_diff] = m.diff();
      return std::max(std::abs(rank_diff), std::abs(file_diff)) == 1; // max-norm
    }
    case piece::knight: {
      const auto [rank_diff, file_diff] = m.diff();
      return std::abs(rank_diff * file_diff) == 2;
    }
    case piece::rook:
      if (const auto path = m.cardinal_path()) {
        return empty(m.src(*path)); // All squares between src and dst are empty.
      }
      return false;
    case piece::bishop:
      if (const auto path = m.ordinal_path()) {
        return empty(m.src(*path)); // All squares between src and dst are empty.
      }
      return false;
    case piece::queen:
      if (const auto path = m.cardinal_path()) {
        return empty(m.src(*path)); // All squares between src and dst are empty.
      }
      if (const auto path = m.ordinal_path()) {
        return empty(m.src(*path)); // All squares between src and dst are empty.
      }
      return false;
    case piece::empty:
      return false;
    }
    std::unreachable();
  }

  friend std::ostream &operator<<(std::ostream &os, const configuration &config);
};

struct ply {
  configuration config;
  bool white_turn;
};

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
  for (const auto [piece, shift] : config.white) {
    board[63 ^ shift] = piece;
  }
  for (const auto [piece, shift] : config.black) {
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
      os << *bgcolor++ << colored_piece(*square++, pos & config.white.get_occupancy());
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
  assert(tcgetattr(STDOUT_FILENO, &t) == 0);
  static const auto initial_termios = t;
  std::atexit([] {
    std::cout << ansi::cursor_position(11, 1) << std::flush;
    assert_tcsetattr(STDOUT_FILENO, TCSANOW, initial_termios);
  });
  t.c_lflag &= ~(ECHO | ICANON);
  assert_tcsetattr(STDOUT_FILENO, TCSANOW, t);

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
  while (true) {
    char ch{};
    constexpr auto nbyte = sizeof(ch);
    assert(read(STDIN_FILENO, &ch, nbyte) == nbyte);
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

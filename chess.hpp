#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <ranges>
#include <utility>

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

  [[nodiscard]] constexpr const auto &get_white() const noexcept { return white; }
  [[nodiscard]] constexpr const auto &get_black() const noexcept { return black; }

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
      return std::max(::abs(rank_diff), std::abs(file_diff)) == 1; // max-norm
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
};

struct ply {
  configuration config;
  bool white_turn;
};

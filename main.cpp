#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <utility>

// chess board: lower-left is the dark square a1 with white rook
// d1 is white queen, e1 is white king
// alphabet for files (columns), digits for ranks (rows)
// upper-left is msb, lower-right is lsb

class board {
  const uint64_t black, white, pawn, rook, knight, bishop, queen, king;

public:
  using iterator = unsigned _BitInt(6);

  class move {
    const int src_shift, dst_shift;

    template <uint64_t L, uint64_t R = L> [[nodiscard]] constexpr uint64_t path() const noexcept {
      if (src_shift < dst_shift) {
        return (L << src_shift) ^ (L << dst_shift);
      } else {
        return (R >> (63 ^ src_shift)) ^ (R >> (63 ^ dst_shift));
        // static_assert((63 - x) == (63 ^ x)); // forall 0 <= x < 64
      }
    }

  public:
    move(const iterator src_shift, const iterator dst_shift)
        : src_shift(src_shift), dst_shift(dst_shift) {}

    [[nodiscard]] constexpr uint64_t src() const noexcept { return 1 << src_shift; }
    [[nodiscard]] constexpr uint64_t src(uint64_t mask) const noexcept { return mask & src(); }
    [[nodiscard]] constexpr uint64_t dst() const noexcept { return 1 << dst_shift; }
    [[nodiscard]] constexpr uint64_t dst(uint64_t mask) const noexcept { return mask & dst(); }
    [[nodiscard]] constexpr uint64_t exclude_dst_from(uint64_t mask) const noexcept {
      return mask & ~dst();
    }

    /** Returns rank-difference followed by file-difference. */
    [[nodiscard]] constexpr auto diff() const noexcept {
      return std::div(dst_shift - src_shift, 8);
    }

    /** The absolute value of the product of rank-difference and file-difference. */
    [[nodiscard]] constexpr int mul_diff() const noexcept {
      const auto [rank_diff, file_diff] = diff();
      return std::abs(rank_diff * file_diff);
    }

    /** The four cardinal directions: north, south, east, and west. */
    [[nodiscard]] constexpr std::optional<uint64_t> cardinal_path() const noexcept {
      const auto [rank_diff, file_diff] = diff();
      if (rank_diff == 0) { // same rank
        return path<0xFFFF'FFFF'FFFF'FFFF>();
      }
      if (file_diff == 0) { // same file
        return path<0x0101'0101'0101'0101, 0x8080'8080'8080'8080>();
      }
      return std::nullopt;
    };

    /** The four ordinal directions: northeast, southeast, southwest, and northwest. */
    [[nodiscard]] constexpr std::optional<uint64_t> ordinal_path() const noexcept {
      const auto [rank_diff, file_diff] = diff();
      if (rank_diff == file_diff) { // northwest-southeast
        return path<0x8040'2010'0804'0201>();
      }
      if (rank_diff == -file_diff) { // northeast-southwest
        return path<0x0204'0810'2040'8001, 0x8001'0204'0810'2040>();
      }
      return std::nullopt;
    };
  };

private:
  [[nodiscard]] constexpr bool empty(const uint64_t mask) const noexcept {
    return !(mask & (black ^ white));
  }

  [[nodiscard]] constexpr bool check(const bool is_white) const noexcept {
    const auto dst_shift = std::countr_zero(king & (is_white ? white : black));
    uint64_t opponent = is_white ? black : white;
    do { // `opponent` will never begin as 0, as there is always a king.
      const uint64_t attacker = opponent & -opponent; // Lowest set bit.
      if (test_move(move(std::countr_zero(attacker), dst_shift)) != test_move_result_t::invalid) {
        return true;
      }
      opponent ^= attacker;
    } while (opponent);
    return false;
  }

public:
  constexpr board(board &other) = default;
  constexpr board(board &&other) = default;
  board &operator=(board &) = delete;
  board &operator=(board &&) = delete;
  ~board() = default;

  /** Initial placement */
  constexpr board()
      : board(0xFFFF'0000'0000'0000, 0x0000'0000'0000'FFFF, 0x00FF'0000'0000'FF00,
              0x8100'0000'0000'0081, 0x4200'0000'0000'0042, 0x2400'0000'0000'0024,
              0x1000'0000'0000'0010, 0x0800'0000'0000'0008) {}

  constexpr board(const uint64_t black, const uint64_t white, const uint64_t pawn,
                  const uint64_t rook, const uint64_t knight, const uint64_t bishop,
                  const uint64_t queen, const uint64_t king)
      : black(black), white(white), pawn(pawn), rook(rook), knight(knight), bishop(bishop),
        queen(queen), king(king) {
    // There are exactly 2 kings.
    assert(std::popcount(king) == 2);

    // Pieces can be partitioned by color or type.
    assert((black ^ white) == (pawn ^ rook ^ knight ^ bishop ^ queen ^ king));

    // Pieces of different colors DO NOT share any square.
    assert((black & white) == 0);

    // Pieces of different types DO NOT share any square.
    const std::array types{pawn, rook, knight, bishop, queen, king};
    for (auto first = types.begin(); first != types.end(); ++first) {
      for (auto second = std::next(first); second != types.end(); ++second) {
        assert((*first & *second) == 0);
      }
    }
  }

  /**
   * 1. Must not leave your king attacked after the move.
   * 2. Stalemate?
   */
  [[nodiscard]] constexpr std::optional<board> try_move(const move m) const {
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

    auto new_pawn = m.exclude_dst_from(pawn);
    auto new_rook = m.exclude_dst_from(rook);
    auto new_knight = m.exclude_dst_from(knight);
    auto new_bishop = m.exclude_dst_from(bishop);
    auto new_queen = m.exclude_dst_from(queen);
    auto new_king = m.exclude_dst_from(king);

    switch (const auto both_ends = m.src(m.dst()); test_move(m)) {
    case test_move_result_t::invalid:
      return std::nullopt;
    case test_move_result_t::pawn:
      new_pawn ^= both_ends;
      break;
    case test_move_result_t::rook:
      new_rook ^= both_ends;
      break;
    case test_move_result_t::knight:
      new_knight ^= both_ends;
      break;
    case test_move_result_t::bishop:
      new_bishop ^= both_ends;
      break;
    case test_move_result_t::queen:
      new_queen ^= both_ends;
      break;
    case test_move_result_t::king:
      new_king ^= both_ends;
      break;
    }

    if (check(m.src(white))) {
      return std::nullopt;
    }
    return board{
        black ^ m.src(black) ^ m.dst(black),
        white ^ m.src(white) ^ m.dst(white),
        new_pawn,
        new_rook,
        new_knight,
        new_bishop,
        new_queen,
        new_king,
    };
  }

  enum class test_move_result_t { invalid, pawn, rook, knight, bishop, queen, king };

  [[nodiscard]] constexpr test_move_result_t test_move(const class move m) const {
    if (m.src(pawn)) {
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
      return test_move_result_t::pawn;
    } else if (m.src(king)) {
      if (m.mul_diff() < 2) {
        return test_move_result_t::king;
      } else {
        // TODO: castling; no suicide.
      }
    } else if (m.src(knight)) {
      if (m.mul_diff() == 2) {
        return test_move_result_t::knight;
      }
    } else if (m.src(rook)) {
      if (const auto path = m.cardinal_path()) {
        if (empty(m.src(*path))) { // All squares between src and dst are empty.
          return test_move_result_t::rook;
        }
      }
    } else if (m.src(bishop)) {
      if (const auto path = m.ordinal_path()) {
        if (empty(m.src(*path))) { // All squares between src and dst are empty.
          return test_move_result_t::bishop;
        }
      }
    } else if (m.src(queen)) {
      if (const auto path = m.cardinal_path()) {
        if (empty(m.src(*path))) { // All squares between src and dst are empty.
          return test_move_result_t::queen;
        }
      } else if (const auto path = m.ordinal_path()) {
        if (empty(m.src(*path))) { // All squares between src and dst are empty.
          return test_move_result_t::queen;
        }
      }
    } else {
      std::unreachable();
    }
    return test_move_result_t::invalid;
  }
};

int main() { board initial; }

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <optional>

// chess board: lower-left is the dark square a1 with white rook
// d1 is white queen, e1 is white king
// alphabet for files (columns), digits for ranks (rows)
// upper-left is msb, lower-right is lsb

class board {
  const uint64_t black, white, pawn, rook, knight, bishop, queen, king;

  class move {
    const int src_shift, dst_shift;

  public:
    move(move &) = delete;
    move(move &&) = delete;
    auto operator=(move &) = delete;
    auto &operator=(move &&) = delete;
    ~move() = default;

    constexpr move(int src_shift, int dst_shift) : src_shift(src_shift), dst_shift(dst_shift) {
      assert(0 <= src_shift && src_shift < 64);
      assert(0 <= dst_shift && dst_shift < 64);
    }

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

    template <uint64_t L, uint64_t R = L> [[nodiscard]] constexpr uint64_t path() const noexcept {
      if (src_shift < dst_shift) {
        return (L << src_shift) ^ (L << dst_shift);
      } else {
        return (R >> (63 ^ src_shift)) ^ (R >> (63 ^ dst_shift));
        // static_assert((63 - x) == (63 ^ x)); // forall 0 <= x < 64
      }
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

    /** The absolute value of the product of rank-difference and file-difference. */
    [[nodiscard]] constexpr int mul_diff() const noexcept {
      const auto [rank_diff, file_diff] = diff();
      return std::abs(rank_diff * file_diff);
    }
  };

  [[nodiscard]] constexpr bool empty(uint64_t mask) const noexcept {
    return !(mask & (black ^ white));
  }

  [[nodiscard]] constexpr bool check() const noexcept {
    return false; // TODO
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

  constexpr board(uint64_t black, uint64_t white, uint64_t pawn, uint64_t rook, uint64_t knight,
                  uint64_t bishop, uint64_t queen, uint64_t king)
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
  [[nodiscard]] constexpr board move(int src_shift, int dst_shift) const {
    const class move m(src_shift, dst_shift);
    // TODO: escape if in check; otherwise, checkmate.

    // src must not be empty.
    assert(m.src(black) || m.src(white));

    // src and dst are NOT of the same color; dst could be empty.
    assert(!(m.src(black) && m.dst(black)));
    assert(!(m.src(white) && m.dst(white)));

    // assert(src != dst); // implied by the previous conditions

    auto new_pawn = m.exclude_dst_from(pawn);
    auto new_rook = m.exclude_dst_from(rook);
    auto new_knight = m.exclude_dst_from(knight);
    auto new_bishop = m.exclude_dst_from(bishop);
    auto new_queen = m.exclude_dst_from(queen);
    auto new_king = m.exclude_dst_from(king);

    if (const auto both_ends = m.src(m.dst()); m.src(pawn)) {
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
      new_pawn ^= both_ends;
    } else if (m.src(king)) {
      // TODO: castling; no suicide.
      assert(m.mul_diff() < 2);
      new_king ^= both_ends;
    } else if (m.src(knight)) {
      assert(m.mul_diff() == 2);
      new_knight ^= both_ends;
    } else if (m.src(rook)) {
      if (const auto path = m.cardinal_path()) {
        // All squares between src and dst are empty.
        assert(empty(m.src(*path)));
      } else {
        assert(false);
      }
      new_rook ^= both_ends;
    } else if (m.src(bishop)) {
      if (const auto path = m.ordinal_path()) {
        // All squares between src and dst are empty.
        assert(empty(m.src(*path)));
      } else {
        assert(false);
      }
      new_bishop ^= both_ends;
    } else if (m.src(queen)) {
      if (const auto path = m.cardinal_path()) {
        // All squares between src and dst are empty.
        assert(empty(m.src(*path)));
      } else if (const auto path = m.ordinal_path()) {
        // All squares between src and dst are empty.
        assert(empty(m.src(*path)));
      } else {
        assert(false);
      }
      new_queen ^= both_ends;
    } else {
      std::unreachable();
    }

    // TODO: assert not in check.
    return {
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
};

int main() { board initial; }

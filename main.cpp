#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <utility>

// chess board: lower-left is the dark square a1 with white rook
// d1 is white queen, e1 is white king
// alphabet for files (columns), digits for ranks (rows)
// upper-left is msb, lower-right is lsb

struct board {
  const uint64_t black, white, pawn, rook, knight, bishop, queen, king;

  board(board &) = delete;
  board(board &&) = delete;
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
  constexpr board move(int src_shift, int dst_shift) {
    assert(0 <= src_shift && src_shift < 64);
    assert(0 <= dst_shift && dst_shift < 64);
    const uint64_t src = 1 << src_shift, dst = 1 << dst_shift;

    // src must not be empty.
    assert((src & black) || (src & white));

    // src and dst are NOT of the same color; dst could be empty.
    assert(!((src & black) && (dst & black)));
    assert(!((src & white) && (dst & white)));

    // assert(src != dst); // implied by the previous conditions

    if (src & pawn) {
      // TODO: advancing 1 square
      // TODO: advancing 2 squares on first move
      // TODO: regular capturing (diagonally forward)
      // TODO: capturing en passant
      // TODO: promotion at last rank
      if (src & black) {
        // TODO: move black pawn
      } else {
        // TODO: move white pawn
      }
    } else if (src & rook) {
      int delta;
      if (src_shift / 8 == dst_shift / 8) { // same rank
        delta = src < dst ? 1 : -1;
      } else if (src_shift % 8 == dst_shift % 8) { // same file
        delta = src < dst ? 8 : -8;
      } else {
        assert(false);
      }
      // All squares between src and dst are empty.
      for (auto square = src; (square += delta) != dst;) {
        assert((square & (black ^ white)) == 0);
      }
    } else if (src & knight) {
      const auto diff = std::abs(dst_shift - src_shift);
      assert((diff / 8) * (diff % 8) == 2);
    } else if (src & bishop) {
      const auto diff = dst_shift - src_shift;
      const auto rank_diff = diff / 8; // FIXME: what if negative?
      const auto file_diff = diff % 8; // FIXME: what if negative?
      assert(std::abs(rank_diff) == std::abs(file_diff));
      const auto delta = rank_diff < 0 ? (file_diff < 0 ? -9 : -7) : (file_diff < 0 ? 7 : 9);
      // All squares between src and dst are empty.
      for (auto square = src; (square += delta) != dst;) {
        assert((square & (black ^ white)) == 0);
      }
    } else if (src & queen) {
      // TODO: combination of rook and bishop
    } else if (src & king) {
      // TODO: castling; no suicide
      const auto diff = std::abs(dst_shift - src_shift);
      assert((diff / 8) * (diff % 8) <= 1);
    } else {
      std::unreachable();
    }

    const auto new_black = black ^ (src & black) ^ (dst & black);
    const auto new_white = white ^ (src & white) ^ (dst & white);
  }
};

int main() { board initial; }

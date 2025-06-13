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

    auto new_pawn = pawn;
    auto new_rook = rook;
    auto new_knight = knight;
    auto new_bishop = bishop;
    auto new_queen = queen;
    auto new_king = king;

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
      new_pawn ^= src ^ dst;
    } else if (src & rook) {
      const auto shift_diff = dst_shift - src_shift;
      const auto rank_diff = shift_diff / 8;
      const auto file_diff = shift_diff % 8;
      uint64_t path = src;
      if (rank_diff == 0) { // same rank
        if (src < dst) {
          path ^= dst - src;
        } else {
          path ^= src - dst;
        }
      } else if (file_diff == 0) { // same file
        if (src < dst) {
          constexpr uint64_t leftmost_file = 0x0101'0101'0101'0101;
          path ^= (leftmost_file << src_shift) ^ (leftmost_file << dst_shift);
        } else {
          constexpr uint64_t rightmost_file = 0x8080'8080'8080'8080;
          path ^= (rightmost_file >> (63 - src_shift)) ^ (rightmost_file >> (63 - dst_shift));
        }
      } else {
        assert(false);
      }
      // All squares between src and dst are empty.
      assert((path & (black ^ white)) == 0);
      new_rook ^= src ^ dst;
    } else if (src & knight) {
      const auto shift_diff = std::abs(dst_shift - src_shift);
      assert((shift_diff / 8) * (shift_diff % 8) == 2);
      new_knight ^= src ^ dst;
    } else if (src & bishop) {
      const auto shift_diff = dst_shift - src_shift;
      const auto rank_diff = shift_diff / 8;
      const auto file_diff = shift_diff % 8;
      uint64_t path = src;
      if (rank_diff == file_diff) {
        const uint64_t major_diag = 0x8040'2010'0804'0201;
        if (src < dst) {
          path ^= (major_diag << src_shift) ^ (major_diag << dst_shift);
        } else {
          path ^= (major_diag >> (63 - src_shift)) ^ (major_diag >> (63 - dst_shift));
        }
      } else if (rank_diff == -file_diff) {
        if (src < dst) {
          const uint64_t diag = 0x0204'0810'2040'8001;
          path ^= (diag << src_shift) ^ (diag << dst_shift);
        } else {
          const uint64_t diag = 0x8001'0204'0810'2040;
          path ^= (diag >> (63 - src_shift)) ^ (diag >> (63 - dst_shift));
        }
      } else {
        assert(false);
      }
      // All squares between src and dst are empty.
      assert((path & (black ^ white)) == 0);
      new_bishop ^= src ^ dst;
    } else if (src & queen) { // The combination of rook and bishop.
      const auto shift_diff = dst_shift - src_shift;
      const auto rank_diff = shift_diff / 8;
      const auto file_diff = shift_diff % 8;
      uint64_t path = src;
      if (rank_diff == 0) { // same rank
        if (src < dst) {
          path ^= dst - src;
        } else {
          path ^= src - dst;
        }
      } else if (file_diff == 0) { // same file
        if (src < dst) {
          constexpr uint64_t leftmost_file = 0x0101'0101'0101'0101;
          path ^= (leftmost_file << src_shift) ^ (leftmost_file << dst_shift);
        } else {
          constexpr uint64_t rightmost_file = 0x8080'8080'8080'8080;
          path ^= (rightmost_file >> (63 - src_shift)) ^ (rightmost_file >> (63 - dst_shift));
        }
      } else if (rank_diff == file_diff) {
        const uint64_t major_diag = 0x8040'2010'0804'0201;
        if (src < dst) {
          path ^= (major_diag << src_shift) ^ (major_diag << dst_shift);
        } else {
          path ^= (major_diag >> (63 - src_shift)) ^ (major_diag >> (63 - dst_shift));
        }
      } else if (rank_diff == -file_diff) {
        if (src < dst) {
          const uint64_t diag = 0x0204'0810'2040'8001;
          path ^= (diag << src_shift) ^ (diag << dst_shift);
        } else {
          const uint64_t diag = 0x8001'0204'0810'2040;
          path ^= (diag >> (63 - src_shift)) ^ (diag >> (63 - dst_shift));
        }
      } else {
        assert(false);
      }
      // All squares between src and dst are empty.
      assert((path & (black ^ white)) == 0);
      new_queen ^= src ^ dst;
    } else if (src & king) {
      // TODO: castling; no suicide.
      const auto shift_diff = std::abs(dst_shift - src_shift);
      assert((shift_diff / 8) * (shift_diff % 8) <= 1);
      new_king ^= src ^ dst;
    } else {
      std::unreachable();
    }

    if (dst & pawn) {
      new_pawn ^= dst;
    } else if (dst & rook) {
      new_rook ^= dst;
    } else if (dst & knight) {
      new_knight ^= dst;
    } else if (dst & bishop) {
      new_bishop ^= dst;
    } else if (dst & queen) {
      new_queen ^= dst;
    } else if (dst & king) {
      new_king ^= dst;
    } else {
      std::unreachable();
    }

    return {
        black ^ (src & black) ^ (dst & black),
        white ^ (src & white) ^ (dst & white),
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

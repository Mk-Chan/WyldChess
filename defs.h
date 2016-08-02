#ifndef DEF_H
#define DEF_H

#define BB(x) (1ULL << (x))
#define MAX_PLY (128)

#define MOVE_TYPE_SHIFT (12)
#define PROM_TYPE_SHIFT (14)
#define CAP_TYPE_SHIFT  (17)

#define MOVE_TYPE_MASK (3 << MOVE_TYPE_SHIFT) 
#define PROM_TYPE_MASK (7 << PROM_TYPE_SHIFT)
#define CAP_TYPE_MASK  (7 << CAP_TYPE_SHIFT)

typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef unsigned __int128   u128;

#ifdef HASH_128_BIT
  typedef u128 HashKey;
#else
  typedef u64  HashKey;
#endif

enum Color {
  WHITE = 0,
  BLACK
};

enum BitboardConstants {
  PAWN = 2,
  KNIGHT,
  BISHOP,
  ROOK,
  QUEEN,
  KING,
  PINNED,
  FULL
};

enum PieceWithColors {
  WP = 2, WN, WB, WR, WQ, WK,
  BP = 10, BN, BB, BR, BQ, BK
};

enum Squares {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
};

enum Ranks {
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
};

enum Files {
  FILE_1,
  FILE_2,
  FILE_3,
  FILE_4,
  FILE_5,
  FILE_6,
  FILE_7,
  FILE_8
};

enum CastlingRights {
  WKC = 1,
  WQC = 2,
  BKC = 4,
  BQC = 8
};

enum MoveType {
  NORMAL,
  CASTLE    = 1 << MOVE_TYPE_SHIFT,
  ENPASSANT = 2 << MOVE_TYPE_SHIFT,
  PROMOTION = 3 << MOVE_TYPE_SHIFT
};

enum PromotionType {
  TO_KNIGHT = KNIGHT << PROM_TYPE_SHIFT,
  TO_BISHOP = BISHOP << PROM_TYPE_SHIFT,
  TO_ROOK   = ROOK   << PROM_TYPE_SHIFT,
  TO_QUEEN  = QUEEN  << PROM_TYPE_SHIFT
};

enum CaptureType {
  PAWN_CAP   = PAWN   << CAP_TYPE_SHIFT,
  KNIGHT_CAP = KNIGHT << CAP_TYPE_SHIFT,
  BISHOP_CAP = BISHOP << CAP_TYPE_SHIFT,
  ROOK_CAP   = ROOK   << CAP_TYPE_SHIFT,
  QUEEN_CAP  = QUEEN  << CAP_TYPE_SHIFT
};

#define make_piece(pt, c)  (pt | (c << 3))
#define piece_type(piece)  (piece & 7)
#define piece_color(piece) (piece >> 3)

#define rank_of(sq) (sq >> 3)
#define file_of(sq) (sq & 7)

#define from_sq(m)    (m & 0x3f)
#define to_sq(m)      ((m >> 6) & 0x3f)
#define move_type(m)  (m & MOVE_TYPE_MASK)
#define prom_type(m)  ((m & PROM_TYPE_MASK) >> PROM_TYPE_SHIFT)
#define cap_type(m)   (m >> CAP_TYPE_SHIFT)

#define popcnt(bb)  (__builtin_popcountll(bb))
#define bitscan(bb) (__builtin_ffsll(bb) - 1)

#define move_normal(from, to)     (from | (to << 6) | NORMAL)
#define move_castle(from, to)     (from | (to << 6) | CASTLE)
#define move_ep(from, to)         (from | (to << 6) | ENPASSANT)
#define move_prom(from, to, prom) (from | (to << 6) | PROMOTION | prom)
#define move(from, to, mt, prom)  (from | (to << 6) | mt | prom)

#endif

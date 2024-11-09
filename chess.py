import sys, time, os
from collections import deque
from typing import List, Tuple, Set
from dataclasses import dataclass
from copy import deepcopy
from sortedcontainers import SortedList
class SquareError(Exception):
    def __init__(self, msg):
        super().__init__(msg)

def timer(f):
    def wrapper(*args, **kwargs):
        start = time.time()
        result = f(*args, **kwargs)
        print(f'{(time.time() - start)*1000:.5f}')
        return result
    return wrapper

class Board:
    _piece_map = {
        0: '_white_bishops', 1: '_white_king', 2: '_white_knight', 3: '_white_pawns',
        4: '_white_queen', 5: '_white_rooks', 6: '_black_bishops', 7: '_black_king',
        8: '_black_knight', 9: '_black_pawns', 10: '_black_queen', 11: '_black_rooks'
    }

    _vals = [3,100,3,1,9,5,-3,-100,-3,-1,-9,-5]

    _black_rooks = int.from_bytes(b'\x81\x00\x00\x00\x00\x00\x00\x00', 'big')
    _black_knight = int.from_bytes(b'\x42\x00\x00\x00\x00\x00\x00\x00', 'big')
    _black_bishops = int.from_bytes(b'\x24\x00\x00\x00\x00\x00\x00\x00', 'big')
    _black_queen = int.from_bytes(b'\x10\x00\x00\x00\x00\x00\x00\x00', 'big')
    _black_king = int.from_bytes(b'\x08\x00\x00\x00\x00\x00\x00\x00', 'big')
    _black_pawns = int.from_bytes(b'\x00\xff\x00\x00\x00\x00\x00\x00', 'big')
    _white_rooks = int.from_bytes(b'\x00\x00\x00\x00\x00\x00\x00\x81', 'big')
    _white_knight = int.from_bytes(b'\x00\x00\x00\x00\x00\x00\x00\x42', 'big')
    _white_bishops = int.from_bytes(b'\x00\x00\x00\x00\x00\x00\x00\x24', 'big')
    _white_queen = int.from_bytes(b'\x00\x00\x00\x00\x00\x00\x00\x10', 'big')
    _white_king =  int.from_bytes(b'\x00\x00\x00\x00\x00\x00\x00\x08', 'big')
    _white_pawns = int.from_bytes(b'\x00\x00\x00\x00\x00\x00\xff\x00', 'big')

    def __init__(self):
        self.turn = 'white'
        self._update_piece_masks()

    def _update_piece_masks(self):
        self._white_pieces = (self._white_pawns | self._white_knight | self._white_bishops |
                              self._white_rooks | self._white_queen | self._white_king)
        self._black_pieces = (self._black_pawns | self._black_knight | self._black_bishops |
                              self._black_rooks | self._black_queen | self._black_king)
        self._all_pieces = self._white_pieces | self._black_pieces

    @property
    def board(self):
        arr = bytearray()
        for piece in [
            self._white_bishops, self._white_king, self._white_knight, self._white_pawns,
            self._white_queen, self._white_rooks, self._black_bishops, self._black_king,
            self._black_knight, self._black_pawns, self._black_queen, self._black_rooks
        ]:
            arr.extend(piece.to_bytes(8, 'big'))
        return arr

    def move(self, start: str, to: str):
        if (start,to) not in self.possible_moves:
            raise SquareError("Invalid Move")
        x1, y1 = ord(start[0]) - 97, int(start[1]) - 1
        x2, y2 = ord(to[0]) - 97, int(to[1]) - 1
        from_square = (1 << (7-x1)) << (y1 * 8)
        to_square = (1 << (7-x2)) << (y2 * 8)
        starting_piece = None
        ending_piece = None

        for i in range(12):
            piece_data = getattr(self, self._piece_map[i])
            piece_int = piece_data
            if piece_int & from_square:
                updated = piece_int ^ from_square | to_square
                setattr(self, self._piece_map[i], updated)
                starting_piece = i
            elif piece_int & to_square:
                to_updated = piece_int ^ to_square
                setattr(self, self._piece_map[i], to_updated)
                ending_piece = i
            if starting_piece is not None and ending_piece is not None:
                break

        self._update_piece_masks()
        self.turn = 'black' if self.turn == 'white' else 'white'

    @property
    def evaluation(self):
        count = 0
        for i in range(12):
            bitboard = getattr(self, self._piece_map[i])
            count += bin(bitboard).count('1')*self._vals[i]
        return count

    @property
    def possible_moves(self) -> Set[Tuple[str,str]]:
        moves = set()

        knight_moves = [ ( 1, 2),( 2, 1),( 2,-1),( 1,-2),
                         (-1,-2),(-2,-1),(-2, 1),(-1, 2)]

        king_moves =   [ ( 1, 0),( 1, 1),( 0, 1),(-1, 1),
                         (-1, 0),(-1,-1),( 0,-1),( 1,-1)]

        if self.turn == 'white':
            piece_indices = range(0,6)
            opponent_indices = range(6,12)
        else:
            piece_indices = range(6,12)
            opponent_indices = range(0,6)

        own_pieces = 0
        for i in piece_indices:
            own_pieces |= getattr(self, self._piece_map[i])
        opponent_pieces = 0
        for i in opponent_indices:
            opponent_pieces |= getattr(self, self._piece_map[i])
        all_pieces = own_pieces | opponent_pieces

        for i in piece_indices:
            piece_bb = getattr(self, self._piece_map[i])
            piece_type_index = i
            piece_bitboard = piece_bb
            while piece_bitboard:
                lsb = piece_bitboard & -piece_bitboard
                bit_index = lsb.bit_length() - 1
                from_square = 63 - bit_index
                from_coord = index_to_coord(from_square)

                if i in [2,8]:
                    rank = from_square //8
                    file = from_square %8
                    for dx, dy in knight_moves:
                        x = file + dx
                        y = rank + dy
                        if x < 0 or x > 7 or y < 0 or y >7:
                            continue
                        to_square = y*8 + x
                        to_bit = 1 << (63 - to_square)
                        if to_bit & own_pieces:
                            continue
                        to_coord = index_to_coord(to_square)
                        moves.add( (from_coord, to_coord) )

                elif i in [5,11]:
                    for direction in [(1,0),(-1,0),(0,1),(0,-1)]:
                        moves.update(self._gen_sliding_moves(from_square, direction, own_pieces, opponent_pieces))

                elif i in [0,6]:
                    for direction in [(1,1),(1,-1),(-1,1),(-1,-1)]:
                        moves.update(self._gen_sliding_moves(from_square, direction, own_pieces, opponent_pieces))

                elif i in [4,10]:
                    for direction in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]:
                        moves.update(self._gen_sliding_moves(from_square, direction, own_pieces, opponent_pieces))

                elif i in [1,7]:
                    rank = from_square //8
                    file = from_square %8
                    for dx, dy in king_moves:
                        x = file + dx
                        y = rank + dy
                        if x < 0 or x > 7 or y < 0 or y >7:
                            continue
                        to_square = y*8 + x
                        to_bit = 1 << (63 - to_square)
                        if to_bit & own_pieces:
                            continue
                        to_coord = index_to_coord(to_square)
                        moves.add((from_coord, to_coord))

                elif i in [3,9]:
                    moves.update(self._pawn_moves(from_square, own_pieces, opponent_pieces, self.turn))

                piece_bitboard &= piece_bitboard - 1

        return moves

    def _gen_sliding_moves(self, from_square: int, direction: Tuple[int, int], own_pieces: int, opponent_pieces: int) -> List[Tuple[str, str]]:
        moves = set()
        rank = from_square // 8
        file = from_square % 8
        dx, dy = direction
        x = file
        y = rank
        while True:
            x += dx
            y += dy
            if x < 0 or x >=8 or y < 0 or y >=8:
                break
            to_square = y * 8 + x
            to_bit = 1 << (63 - to_square)
            if to_bit & own_pieces:
                break
            from_coord = index_to_coord(from_square)
            to_coord = index_to_coord(to_square)
            moves.add( (from_coord, to_coord) )
            if to_bit & opponent_pieces:
                break
        return moves

    def _pawn_moves(self, from_square: int, own_pieces: int, opponent_pieces: int, color: str) -> List[Tuple[str,str]]:
        moves = set()
        rank = from_square //8
        file = from_square %8
        if color == 'white':
            direction = -1
            start_rank = 6
            promotion_rank = 0
            opponent_pawns = getattr(self, '_black_pawns')
        else:
            direction = 1
            start_rank = 1
            promotion_rank = 7
            opponent_pawns = getattr(self, '_white_pawns')
        forward_rank = rank + direction
        if 0 <= forward_rank <=7:
            to_square = forward_rank *8 + file
            to_bit = 1 << (63 - to_square)
            if not (to_bit & (own_pieces | opponent_pieces)):
                from_coord = index_to_coord(from_square)
                to_coord = index_to_coord(to_square)
                moves.add( (from_coord, to_coord) )
                if rank == start_rank:
                    forward_rank2 = rank + 2*direction
                    if 0 <= forward_rank2 <=7:
                        to_square2 = forward_rank2 *8 + file
                        to_bit2 = 1 << (63 - to_square2)
                        if not (to_bit2 & (own_pieces | opponent_pieces)):
                            to_coord2 = index_to_coord(to_square2)
                            moves.add( (from_coord, to_coord2) )

        for dx in [-1,1]:
            x = file + dx
            y = rank + direction
            if x <0 or x >7 or y < 0 or y >7:
                continue
            to_square = y*8 + x
            to_bit = 1 << (63 - to_square)
            if to_bit & opponent_pieces:
                from_coord = index_to_coord(from_square)
                to_coord = index_to_coord(to_square)
                moves.add( (from_coord, to_coord) )

        return moves

    def __str__(self):
        matrix = [["." for _ in range(8)] for _ in range(8)]
        pieces = {
            self._white_rooks: 'R', self._white_knight: 'N', self._white_bishops: 'B',
            self._white_queen: 'Q', self._white_king: 'K', self._white_pawns: 'P',
            self._black_rooks: 'r', self._black_knight: 'n', self._black_bishops: 'b',
            self._black_queen: 'q', self._black_king: 'k', self._black_pawns: 'p'
        }

        for piece_bytes, piece_char in pieces.items():
            piece_positions = piece_bytes
            for j in range(64):
                if piece_positions & (1 << (63 - j)):
                    row, col = divmod(j, 8)
                    matrix[row][col] = piece_char

        board_str = "\n".join(" ".join(row) for row in matrix)
        return board_str + '\n'

def index_to_coord(index: int) -> str:
    rank = index // 8
    file = index % 8
    rank_char = str(8 - rank)
    file_char = chr(ord('a') + file)
    return f"{file_char}{rank_char}"

@dataclass
class Move:
    evaluation : int
    move : Tuple[str,str]

class Game(Board):
    def __init__(self):
        super().__init__()
        self.moves = deque()
        self.white = None
        self.black = None

    def move(self, start : str, to : str):
        self.moves.append(self.board)
        super().move(start, to)
        # print(self)
        # print(self.possible_moves)

    @property
    def best_move(self, depth: int = 2) -> Move:

        def minimax(board, depth, maximizing_player):
            if depth == 0:
                return board.evaluation

            if maximizing_player:
                max_eval = -float('inf')
                for move in board.possible_moves:
                    new_board = deepcopy(board)
                    new_board.move(*move)
                    evaluation = minimax(new_board, depth - 1, False)
                    max_eval = max(max_eval, evaluation)
                return max_eval
            else:
                min_eval = float('inf')
                for move in board.possible_moves:
                    new_board = deepcopy(board)
                    new_board.move(*move)
                    evaluation = minimax(new_board, depth - 1, True)
                    min_eval = min(min_eval, evaluation)
                return min_eval

        best_evaluation = -float('inf') if self.turn == 'white' else float('inf')
        best_move = None

        for move in self.possible_moves:
            new_board = deepcopy(self)
            new_board.move(*move)
            evaluation = minimax(new_board, depth - 1, self.turn != 'white')

            if self.turn == 'white' and evaluation > best_evaluation:
                best_evaluation = evaluation
                best_move = move
            elif self.turn == 'black' and evaluation < best_evaluation:
                best_evaluation = evaluation
                best_move = move

        return Move(evaluation=best_evaluation, move=best_move)
"""

"""


if __name__ == '__main__':
    game = Game()
    game.move('b2', 'b3')
    game.move('e7','e5')
    game.move('c1','b2')
    game.move('b8','c6')
    game.move('f2','f3')
    game.move('d7','d5')
    game.move('b2','c3')
    game.move('d5','d4')
    game.move('c3','b2')
    game.move('f8','c5')
    game.move('e2','e4')
    game.move('g8','f6')
    game.move('f1','b5')
    game.move('a7','a6')
    game.move('b5','c4')
    game.move('b7','b5')
    game.move('c4','d3')
    game.move('c8','f5')
    game.move('e4','f5')
    game.move('e5','e4')
    game.move('f3','e4')
    game.move('h7','h5')
    print(game.best_move)

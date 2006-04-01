/* sudoku.c - sudoku game
 *
 * Writing a fun Su-Do-Ku game has turned out to be a difficult exercise.
 * The biggest difficulty is keeping the game fun - and this means allowing
 * the user to make mistakes. The game is not much fun if it prevents the
 * user from making moves, or if it informs them of an incorrect move.
 * With movement constraints, the 'game' is little more than an automated
 * solver (and no fun at all).
 *
 * Another challenge is generating good puzzles that are entertaining to
 * solve. It is certainly true that there is an art to creating good
 * Su-Do-Ku puzzles, and that good hand generated puzzles are more
 * entertaining than many computer generated puzzles - I just hope that
 * the algorithm implemented here provides fun puzzles. It is an area
 * that needs work. The puzzle classification is very simple, and could
 * also do with work. Finally, understanding the automatically generated
 * hints is sometimes more work than solving the puzzle - a better, and
 * more human friendly, mechanism is needed.
 *
 * Comments, suggestions, and contributions are always welcome - send email
 * to: mike 'at' laurasia.com.au. Note that this code assumes a single
 * threaded process, makes extensive use of global variables, and has
 * not been written to be reused in other applications. The code makes no
 * use of dynamic memory allocation, and hence, requires no heap. It should
 * also run with minimal stack space.
 *
 * This code and accompanying files have been placed into the public domain
 * by Michael Kennett, July 2005. It is provided without any warranty
 * whatsoever, and in no event shall Michael Kennett be liable for
 * any damages of any kind, however caused, arising from this software.
 */

#include "plugin.h"

#include "sudoku.h"
#include "templates.h"

extern struct plugin_api* rb;

#define assert(x)

/* Common state encoding in a 32-bit integer:
 *   bits  0-6    index
 *         7-15   state  [bit high signals digits not possible]
 *        16-19   digit
 *           20   fixed  [set if digit initially fixed]
 *           21   choice [set if solver chose this digit]
 *           22   ignore [set if ignored by reapply()]
 *           23   unused
 *        24-26   hint
 *        27-31   unused
 */
#define INDEX_MASK              0x0000007f
#define GET_INDEX(val)          (INDEX_MASK&(val))
#define SET_INDEX(val)          (val)

#define STATE_MASK              0x0000ff80
#define STATE_SHIFT             (7-1)                        /* digits 1..9 */
#define DIGIT_STATE(digit)      (1<<(STATE_SHIFT+(digit)))

#define DIGIT_MASK              0x000f0000
#define DIGIT_SHIFT             16
#define GET_DIGIT(val)          (((val)&DIGIT_MASK)>>(DIGIT_SHIFT))
#define SET_DIGIT(val)          ((val)<<(DIGIT_SHIFT))

#define FIXED                   0x00100000
#define CHOICE                  0x00200000
#define IGNORED                 0x00400000

/* Hint codes (c.f. singles(), pairs(), findmoves()) */
#define HINT_ROW                0x01000000
#define HINT_COLUMN             0x02000000
#define HINT_BLOCK              0x04000000

/* For a general board it may be necessary to do backtracking (i.e. to
 * rewind the board to an earlier state), and make choices during the
 * solution process. This can be implemented naturally using recursion,
 * but it is more efficient to maintain a single board.
 */
static int board[ 81 ];

/* Addressing board elements: linear array 0..80 */
#define ROW(idx)                ((idx)/9)
#define COLUMN(idx)             ((idx)%9)
#define BLOCK(idx)              (3*(ROW(idx)/3)+(COLUMN(idx)/3))
#define INDEX(row,col)          (9*(row)+(col))

/* Blocks indexed 0..9 */
#define IDX_BLOCK(row,col)      (3*((row)/3)+((col)/3))
#define TOP_LEFT(block)         (INDEX(block/3,block%3))

/* Board state */
#define STATE(idx)              ((board[idx])&STATE_MASK)
#define DIGIT(idx)              (GET_DIGIT(board[idx]))
#define HINT(idx)               ((board[idx])&HINT_MASK)
#define IS_EMPTY(idx)           (0 == DIGIT(idx))
#define DISALLOWED(idx,digit)   ((board[idx])&DIGIT_STATE(digit))
#define IS_FIXED(idx)           (board[idx]&FIXED)

/* Record move history, and maintain a counter for the current
 * move number. Concessions are made for the user interface, and
 * allow digit 0 to indicate clearing a square. The move history
 * is used to support 'undo's for the user interface, and hence
 * is larger than required - there is sufficient space to solve
 * the puzzle, undo every move, and then redo the puzzle - and
 * if the user requires more space, then the full history will be
 * lost.
 */
static int idx_history;
static int history[ 3 * 81 ];

/* Possible moves for a given board (c.f. fillmoves()).
 * Also used by choice() when the deterministic solver has failed,
 * and for calculating user hints. The number of hints is stored
 * in num_hints, or -1 if no hints calculated. The number of hints
 * requested by the user since their last move is stored in req_hints;
 * if the user keeps requesting hints, start giving more information.
 * Finally, record the last hint issued to the user; attempt to give
 * different hints each time.
 */
static int idx_possible;
static int possible[ 81 ];

static int pass;    /* count # passes of deterministic solver */

/* Support for template file */
static int tmplt[ 81 ];             /* Template indices */
static int len_tmplt;               /* Number of template indices */

/* Reset global state */
static
void
reset( void )
{
    rb->memset( board, 0x00, sizeof( board ) );
    rb->memset( history, 0x00, sizeof( history ) );
    idx_history = 0;
    pass = 0;
}

/* Management of the move history - compression */
static
void
compress( int limit )
{
    int i, j;
    for( i = j = 0 ; i < idx_history && j < limit ; ++i )
        if( !( history[ i ] & IGNORED ) )
            history[ j++ ] = history[ i ];
    for( ; i < idx_history ; ++i )
        history[ j++ ] = history[ i ];
    idx_history = j;
}

/* Management of the move history - adding a move */
static
void
add_move( int idx, int digit, int choice )
{
    int i;

    if( sizeof( history ) / sizeof( int ) == idx_history )
        compress( 81 );

    /* Never ignore the last move */
    history[ idx_history++ ] = SET_INDEX( idx ) | SET_DIGIT( digit ) | choice;

    /* Ignore all previous references to idx */
    for( i = idx_history - 2 ; 0 <= i ; --i )
        if( GET_INDEX( history[ i ] ) == idx )
        {
            history[ i ] |= IGNORED;
            break;
        }
}

/* Iteration over rows/columns/blocks handled by specialised code.
 * Each function returns a block index - call must manage element/idx.
 */
static
int
idx_row( int el, int idx )      /* Index within a row */
{
    return INDEX( el, idx );
}

static
int
idx_column( int el, int idx )   /* Index within a column */
{
    return INDEX( idx, el );
}

static
int
idx_block( int el, int idx )    /* Index within a block */
{
    return INDEX( 3 * ( el / 3 ) + idx / 3, 3 * ( el % 3 ) + idx % 3 );
}

/* Update board state after setting a digit (clearing not handled)
 */
static
void
update( int idx )
{
    const int row = ROW( idx );
    const int col = COLUMN( idx );
    const int block = IDX_BLOCK( row, col );
    const int mask = DIGIT_STATE( DIGIT( idx ) );
    int i;

    board[ idx ] |= STATE_MASK;  /* filled - no choice possible */

    /* Digit cannot appear in row, column or block */
    for( i = 0 ; i < 9 ; ++i )
    {
        board[ idx_row( row, i ) ] |= mask;
        board[ idx_column( col, i ) ] |= mask;
        board[ idx_block( block, i ) ] |= mask;
    }
}

/* Refresh board state, given move history. Note that this can yield
 * an incorrect state if the user has made errors - return -1 if an
 * incorrect state is generated; else return 0 for a correct state.
 */
static
int
reapply( void )
{
    int digit, idx, j;
    int allok = 0;
    rb->memset( board, 0x00, sizeof( board ) );
    for( j = 0 ; j < idx_history ; ++j )
        if( !( history[ j ] & IGNORED ) && 0 != GET_DIGIT( history[ j ] ) )
        {
            idx = GET_INDEX( history[ j ] );
            digit = GET_DIGIT( history[ j ] );
            if( !IS_EMPTY( idx ) || DISALLOWED( idx, digit ) )
                allok = -1;
            board[ idx ] = SET_DIGIT( digit );
            if( history[ j ] & FIXED )
                board[ idx ] |= FIXED;
            update( idx );
        }
    return allok;
}

/* Clear moves, leaving fixed squares
 */
static
void
clear_moves( void )
{
    for( idx_history = 0 ; history[ idx_history ] & FIXED ; ++idx_history )
        ;
    reapply( );
}

static int digits[ 9 ];    /* # digits expressed in element square */
static int counts[ 9 ];    /* Count of digits (c.f. count_set_digits()) */

/* Count # set bits (within STATE_MASK) */
static
int
numset( int mask )
{
    int i, n = 0;
    for( i = STATE_SHIFT + 1 ; i <= STATE_SHIFT + 9 ; ++i )
        if( mask & (1<<i) )
            ++n;
        else
            ++counts[ i - STATE_SHIFT - 1 ];
    return n;
}

static
void
count_set_digits( int el, int (*idx_fn)( int, int ) )
{
    int i;
    rb->memset( counts, 0x00, sizeof( counts ) );
    for( i = 0 ; i < 9 ; ++i )
        digits[ i ] = numset( board[ (*idx_fn)( el, i ) ] );
}

/* Fill square with given digit, and update state.
 * Returns 0 on success, else -1 on error (i.e. invalid fill)
 */
static
int
fill( int idx, int digit )
{
    assert( 0 != digit );

    if( !IS_EMPTY( idx ) )
        return ( DIGIT( idx ) == digit ) ? 0 : -1;

    if( DISALLOWED( idx, digit ) )
        return -1;

    board[ idx ] = SET_DIGIT( digit );
    update( idx );
    add_move( idx, digit, 0 );

    return 0;
}

/* Find all squares with a single digit allowed -- do not mutate board */
static
void
singles( int el, int (*idx_fn)( int, int ), int hintcode )
{
    int i, j, idx;

    count_set_digits( el, idx_fn );

    for( i = 0 ; i < 9 ; ++i )
    {
        if( 1 == counts[ i ] )
        {
            /* Digit 'i+1' appears just once in the element */
            for( j = 0 ; j < 9 ; ++j )
            {
                idx = (*idx_fn)( el, j );
                if( !DISALLOWED( idx, i + 1 ) && idx_possible < 81 )
                    possible[ idx_possible++ ] = SET_INDEX( idx )
                                               | SET_DIGIT( i + 1 )
                                               | hintcode;
            }
        }
        if( 8 == digits[ i ] )
        {
            /* 8 digits are masked at this position - just one remaining */
            idx = (*idx_fn)( el, i );
            for( j = 1 ; j <= 9 ; ++j )
                if( 0 == ( STATE( idx ) & DIGIT_STATE( j ) ) && idx_possible < 81 )
                    possible[ idx_possible++ ] = SET_INDEX( idx )
                                               | SET_DIGIT( j )
                                               | hintcode;
        }
    }
}

/* Given the board state, find all possible 'moves' (i.e. squares with just
 * a single digit).
 *
 * Returns the number of (deterministic) moves (and fills the moves array),
 * or 0 if no moves are possible. This function does not mutate the board
 * state, and hence, can return the same move multiple times (with
 * different hints).
 */
static
int
findmoves( void )
{
    int i;

    idx_possible = 0;
    for( i = 0 ; i < 9 ; ++i )
    {
        singles( i, idx_row, HINT_ROW );
        singles( i, idx_column, HINT_COLUMN );
        singles( i, idx_block, HINT_BLOCK );
    }
    return idx_possible;
}

/* Strategies for refining the board state
 *  - 'pairs'     if there are two unfilled squares in a given row/column/
 *                block with the same state, and just two possibilities,
 *                then all other unfilled squares in the row/column/block
 *                CANNOT be either of these digits.
 *  - 'block'     if the unknown squares in a block all appear in the same
 *                row or column, then all unknown squares outside the block
 *                and in the same row/column cannot be any of the unknown
 *                squares in the block.
 *  - 'common'    if all possible locations for a digit in a block appear
 *                in a row or column, then that digit cannot appear outside
 *                the block in the same row or column.
 *  - 'position2' if the positions of 2 unknown digits in a block match
 *                identically in precisely 2 positions, then those 2 positions
 *                can only contain the 2 unknown digits.
 *
 * Recall that each state bit uses a 1 to prevent a digit from
 * filling that square.
 */

static
void
pairs( int el, int (*idx_fn)( int, int ) )
{
    int i, j, k, mask, idx;
    for( i = 0 ; i < 8 ; ++i )
        if( 7 == digits[ i ] ) /* 2 digits unknown */
            for( j = i + 1 ; j < 9 ; ++j )
            {
                idx = (*idx_fn)( el, i );
                if( STATE( idx ) == STATE( (*idx_fn)( el, j ) ) )
                {
                    /* Found a row/column pair - mask other entries */
                    mask = STATE_MASK ^ (STATE_MASK & board[ idx ] );
                    for( k = 0 ; k < i ; ++k )
                        board[ (*idx_fn)( el, k ) ] |= mask;
                    for( k = i + 1 ; k < j ; ++k )
                        board[ (*idx_fn)( el, k ) ] |= mask;
                    for( k = j + 1 ; k < 9 ; ++k )
                        board[ (*idx_fn)( el, k ) ] |= mask;
                    digits[ j ] = -1; /* now processed */
                }
            }
}

/* Worker: mask elements outside block */
static
void
exmask( int mask, int block, int el, int (*idx_fn)( int, int ) )
{
    int i, idx;

    for( i = 0 ; i < 9 ; ++i )
    {
        idx = (*idx_fn)( el, i );
        if( block != BLOCK( idx ) && IS_EMPTY( idx ) )
            board[ idx ] |= mask;
    }
}

/* Worker for block() */
static
void
exblock( int block, int el, int (*idx_fn)( int, int ) )
{
    int i, idx, mask;

    /* By assumption, all unknown squares in the block appear in the
     * same row/column, so to construct a mask for these squares, it
     * is sufficient to invert the mask for the known squares in the
     * block.
     */
    mask = 0;
    for( i = 0 ; i < 9 ; ++i )
    {
        idx = idx_block( block, i );
        if( !IS_EMPTY( idx ) )
            mask |= DIGIT_STATE( DIGIT( idx ) );
    }
    exmask( mask ^ STATE_MASK, block, el, idx_fn );
}

static
void
block( int el )
{
    int i, idx = 0, row, col;

    /* Find first unknown square */
    for( i = 0 ; i < 9 && !IS_EMPTY( idx = idx_block( el, i ) ) ; ++i )
        ;
    if( i < 9 )
    {
        assert( IS_EMPTY( idx ) );
        row = ROW( idx );
        col = COLUMN( idx );
        for( ++i ; i < 9 ; ++i )
        {
            idx = idx_block( el, i );
            if( IS_EMPTY( idx ) )
            {
                if( ROW( idx ) != row )
                    row = -1;
                if( COLUMN( idx ) != col )
                    col = -1;
            }
        }
        if( 0 <= row )
            exblock( el, row, idx_row );
        if( 0 <= col )
            exblock( el, col, idx_column );
    }
}

static
void
common( int el )
{
    int i, idx, row, col, digit, mask;

    for( digit = 1 ; digit <= 9 ; ++digit )
    {
        mask = DIGIT_STATE( digit );
        row = col = -1;  /* Value '9' indicates invalid */
        for( i = 0 ; i < 9 ; ++i )
        {
            /* Digit possible? */
            idx = idx_block( el, i );
            if( IS_EMPTY( idx ) && 0 == ( board[ idx ] & mask ) )
            {
                if( row < 0 )
                    row = ROW( idx );
                else
                if( row != ROW( idx ) )
                    row = 9; /* Digit appears in multiple rows */
                if( col < 0 )
                    col = COLUMN( idx );
                else
                if( col != COLUMN( idx ) )
                    col = 9; /* Digit appears in multiple columns */
            }
        }
        if( -1 != row && row < 9 )
            exmask( mask, el, row, idx_row );
        if( -1 != col && col < 9 )
            exmask( mask, el, col, idx_column );
    }
}

/* Encoding of positions of a digit (c.f. position2()) - abuse DIGIT_STATE */
static int posn_digit[ 10 ];

static
void
position2( int el )
{
    int digit, digit2, i, mask, mask2, posn, count, idx;

    /* Calculate positions of each digit within block */
    for( digit = 1 ; digit <= 9 ; ++digit )
    {
        mask = DIGIT_STATE( digit );
        posn_digit[ digit ] = count = posn = 0;
        for( i = 0 ; i < 9 ; ++i )
            if( 0 == ( mask & board[ idx_block( el, i ) ] ) )
            {
                ++count;
                posn |= DIGIT_STATE( i );
            }
        if( 2 == count )
            posn_digit[ digit ] = posn;
    }
    /* Find pairs of matching positions, and mask */
    for( digit = 1 ; digit < 9 ; ++digit )
        if( 0 != posn_digit[ digit ] )
            for( digit2 = digit + 1 ; digit2 <= 9 ; ++digit2 )
                if( posn_digit[ digit ] == posn_digit[ digit2 ] )
                {
                    mask = STATE_MASK
                           ^ ( DIGIT_STATE( digit ) | DIGIT_STATE( digit2 ) );
                    mask2 = DIGIT_STATE( digit );
                    for( i = 0 ; i < 9 ; ++i )
                    {
                        idx = idx_block( el, i );
                        if( 0 == ( mask2 & board[ idx ] ) )
                        {
                            assert( 0 == (DIGIT_STATE(digit2) & board[idx]) );
                            board[ idx ] |= mask;
                        }
                    }
                    posn_digit[ digit ] = posn_digit[ digit2 ] = 0;
                    break;
                }
}

/* Find some moves for the board; starts with a simple approach (finding
 * singles), and if no moves found, starts using more involved strategies
 * until a move is found. The more advanced strategies can mask states
 * in the board, making this an efficient mechanism, but difficult for
 * a human to understand.
 */
static
int
allmoves( void )
{
    int i, n;

    n = findmoves( );
    if( 0 < n )
        return n;

    for( i = 0 ; i < 9 ; ++i )
    {
        count_set_digits( i, idx_row );
        pairs( i, idx_row );

        count_set_digits( i, idx_column );
        pairs( i, idx_column );

        count_set_digits( i, idx_block );
        pairs( i, idx_block );
    }
    n = findmoves( );
    if( 0 < n )
        return n;

    for( i = 0 ; i < 9 ; ++i )
    {
        block( i );
        common( i );
        position2( i );
    }
    return findmoves( );
}

/* Helper: sort based on index */
static
int
cmpindex( const void * a, const void * b )
{
    return GET_INDEX( *((const int *)b) ) - GET_INDEX( *((const int *)a) );
}

/* Return number of hints. The hints mechanism should attempt to find
 * 'easy' moves first, and if none are possible, then try for more
 * cryptic moves.
 */
int
findhints( void )
{
    int i, n, mutated = 0;

    n = findmoves( );
    if( n < 2 )
    {
        /* Each call to pairs() can mutate the board state, making the
         * hints very, very cryptic... so later undo the mutations.
         */
        for( i = 0 ; i < 9 ; ++i )
        {
            count_set_digits( i, idx_row );
            pairs( i, idx_row );

            count_set_digits( i, idx_column );
            pairs( i, idx_column );

            count_set_digits( i, idx_block );
            pairs( i, idx_block );
        }
        mutated = 1;
        n = findmoves( );
    }
    if( n < 2 )
    {
        for( i = 0 ; i < 9 ; ++i )
        {
            block( i );
            common( i );
        }
        mutated = 1;
        n = findmoves( );
    }

    /* Sort the possible moves, and allow just one hint per square */
    if( 0 < n )
    {
        int i, j;

        rb->qsort( possible, n, sizeof( int ), cmpindex );
        for( i = 0, j = 1 ; j < n ; ++j )
        {
            if( GET_INDEX( possible[ i ] ) == GET_INDEX( possible[ j ] ) )
            {
                /* Let the user make mistakes - do not assume the
                 * board is in a consistent state.
                 */
                if( GET_DIGIT( possible[i] ) == GET_DIGIT( possible[j] ) )
                    possible[ i ] |= possible[ j ];
            }
            else
                i = j;
        }
        n = i + 1;
    }

    /* Undo any mutations of the board state */
    if( mutated )
        reapply( );

    return n;
}

/* Deterministic solver; return 0 on success, else -1 on error.
 */
static
int
deterministic( void )
{
    int i, n;

    n = allmoves( );
    while( 0 < n )
    {
        ++pass;
        for( i = 0 ; i < n ; ++i )
            if( -1 == fill( GET_INDEX( possible[ i ] ),
                            GET_DIGIT( possible[ i ] ) ) )
                return -1;
        n = allmoves( );
    }
    return 0;
}

/* Return index of square for choice.
 *
 * If no choice is possible (i.e. board solved or inconsistent),
 * return -1.
 *
 * The current implementation finds a square with the minimum
 * number of unknown digits (i.e. maximum # masked digits).
 */
static
int
cmp( const void * e1, const void * e2 )
{
    return GET_DIGIT( *(const int *)e2 ) - GET_DIGIT( *(const int *)e1 );
}

static
int
choice( void )
{
    int i, n;
    for( n = i = 0 ; i < 81 ; ++i )
        if( IS_EMPTY( i ) )
        {
            possible[ n ] = SET_INDEX( i ) | SET_DIGIT( numset( board[ i ] ) );

            /* Inconsistency if square unknown, but nothing possible */
            if( 9 == GET_DIGIT( possible[ n ] ) )
                return -2;
            ++n;
        }

    if( 0 == n )
        return -1;      /* All squares known */

    rb->qsort( possible, n, sizeof( possible[ 0 ] ), cmp );
    return GET_INDEX( possible[ 0 ] );
}

/* Choose a digit for the given square.
 * The starting digit is passed as a parameter.
 * Returns -1 if no choice possible.
 */
static
int
choose( int idx, int digit )
{
    for( ; digit <= 9 ; ++digit )
        if( !DISALLOWED( idx, digit ) )
        {
            board[ idx ] = SET_DIGIT( digit );
            update( idx );
            add_move( idx, digit, CHOICE );
            return digit;
        }

    return -1;
}

/* Backtrack to a previous choice point, and attempt to reseed
 * the search. Return -1 if no further choice possible, or
 * the index of the changed square.
 *
 * Assumes that the move history and board are valid.
 */
static
int
backtrack( void )
{
    int digit, idx;

    for( ; 0 <= --idx_history ; )
        if( history[ idx_history ] & CHOICE )
        {
            /* Remember the last choice, and advance */
            idx = GET_INDEX( history[ idx_history ] );
            digit = GET_DIGIT( history[ idx_history ] ) + 1;
            reapply( );
            if( -1 != choose( idx, digit ) )
                return idx;
        }

    return -1;
}

/* Attempt to solve 'board'; return 0 on success else -1 on error.
 *
 * The solution process attempts to fill-in deterministically as
 * much of the board as possible. Once that is no longer possible,
 * need to choose a square to fill in.
 */
static
int
solve( void )
{
    int idx;

    while( 1 )
    {
        if( 0 == deterministic( ) )
        {
            /* Solved, make a new choice, or rewind a previous choice */
            idx = choice( );
            if( -1 == idx )
                return 0;
            else
            if( ( idx < 0 || -1 == choose( idx, 1 ) ) && -1 == backtrack( ) )
                return -1;
        }
        else /* rewind to a previous choice */
        if( -1 == backtrack( ) )
            return -1;
    }
    return -1;
}

static
int
init_template( int template )
{
    int i, row, col;
    int mask;

    reset( );
    len_tmplt = 0;

    /* Consume grid - allow leading spaces and comments at end */
    for( row = 0 ; row < 9 ; ++row )
    {
        mask=0x100;
        for( col = 0 ; col < 9 ; ++col )
        {
            if (templates[template][row] & mask)
                    tmplt[ len_tmplt++ ] = INDEX( row, col );
            mask /= 2;
        }
    }

    /* Construct move history for a template */
    idx_history = 0;
    for( i = 0 ; i < 81 ; ++i )
        if( 0 != DIGIT( i ) )
            history[ idx_history++ ] = i | (DIGIT( i )<<8);

    /* Finally, markup all of these moves as 'fixed' */
    for( i = 0 ; i < idx_history ; ++i )
        history[ i ] |= FIXED;

    return 0;
}

/* Classify a SuDoKu, given its solution.
 *
 * The classification is based on the average number of possible moves
 * for each pass of the deterministic solver - it is a rather simplistic
 * measure, but gives reasonable results. Note also that the classification
 * is based on the first solution found (but does handle the pathological
 * case of multiple solutions). Note that the average moves per pass
 * depends just on the number of squares initially set... this simplifies
 * the statistics collection immensely, requiring just the number of passes
 * to be counted.
 *
 * Return 0 on error, else a string classification.
 */

static
char *
classify( void )
{
    int i, score;

    pass = 0;
    clear_moves( );
    if( -1 == solve( ) )
        return 0;

    score = 81;
    for( i = 0 ; i < 81 ; ++i )
        if( IS_FIXED( i ) )
            --score;

    assert( 81 == idx_history );

    for( i = 0 ; i < 81 ; ++i )
        if( history[ i ] & CHOICE )
            score -= 5;

    if( 15 * pass < score )
        return "very easy";
    else
    if( 11 * pass < score )
        return "easy";
    else
    if( 7 * pass < score )
        return "medium";
    else
    if( 4 * pass < score )
        return "hard";
    else
        return "fiendish";
}

/* exchange disjoint, identical length blocks of data */
static
void
exchange( int * a, int * b, int len )
{
    int i, tmp;
    for( i = 0 ; i < len ; ++i )
    {
        tmp = a[ i ];
        a[ i ] = b[ i ];
        b[ i ] = tmp;
    }
}

/* rotate left */
static
void
rotate1_left( int * a, int len )
{
    int i, tmp;
    tmp = a[ 0 ];
    for( i = 1 ; i < len ; ++i )
        a[ i - 1 ] = a[ i ];
    a[ len - 1 ] = tmp;
}

/* rotate right */
static
void
rotate1_right( int * a, int len )
{
    int i, tmp;
    tmp = a[ len - 1 ];
    for( i = len - 1 ; 0 < i ; --i )
        a[ i ] = a[ i - 1 ];
    a[ 0 ] = tmp;
}

/* Generalised left rotation - there is a naturally recursive
 * solution that is best implementation using iteration.
 * Note that it is not necessary to do repeated unit rotations.
 *
 * This function is analogous to 'cutting' a 'pack of cards'.
 *
 * On entry: 0 < idx < len
 */
static
void
rotate( int * a, int len, int idx )
{
    int xdi = len - idx;
    int delta = idx - xdi;

    while( 0 != delta && 0 != idx )
    {
        if( delta < 0 )
        {
            if( 1 == idx )
            {
                rotate1_left( a, len );
                idx = 0;
            }
            else
            {
                exchange( a, a + xdi, idx );
                len = xdi;
            }
        }
        else /* 0 < delta */
        {
            if( 1 == xdi )
            {
                rotate1_right( a, len );
                idx = 0;
            }
            else
            {
                exchange( a, a + idx, xdi );
                a += xdi;
                len = idx;
                idx -= xdi;
            }
        }
        xdi = len - idx;
        delta = idx - xdi;
    }
    if( 0 < idx )
        exchange( a, a + idx, idx );
}

/* Shuffle an array of integers */
static
void
shuffle( int * a, int len )
{
    int i, j, tmp;

    i = len;
    while( 1 <= i )
    {
        j = rb->rand( ) % i;
        tmp = a[ --i ];
        a[ i ] = a[ j ];
        a[ j ] = tmp;
    }
}

/* Generate a SuDoKu puzzle
 *
 * The generation process selects a random template, and then attempts
 * to fill in the exposed squares to generate a board. The order of the
 * digits and of filling in the exposed squares are random.
 */

/* Select random template; sets tmplt, len_tmplt */
static
void
select_template( void )
{
    int i = rb->rand( ) % NUM_TEMPLATES;
    init_template( i );
}


static
void
generate( void )
{
    static int digits[ 9 ];

    int i;

start:
    for( i = 0 ; i < 9 ; ++i )
        digits[ i ] = i + 1;

    rotate( digits, 9, 1 + rb->rand( ) % 8 );
    shuffle( digits, 9 );
    select_template( );

    rotate( tmplt, len_tmplt, 1 + rb->rand( ) % ( len_tmplt - 1 ) );
    shuffle( tmplt, len_tmplt );

    reset( );  /* construct a new board */

    for( i = 0 ; i < len_tmplt ; ++i )
        fill( tmplt[ i ], digits[ i % 9 ] );

    if( 0 != solve( ) || idx_history < 81 )
        goto start;

    for( i = 0 ; i < len_tmplt ; ++i )
        board[ tmplt[ i ] ] |= FIXED;

    /* Construct fixed squares */
    for( idx_history = i = 0 ; i < 81 ; ++i )
        if( IS_FIXED( i ) )
            history[ idx_history++ ] = SET_INDEX( i )
                                     | SET_DIGIT( DIGIT( i ) )
                                     | FIXED;
    clear_moves( );

    if( 0 != solve( ) || idx_history < 81 )
        goto start;
    if( -1 != backtrack( ) && 0 == solve( ) )
        goto start;

    clear_moves( );
}

bool sudoku_generate_board(struct sudoku_state_t* state, char** difficulty)
{
    int r,c,i;

    rb->srand(*rb->current_tick);

    generate();

    i=0;
    for (r=0;r<9;r++) {
        for (c=0;c<9;c++) {
            if( IS_EMPTY( i ) )
                state->startboard[r][c]='0';
            else
                state->startboard[r][c]='0'+GET_DIGIT( board[ i ] );

            state->currentboard[r][c]=state->startboard[r][c];
            i++;
        }
    }

    *difficulty = classify( );
    return true;
}

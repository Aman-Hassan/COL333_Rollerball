## Tasks

- Material checking update
- Distance checking
- Range of movement (simple)
- Re-assess weights

## Bugs

- Undo move seems to undo the move but not undo the killing of a piece, and hence messing up the minimax search.

## Evaluation function

Ideas:

- Material checking (update for pawn upgrade)
- Checkmate (done)
- Distance checking
- Search evals
  (i.e. requires get_legal_moves() which is the computation of one extra depth of search)
  - Threat range (already sort of handled by material checking)
  - Range of movement (basically branching factor number)

### Material checking

- Replace array with if statements for each piece and their piece id.

### Distance checking

#### For rooks:

- Calculate distance from king.
- should prioritise only moving to a ring sector, rather than physically closer to king

#### For pawns:

- incremental distance is better.

#### For bishop:

- not sure

### Adding temporal logic

- (if we get time)
- Will require a new data structure which gets updated with every move, amortising any high time complexities.

## Extras

- Implement single function Minimax function like in the YT video. Will make the code more readable.

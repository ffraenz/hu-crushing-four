
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// *****************
// *   CONSTANTS   *
// *****************

// Initial and minimum column array size
#define MIN_COL_SIZE 8

// Initial playground changes array size
#define INITIAL_CHANGES_SIZE 80

// Initial piece removal array size
#define INITIAL_REMOVAL_SIZE 80

// Empty piece value
#define PIECE_EMPTY 255

// Min number of pieces required to form a line
#define MIN_LINE_COUNT 4

// ********************
// *   HEADER TYPES   *
// ********************

// Value range: [0, 254], 255 = cleared
typedef unsigned short piece;

// Col types
typedef enum { COL_PIECES, COL_PADDING } colType;

// Col data structure
struct Col {
  // Col type
  colType type;

  // Depending on the type:
  // - DEFAULT: Number of pieces stacked on top of each other in this col
  // - PADDING: Number of padding cols between the previous and the next col
  unsigned long size;

  // Number of pieces that are part of the array
  unsigned long count;

  // Piece index from which changes happend. Equal to size if there is no change
  unsigned long changeY;

  // Pointer to next col
  struct Col* next;

  // Pointer to previous col
  struct Col* prev;

  // Column pieces
  piece pieces[];
};

// Data structure describing a single piece removal from a referenced column
struct PieceRemoval {
  // Pointer to the col a piece should be removed from
  struct Col *col;
  
  // Piece Y-position inside the reference col
  unsigned long y;
};

// Playground data structure (doubly linked list of cols)
struct Playground {
  // Pointer to col at the lower extreme
  struct Col* startCol;

  // Position of col at the lower extreme
  long startColX;

  // Pointer to origin col at x = 0
  struct Col* originCol;

  // Pointer to col at the upper extreme
  struct Col* endCol;

  // Position of col at the lower extreme
  long endColX;

  // Pointer to the iterator col
  struct Col* currentCol;

  // Position of the iterator col
  long currentX;

  // Array of changed cols
  struct Col** changedCols;
  unsigned long changedColsCount;
  unsigned long changedColsSize;
  
  // Removals
  struct PieceRemoval** pieceRemovals;
  unsigned long pieceRemovalsCount;
  unsigned long pieceRemovalsSize;
};

// ************************
// *   HEADER FUNCTIONS   *
// ************************

struct Playground* createPlayground(void);
void freePlayground(struct Playground* playground);
struct Col* createCol(void);
struct Col* createPaddingCol(unsigned long size);
struct Col* playgroundGetColAt(struct Playground* playground, long x);
void playgroundPlacePiece(struct Playground* playground, long x, piece p);
void playgroundRemoveLines(struct Playground* playground);
void playgroundRemovePiece(struct Playground* playground, struct Col* col, unsigned long y);
void playgroundTrackChange(struct Playground* playground, struct Col* col, unsigned long y);
void playgroundCauseGravity(struct Playground* playground);
void playgroundPrint(struct Playground* playground);
void handleOutOfMemory(char description[]);

// ************
// *   BODY   *
// ************

/**
 * Global debug flag
 */
bool debug = false;

/**
 * Global playground instance
 */
struct Playground* playground;

/**
 * Main entry point
 * @return Exit code
 */
int main(int argc, char *argv[]) {
  // Create empty playground
  playground = createPlayground();
  
  // Debug mode: Run specific test case if first argument is set
  if (argc == 2) {
    debug = true;
    freopen(argv[1], "r", stdin);
  }

  // Read placements from stdin
  // TODO: Use getline(3) instead
  long x = 0;
  piece p = 0;

  // Place each piece on the playground
  // "<color> <x>" color in [0; 254]; x in [-2^21; +2^21]
  while (fscanf(stdin, "%hu%ld", &p, &x) == 2) {
    if (debug) {
      printf("Place piece %3hd at %ld\n", p, x);
    }
    
    playgroundPlacePiece(playground, x, p);
    
    if (debug) {
      playgroundPrint(playground);
    }
  }

  // Print playground to stout
  playgroundPrint(playground);

  // Dealloc used memory before quitting
  freePlayground(playground);
  return 0;
}

/**
 * Handles the event of running out of memory.
 * @param description Task at which the program ran out of memory
 */
void handleOutOfMemory(char description[]) {
  printf("Not enough memory left %s\n", description);
  freePlayground(playground);
  playground = NULL;
  exit(1);
}

/**
 * Create empty playground (containing a single origin node).
 * Assumptions:
 * - Origin, start and end cols must not be of type padding
 * - Two adjancent padding cols are not allowed
 * @return Pointer to playground
 */
struct Playground* createPlayground() {
  struct Playground* playground = (struct Playground*) malloc(
    sizeof(struct Playground));
  if (!playground) {
    handleOutOfMemory("creating a playground");
  }

  struct Col* col = createCol();
  playground->originCol = col;
  playground->startCol = col;
  playground->startColX = 0;
  playground->endCol = col;
  playground->endColX = 0;
  playground->currentCol = col;
  playground->currentX = 0;
  
  playground->changedColsSize = INITIAL_CHANGES_SIZE;
  playground->changedColsCount = 0;
  playground->changedCols = (struct Col**)
    malloc(playground->changedColsSize * sizeof(struct Col*));
  if (!playground->changedCols) {
    handleOutOfMemory("creating a playground");
  }
  
  playground->pieceRemovalsSize = INITIAL_REMOVAL_SIZE;
  playground->pieceRemovalsCount = 0;
  playground->pieceRemovals = (struct PieceRemoval**)
    malloc(playground->pieceRemovalsSize * sizeof(struct PieceRemoval*));
  if (!playground->pieceRemovals) {
    handleOutOfMemory("creating a playground");
  }
  
  return playground;
}

/**
 * Frees a playground instance with all of its cols.
 * @param playground Playground instance to free
 */
void freePlayground(struct Playground* playground) {
  if (playground) {
    // Free cols in playground
    struct Col* col = playground->startCol;
    struct Col* next;
    while (col) {
      next = col->next;
      free(col);
      col = next;
    }
    
    // Free piece removal and change arrays
    free(playground->changedCols);
    free(playground->pieceRemovals);

    // Free playground itself
    free(playground);
  }
}

/**
 * Create a col node with the default initial size.
 * @return Pointer to new col node
 */
struct Col* createCol() {
  struct Col* col = (struct Col*)
    malloc(sizeof(struct Col) + sizeof(piece) * MIN_COL_SIZE);
  if (!col) {
    handleOutOfMemory("creating a column with size %lu");
  }
  col->type = COL_PIECES;
  col->size = MIN_COL_SIZE;
  col->count = 0;
  col->changeY = MIN_COL_SIZE;
  col->next = NULL;
  col->prev = NULL;
  return col;
}

/**
 * Create a padding col node with the given size.
 * @param size Number of padding cols
 * @return Pointer to new col node
 */
struct Col* createPaddingCol(unsigned long size) {
  struct Col* col = (struct Col*) malloc(sizeof(struct Col));
  if (!col) {
    handleOutOfMemory("creating a padding column");
  }
  col->type = COL_PADDING;
  col->size = size;
  col->next = NULL;
  col->prev = NULL;
  return col;
}

/**
 * Create a piece removal for the given column and y-position.
 * @param col Pointer to col
 * @param y Piece y-position
 * @return Pointer to new piece removal
 */
struct PieceRemoval* createPieceRemoval(struct Col* col, unsigned long y) {
  struct PieceRemoval* pieceRemoval = (struct PieceRemoval*)
    malloc(sizeof(struct PieceRemoval));
  if (!pieceRemoval) {
    handleOutOfMemory("creating a piece removal");
  }
  pieceRemoval->col = col;
  pieceRemoval->y = y;
  return pieceRemoval;
}

/**
 * Finds the Col at the given x. Lazily creates a col instance if not done, yet.
 * Lazily creates padding cols if necessary, but they are never returned.
 * @param playground Playground instance
 * @param x Col position
 * @return Pointer to Col struct
 */
struct Col* playgroundGetColAt(struct Playground* playground, long x) {
  struct Col* col;
  
  // TODO: Make sure col list stays intact between creation calls that may cause out of memory errors

  // Test basic search cases (cheap, in O(1))
  if (x == 0) {
    // Origin col requested
    return playground->originCol;

  } else if (x == playground->endColX) {
    // End col requested
    return playground->endCol;

  } else if (x > playground->endColX) {
    // Requested col exceeds end col

    // Append new padding col, if necessary
    if (playground->endColX + 1 < x) {
      col = createPaddingCol(x - (long) playground->endColX - 1);
      playground->endCol->next = col;
      col->prev = playground->endCol;
      playground->endCol = col;
    }

    // Append new col
    col = createCol();
    playground->endCol->next = col;
    col->prev = playground->endCol;
    playground->endCol = col;
    playground->endColX = x;

    return col;

  } else if (x == playground->startColX) {
    // Start col requested
    return playground->startCol;

  } else if (x < playground->startColX) {
    // Requested col exceeds start col

    // Prepend new padding col, if necessary
    if (playground->startColX - 1 > x) {
      col = createPaddingCol((long) playground->startColX - x - 1);
      playground->startCol->prev = col;
      col->next = playground->startCol;
      playground->startCol = col;
    }

    // Prepend new col
    col = createCol();
    playground->startCol->prev = col;
    col->next = playground->startCol;
    playground->startCol = col;
    playground->startColX = x;

    return col;
  }

  // The easiest cases of finding a col were tested above
  // Now we need to iterate to it (expensive)
  col = playground->currentCol;
  long i = playground->currentX;
  struct Col* newCol;
  
  // Move iterator until reaching the desired position or the end
  if (i <= x) {
    // Move iterator forward
    while (i < x) {
      i += col->type == COL_PADDING ? col->size : 1;
      col = col->next;
    }
  } else {
    // Move iterator backward
    while (i > x) {
      col = col->prev;
      i -= col->type == COL_PADDING ? col->size : 1;
    }
    if (i < x) {
      // TODO: Security check. To be removed in production.
      if (col->type != COL_PADDING) {
        printf("This should not have happened.\n");
        exit(1);
      }
      
      // Ran past the x col, col must be of type padding (with size > 1).
      // Move iterator behind that padding col resulting in the same situation
      // as when moving the iterator forward.
      i += col->size;
      col = col->next;
    }
  }

  if (i > x) {
    // Ran past the x col, col->prev must be of type padding (with size > 1)
    struct Col* lowerPadding = col->prev;

    // Reduce lower padding to insert new col after it
    lowerPadding->size -= i - x;

    // Append new col after lower padding
    newCol = createCol();
    newCol->prev = lowerPadding;
    lowerPadding->next = newCol;

    // Append optional upper padding, if necessary
    if (i - x > 1) {
      newCol = createPaddingCol(i - x - 1);
      newCol->prev = lowerPadding->next;
      newCol->prev->next = newCol;
    }

    // Connect new col(s) to the current col
    col->prev = newCol;
    newCol->next = col;
    col = lowerPadding->next;
  }

  if (col->type == COL_PADDING) {
    // Create new col and connect it to the lower neighbour
    newCol = createCol();
    newCol->prev = col->prev;
    col->prev->next = newCol;

    if (col->size == 1) {
      // Connect new col to the right neighbour, replacing the padding col
      newCol->next = col->next;
      newCol->next->prev = newCol;

      // Let go padding col
      free(col);
      col = newCol;
    } else {
      // Shrink down padding size by 1 and insert new col before it
      col->size--;
      newCol->next = col;
      col->prev = newCol;
      col = newCol;
    }
  }

  // Update iterator
  playground->currentCol = col;
  playground->currentX = x;
  return col;
}

/**
 * Insert a piece at the given x-position.
 * @param playground Playground
 * @param x Position to insert the piece
 * @param p Piece color to be inserted
 */
void playgroundPlacePiece(struct Playground* playground, long x, piece p) {
  struct Col* col = playgroundGetColAt(playground, x);

  if (col->count == col->size) {
    // Double the column size by reallocating memory
    unsigned long size = col->size * 2;
    struct Col* expandedCol = (struct Col*)
      realloc(col, sizeof(struct Col) + sizeof(piece) * size);
    if (!expandedCol) {
      handleOutOfMemory("expanding column size");
    }
    
    // Update size and state
    if (expandedCol->changeY == expandedCol->size) {
      expandedCol->changeY = size;
    }
    expandedCol->size = size;
    
    // Update pointers
    if (expandedCol->next) {
      expandedCol->next->prev = expandedCol;
    } else {
      playground->endCol = expandedCol;
    }
    if (expandedCol->prev) {
      expandedCol->prev->next = expandedCol;
    } else {
      playground->startCol = expandedCol;
    }
    if (col == playground->originCol) {
      playground->originCol = expandedCol;
    }
    if (col == playground->currentCol) {
      playground->currentCol = expandedCol;
    }
    col = expandedCol;
  }

  // Append piece to the top
  col->pieces[col->count] = p;
  playgroundTrackChange(playground, col, col->count);
  col->count++;

  // Scan for lines, cause gravity and remove the process until no more
  // lines are being identified
  playgroundRemoveLines(playground);
  while (playground->pieceRemovalsCount > 0) {
    playgroundCauseGravity(playground);
    playgroundRemoveLines(playground);
  }

  // TODO: Downgrade, free or merge empty cols

  // Clear changes
  for (int i = 0; i < playground->changedColsCount; i++) {
    playground->changedCols[i]->changeY = playground->changedCols[i]->size;
  }
  playground->changedColsCount = 0;
}

/**
 * Identify horizontal (–), vertical (|), diagonal (/, \) lines and mark pieces
 * on those lines as empty while tracking changes.
 * @param playground Playground
 */
void playgroundRemoveLines(struct Playground* playground) {
  int j;
  long y;
  long nextY;
  unsigned long lineLength;
  signed short delY;

  piece currentPiece;
  piece lineColor;

  struct Col* col;
  struct Col* nextCol;
  struct Col* lineEndCol;
  struct Col* lineStartCol;

  // Only consider cols where changes were applied
  for (int i = 0; i < playground->changedColsCount; i++) {
    col = playground->changedCols[i];

    // For each y above changeY identify crossing horizontal and diagonal lines
    for (y = col->changeY; y < col->count; y++) {
      currentPiece = col->pieces[y];
      // TODO: Optimize: No need to search from pieces that are marked as removed

      // Iterate through directions falling diagonal (-1), horizontal (0) and
      // climbing diagonal (1)
      for (delY = -1; delY <= 1; delY++) {
        // Upper col the current line is ending
        lineEndCol = col;

        // Next col piece Y-index
        nextCol = col->next;
        nextY = y + delY;

        // Line piece count
        lineLength = 1;
        
        // Move forward while there is a next col, the next col is not a padding
        // col, the next col is high enough and the next piece in it is of
        // the current color or empty
        while (
          // Next column availability
          nextCol &&
          nextCol->type == COL_PIECES &&
          // Bounds of y-position
          nextY >= 0 &&
          nextY < nextCol->count &&
          // Check piece color
          nextCol->pieces[nextY] == currentPiece
        ) {
          lineEndCol = nextCol;
          nextCol = lineEndCol->next;
          nextY += delY;
          lineLength++;
        }

        // Do the same moving backward
        lineStartCol = col;
        nextCol = col->prev;
        nextY = y - delY;

        while (
          // Next column availability
          nextCol &&
          nextCol->type == COL_PIECES &&
          // Bounds of y-position
          nextY >= 0 &&
          nextY < nextCol->count &&
          // Check piece color
          nextCol->pieces[nextY] == currentPiece
        ) {
          lineStartCol = nextCol;
          nextCol = lineStartCol->prev;
          nextY -= delY;
          lineLength++;
        }

        if (lineLength >= MIN_LINE_COUNT) {
          // We identified a horizontal or diagonal line (based on nextY)
          // Iterate over line cols and remove each piece
          nextY += delY;
          nextCol = lineStartCol;
          while (nextCol && nextCol->prev != lineEndCol) {
            playgroundRemovePiece(playground, nextCol, nextY);
            nextCol = nextCol->next;
            nextY += delY;
          }
        }
      }
    }

    // Remove all vertical lines above change mark in linear time
    lineColor = PIECE_EMPTY;
    lineLength = 0;

    for (y = col->count - 1; y >= 0; y--) {
      currentPiece = col->pieces[y];
      if (currentPiece == lineColor) {
        // Add piece to line
        lineLength++;
        if (lineLength == MIN_LINE_COUNT) {
          // Min line count fulfilled, remove pieces
          for (j = 0; j < MIN_LINE_COUNT; j++) {
            playgroundRemovePiece(playground, col, y + j);
          }
        } else if (lineLength > MIN_LINE_COUNT) {
          // Remove this next line piece
          playgroundRemovePiece(playground, col, y);
        }
      } else if (y >= col->changeY) {
        // Reset line
        lineColor = currentPiece;
        lineLength = 1;
      } else {
        // Below the change mark we can stop searching for new lines
        break;
      }
    }
  }
}

/**
 * Mark piece at the given Y-position inside a col as to be removed.
 * It will definetly be removed in the gravity step of the loop.
 * @param playground Playground instance
 * @param col Col instance to remove piece from
 * @param y Y-position of piece to be removed
 */
void playgroundRemovePiece(struct Playground* playground, struct Col* col, unsigned long y) {
  if (playground->pieceRemovalsCount == playground->changedColsSize) {
    // TODO: Expand dynamic array if necessary
    printf("Piece removall array needs resizing\n");
    exit(1);
  }
  
  playground->pieceRemovals[playground->pieceRemovalsCount++] = createPieceRemoval(col, y);
  playgroundTrackChange(playground, col, y);
}

/**
 * Track a col change at the given Y-position.
 * @param playground Playground instance
 * @param col Col instance that changed
 * @param y Piece Y-position
 */
void playgroundTrackChange(struct Playground* playground, struct Col* col, unsigned long y) {
  // Mark col pieces above y as changed
  if (col->changeY == col->size || col->changeY > y) {
    col->changeY = y;

    // TODO: Dynamically increase change array size if necessary
    if (col->changeY != col->size) {
      playground->changedCols[playground->changedColsCount++] = col;
    }
  }
}

/**
 * Let pieces fall down onto pieces marked as being empty.
 * Remove empty pieces from col count.
 * @param playground Playground
 */
void playgroundCauseGravity(struct Playground* playground) {
  unsigned long i;
  struct Col* col;
  struct PieceRemoval* pieceRemoval;
  
  // Remove pieces
  for (i = 0; i < playground->pieceRemovalsCount; i++) {
    pieceRemoval = playground->pieceRemovals[i];
    pieceRemoval->col->pieces[pieceRemoval->y] = PIECE_EMPTY;
  }
  
  // Clear piece removals array
  playground->pieceRemovalsCount = 0;
  
  // Only consider cols where changes were applied
  for (i = 0; i < playground->changedColsCount; i++) {
    col = playground->changedCols[i];

    // Cause gravity on a single column
    unsigned long removedPieces = 0;
    for (unsigned long y = col->changeY; y < col->count; y++) {
      if (col->pieces[y] == PIECE_EMPTY) {
        removedPieces++;
      } else {
        col->pieces[y - removedPieces] = col->pieces[y];
      }
    }

    // Update col piece count / height
    col->count -= removedPieces;
  }
}

/**
 * Print playground
 * @param playground Pointer to playground struct to be printed
 */
void playgroundPrint(struct Playground* playground) {
  // Iterate from the start col
  struct Col* col = playground->startCol;
  long x = playground->startColX;

  if (debug) {
    printf("Playground: [%ld; %ld]", playground->startColX, playground->endColX);
  }

  while (col) {
    if (col->type == COL_PIECES) {
      if (debug) {
        printf("\n[%8ld] col %2lu/%2lu |", x, col->count, col->size);
      }

      // Print column pieces
      for (long unsigned j = 0; j < col->count; j++) {
        if (debug) {
          printf("%3hu|", col->pieces[j]);
        } else {
          printf("%d %ld %lu\n", col->pieces[j], x, j);
        }
      }
      // Iterate to the next col
      x++;
      col = col->next;
    } else {
      if (debug) {
        printf("\n[%8ld] --- %lu cols ---", x, col->size);
      }

      // Iterate to the next col
      x = x + col->size;
      col = col->next;
    }
  }

  if (debug) {
    printf("\n\n");
  }
}

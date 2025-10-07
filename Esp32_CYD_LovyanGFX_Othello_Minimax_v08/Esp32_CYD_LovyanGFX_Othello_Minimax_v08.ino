/**
 * Othello.ino - Othello/Reversi game implementation using Minimax library
 * 
 * This sketch implements an Othello/Reversi game that can be played:
 * - Human vs. AI
 * - AI vs. AI (self-play)
 * 
 * The game interface uses Serial communication for display and input.
 * Board visualization uses emoji symbols for better visual experience.
 * 
 * March 4, 2025 ++tmw
 * Original Game Source: https://github.com/ripred/Minimax
 * Example: Othello  
 */

/**
 * This is an advanced sketch as it uses the touch surface of the Cheap Yellow Display (CYD)
 * device. 
 * The graphics are displayed with LovyanGFX library that supports Touch on the
 * CYD.
 * Created by AndroidCrypto October 2025.
 */

/**
 * ### Please change the ESP32-CYD display type driver accordingly to your display type in          ###
 * ### 'Display_Logic.h' (ILI9341 or ST7789) and the difficulty below in '#define MINIMAX_DEPTH 4'. ###
 */

/*
  Version Management
  03.10.2025 V08 Tutorial version, code cleaning
  03.10.2025 V07 Finetuning on another way.
  03.10.2025 V06 Finetuning on the UI, but due to changing the position of '#include "Display_Logic.h"'
                 The game is not working any longer  
  02.10.2025 V05 Shorter starting page without managing the difficulty level
  02.10.2025 V04 Starter page for settings. Unfortunately it is not possible
                 to change the difficulty on UI (runtime)
  02.10.2025 V03 Game is playable now, missing start and difficulty level setting
  01.10.2025 V02 The board is displayed, but the touch control is not working
  01.10.2025 V01 Initial programming
*/

#include "Minimax.h"  // https://github.com/ripred/Minimax

//------------------------------------------------------------------------------------------
// Declarations for the display

const uint8_t DIFFICULTY = 4; // this needs to be the same value as 'MINIMAX_DEPTH' below

#include "Display_Logic.h"

//------------------------------------------------------------------------------------------
// Game

// Constants for board representation
#define EMPTY 0
#define BLACK 1  // First player (human in Human vs. AI mode)
#define WHITE 2  // Second player (AI in Human vs. AI mode)

// Game configuration
//#define MINIMAX_DEPTH 2  // Search depth for AI (2 reduced for memory constraints)
// 10 is too much, can take up to some minutes to resolve one move
// 4 is working as well, some milliseconds longer
// if you change the value please do as well with 'DIFFICULTY' above
#define MINIMAX_DEPTH 4  // Search depth for AI (2 reduced for memory constraints)
//int MINIMAX_DEPTH = 2; // 2 = easy, 4 = middle, 6 = hard
//const int mm_depth = 2;

#define MAX_MOVES 60  // Maximum possible moves (worst case)

// Board dimensions
#define ROWS 8
#define COLS 8

// Game modes
#define MODE_HUMAN_VS_AI 0
#define MODE_AI_VS_AI 1

// Direction vectors for searching in 8 directions
const int8_t DIRECTIONS[8][2] = {
  { -1, -1 }, { -1, 0 }, { -1, 1 },  // NW, N, NE
  { 0, -1 },
  { 0, 1 },  // W, E
  { 1, -1 },
  { 1, 0 },
  { 1, 1 }  // SW, S, SE
};

// --------------------------------------------------------------
// Game structure

// Game state - represents the board
struct OthelloState {
  byte board[ROWS][COLS];
  bool whiteTurn;  // true if it's white's turn, false for black's turn

  // Initialize the board with starting position
  void init() {
    whiteTurn = false;  // Black goes first

    // Initialize empty board
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        board[row][col] = EMPTY;
      }
    }

    // Set the four center pieces in starting position
    board[3][3] = WHITE;
    board[3][4] = BLACK;
    board[4][3] = BLACK;
    board[4][4] = WHITE;
  }
};

// Move structure - an Othello move is a row and column position
struct OthelloMove {
  byte row;
  byte col;

  OthelloMove()
    : row(0), col(0) {}
  OthelloMove(byte r, byte c)
    : row(r), col(c) {}
};

// Game logic implementation
class OthelloLogic : public Minimax<OthelloState, OthelloMove, MAX_MOVES, MINIMAX_DEPTH>::GameLogic {
public:
  // Check if a position is on the board
  bool isOnBoard(int row, int col) {
    return row >= 0 && row < ROWS && col >= 0 && col < COLS;
  }

  // Get the opponent's piece color
  byte getOpponent(byte piece) {
    return (piece == BLACK) ? WHITE : BLACK;
  }

  // Check if a move is valid by checking if it captures any opponent pieces
  bool isValidMove(const OthelloState& state, int row, int col, byte piece) {
    // The position must be empty
    if (state.board[row][col] != EMPTY) {
      return false;
    }

    byte opponent = getOpponent(piece);
    bool validInAnyDirection = false;

    // Check all 8 directions
    for (int d = 0; d < 8; d++) {
      int dr = DIRECTIONS[d][0];
      int dc = DIRECTIONS[d][1];
      int r = row + dr;
      int c = col + dc;

      // Look for opponent's piece adjacent
      if (isOnBoard(r, c) && state.board[r][c] == opponent) {
        // Continue in this direction looking for player's piece
        r += dr;
        c += dc;
        while (isOnBoard(r, c)) {
          if (state.board[r][c] == EMPTY) {
            break;  // Empty space, not valid in this direction
          }
          if (state.board[r][c] == piece) {
            validInAnyDirection = true;  // Found own piece, valid move
            break;
          }
          // Continue checking
          r += dr;
          c += dc;
        }
      }
    }

    return validInAnyDirection;
  }

  // Count how many pieces would be flipped by a move
  int countFlips(const OthelloState& state, int row, int col, byte piece) {
    byte opponent = getOpponent(piece);
    int flips = 0;

    // Check all 8 directions
    for (int d = 0; d < 8; d++) {
      int dr = DIRECTIONS[d][0];
      int dc = DIRECTIONS[d][1];
      int r = row + dr;
      int c = col + dc;

      int flipsInThisDirection = 0;

      // Look for opponent's piece adjacent
      if (isOnBoard(r, c) && state.board[r][c] == opponent) {
        // Continue in this direction looking for player's piece
        r += dr;
        c += dc;
        while (isOnBoard(r, c)) {
          if (state.board[r][c] == EMPTY) {
            flipsInThisDirection = 0;  // Not valid, reset count
            break;
          }
          if (state.board[r][c] == piece) {
            flips += flipsInThisDirection;  // Valid, add to total
            break;
          }
          flipsInThisDirection++;  // Count potential flip
          r += dr;
          c += dc;
        }
      }
    }

    return flips;
  }

  // Flip captured pieces after a move
  void flipPieces(OthelloState& state, int row, int col, byte piece) {
    byte opponent = getOpponent(piece);

    // Check all 8 directions
    for (int d = 0; d < 8; d++) {
      int dr = DIRECTIONS[d][0];
      int dc = DIRECTIONS[d][1];
      int r = row + dr;
      int c = col + dc;

      bool foundOpponent = false;

      // Look for opponent's piece adjacent
      if (isOnBoard(r, c) && state.board[r][c] == opponent) {
        foundOpponent = true;

        // Continue in this direction looking for player's piece
        r += dr;
        c += dc;
        while (isOnBoard(r, c)) {
          if (state.board[r][c] == EMPTY) {
            foundOpponent = false;  // Not valid in this direction
            break;
          }
          if (state.board[r][c] == piece) {
            break;  // Found own piece, valid direction
          }
          r += dr;
          c += dc;
        }

        // If we found a valid line to flip, go back and do the flipping
        if (foundOpponent && isOnBoard(r, c) && state.board[r][c] == piece) {
          r = row + dr;
          c = col + dc;
          while (state.board[r][c] == opponent) {
            state.board[r][c] = piece;
            r += dr;
            c += dc;
          }
        }
      }
    }
  }

  // Count pieces of a specific color on the board
  int countPieces(const OthelloState& state, byte piece) {
    int count = 0;
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (state.board[row][col] == piece) {
          count++;
        }
      }
    }
    return count;
  }

  // Check if the current player has any valid moves
  bool hasValidMoves(const OthelloState& state, byte piece) {
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (isValidMove(state, row, col, piece)) {
          return true;
        }
      }
    }
    return false;
  }

  // Evaluate board position from current player's perspective
  int evaluate(const OthelloState& state) override {
    // For the current player
    byte currentPiece = state.whiteTurn ? WHITE : BLACK;
    byte opponentPiece = getOpponent(currentPiece);

    // Count pieces
    int currentPieceCount = countPieces(state, currentPiece);
    int opponentPieceCount = countPieces(state, opponentPiece);

    // Basic score: difference in piece count
    int score = currentPieceCount - opponentPieceCount;

    // Bonus for corner pieces (very valuable in Othello)
    const int cornerValue = 25;
    if (state.board[0][0] == currentPiece) score += cornerValue;
    if (state.board[0][7] == currentPiece) score += cornerValue;
    if (state.board[7][0] == currentPiece) score += cornerValue;
    if (state.board[7][7] == currentPiece) score += cornerValue;

    if (state.board[0][0] == opponentPiece) score -= cornerValue;
    if (state.board[0][7] == opponentPiece) score -= cornerValue;
    if (state.board[7][0] == opponentPiece) score -= cornerValue;
    if (state.board[7][7] == opponentPiece) score -= cornerValue;

    // Penalty for positions adjacent to corners if corner is empty
    // (this can give opponent access to corners)
    const int badPositionValue = 8;

    // Top-left corner adjacents
    if (state.board[0][0] == EMPTY) {
      if (state.board[0][1] == currentPiece) score -= badPositionValue;
      if (state.board[1][0] == currentPiece) score -= badPositionValue;
      if (state.board[1][1] == currentPiece) score -= badPositionValue;

      if (state.board[0][1] == opponentPiece) score += badPositionValue;
      if (state.board[1][0] == opponentPiece) score += badPositionValue;
      if (state.board[1][1] == opponentPiece) score += badPositionValue;
    }

    // Top-right corner adjacents
    if (state.board[0][7] == EMPTY) {
      if (state.board[0][6] == currentPiece) score -= badPositionValue;
      if (state.board[1][7] == currentPiece) score -= badPositionValue;
      if (state.board[1][6] == currentPiece) score -= badPositionValue;

      if (state.board[0][6] == opponentPiece) score += badPositionValue;
      if (state.board[1][7] == opponentPiece) score += badPositionValue;
      if (state.board[1][6] == opponentPiece) score += badPositionValue;
    }

    // Bottom-left corner adjacents
    if (state.board[7][0] == EMPTY) {
      if (state.board[7][1] == currentPiece) score -= badPositionValue;
      if (state.board[6][0] == currentPiece) score -= badPositionValue;
      if (state.board[6][1] == currentPiece) score -= badPositionValue;

      if (state.board[7][1] == opponentPiece) score += badPositionValue;
      if (state.board[6][0] == opponentPiece) score += badPositionValue;
      if (state.board[6][1] == opponentPiece) score += badPositionValue;
    }

    // Bottom-right corner adjacents
    if (state.board[7][7] == EMPTY) {
      if (state.board[7][6] == currentPiece) score -= badPositionValue;
      if (state.board[6][7] == currentPiece) score -= badPositionValue;
      if (state.board[6][6] == currentPiece) score -= badPositionValue;

      if (state.board[7][6] == opponentPiece) score += badPositionValue;
      if (state.board[6][7] == opponentPiece) score += badPositionValue;
      if (state.board[6][6] == opponentPiece) score += badPositionValue;
    }

    // Bonus for edge pieces (also valuable)
    const int edgeValue = 5;
    for (int i = 2; i < 6; i++) {
      if (state.board[0][i] == currentPiece) score += edgeValue;
      if (state.board[7][i] == currentPiece) score += edgeValue;
      if (state.board[i][0] == currentPiece) score += edgeValue;
      if (state.board[i][7] == currentPiece) score += edgeValue;

      if (state.board[0][i] == opponentPiece) score -= edgeValue;
      if (state.board[7][i] == opponentPiece) score -= edgeValue;
      if (state.board[i][0] == opponentPiece) score -= edgeValue;
      if (state.board[i][7] == opponentPiece) score -= edgeValue;
    }

    // Bonus for mobility (having more valid moves)
    int currentMoveCount = 0;
    int opponentMoveCount = 0;

    // Temporarily change state to count opponent's moves
    OthelloState tempState = state;
    tempState.whiteTurn = !tempState.whiteTurn;

    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (isValidMove(state, row, col, currentPiece)) {
          currentMoveCount++;
        }
        if (isValidMove(tempState, row, col, opponentPiece)) {
          opponentMoveCount++;
        }
      }
    }

    const int mobilityValue = 2;
    score += mobilityValue * (currentMoveCount - opponentMoveCount);

    // Terminal state considerations
    if (isTerminal(state)) {
      if (currentPieceCount > opponentPieceCount) {
        return 10000;  // Win
      } else if (currentPieceCount < opponentPieceCount) {
        return -10000;  // Loss
      } else {
        return 0;  // Draw
      }
    }

    return score;
  }

  // Generate all valid moves from the current state
  int generateMoves(const OthelloState& state, OthelloMove moves[], int maxMoves) override {
    int moveCount = 0;
    byte currentPiece = state.whiteTurn ? WHITE : BLACK;

    // A move is valid if it flips at least one opponent's piece
    for (int row = 0; row < ROWS && moveCount < maxMoves; row++) {
      for (int col = 0; col < COLS && moveCount < maxMoves; col++) {
        if (isValidMove(state, row, col, currentPiece)) {
          moves[moveCount] = OthelloMove(row, col);
          moveCount++;
        }
      }
    }

    return moveCount;
  }

  // Apply a move to a state, modifying the state
  void applyMove(OthelloState& state, const OthelloMove& move) override {
    byte currentPiece = state.whiteTurn ? WHITE : BLACK;

    // Place the piece
    state.board[move.row][move.col] = currentPiece;

    // Flip captured pieces
    flipPieces(state, move.row, move.col, currentPiece);

    // Switch turns
    state.whiteTurn = !state.whiteTurn;
  }

  // Check if the game has reached a terminal state
  bool isTerminal(const OthelloState& state) override {
    // Game is over if neither player has valid moves
    bool blackHasMoves = hasValidMoves(state, BLACK);
    bool whiteHasMoves = hasValidMoves(state, WHITE);

    if (!blackHasMoves && !whiteHasMoves) {
      return true;
    }

    return false;
  }

  // Check if the current player is the maximizing player
  bool isMaximizingPlayer(const OthelloState& state) override {
    // WHITE is the maximizing player (AI)
    return state.whiteTurn;
  }
};

// Global variables
OthelloState gameState;
OthelloLogic gameLogic;
Minimax<OthelloState, OthelloMove, MAX_MOVES, MINIMAX_DEPTH> minimaxAI(gameLogic);
int gameMode = MODE_HUMAN_VS_AI;  // Default to Human vs AI

// Function to display the board with emoji symbols
void displayBoard(const OthelloState& state) {
  // Column numbers with emoji numbers
  Serial.print("  ");
  for (int col = 0; col < COLS; col++) {
    switch (col) {
      case 0: Serial.print(" 0️⃣ "); break;
      case 1: Serial.print("1️⃣ "); break;
      case 2: Serial.print("2️⃣ "); break;
      case 3: Serial.print("3️⃣ "); break;
      case 4: Serial.print("4️⃣ "); break;
      case 5: Serial.print("5️⃣ "); break;
      case 6: Serial.print("6️⃣ "); break;
      case 7: Serial.print("7️⃣ "); break;
    }
  }
  Serial.println();

  for (int row = 0; row < ROWS; row++) {
    // Row numbers with emoji numbers
    switch (row) {
      case 0: Serial.print("0️⃣ "); break;
      case 1: Serial.print("1️⃣ "); break;
      case 2: Serial.print("2️⃣ "); break;
      case 3: Serial.print("3️⃣ "); break;
      case 4: Serial.print("4️⃣ "); break;
      case 5: Serial.print("5️⃣ "); break;
      case 6: Serial.print("6️⃣ "); break;
      case 7: Serial.print("7️⃣ "); break;
    }

    for (int col = 0; col < COLS; col++) {
      byte currentPiece = state.whiteTurn ? WHITE : BLACK;

      if (state.board[row][col] == EMPTY) {
        // Show hint if this is a valid move for current player
        if (gameLogic.isValidMove(state, row, col, currentPiece)) {
          Serial.print("❓");  // Question mark for valid move
        } else {
          Serial.print("⬜");  // Empty square
        }
      } else if (state.board[row][col] == BLACK) {
        Serial.print("⚫");  // Black circle
      } else {              // WHITE
        Serial.print("⚪");  // White circle
      }
      Serial.print(" ");
    }

    Serial.println();
  }

  // Display current player and piece counts
  int blackCount = gameLogic.countPieces(state, BLACK);
  int whiteCount = gameLogic.countPieces(state, WHITE);

  Serial.print("⚫ BLACK: ");
  Serial.print(blackCount);
  Serial.print("  ⚪ WHITE: ");
  Serial.println(whiteCount);

  Serial.print(state.whiteTurn ? "⚪ WHITE's turn" : "⚫ BLACK's turn");
  Serial.println();
}


void displayBoardTft(const OthelloState& state) {
  uint8_t btnPos;
  for (int row = 0; row < ROWS; row++) {
    for (int col = 0; col < COLS; col++) {
      byte currentPiece = state.whiteTurn ? WHITE : BLACK;
      btnPos = col + row * 8;
      if (state.board[row][col] == EMPTY) {
        // Show hint if this is a valid move for current player
        if (gameLogic.isValidMove(state, row, col, currentPiece)) {
          //Serial.print("❓");  // Question mark for valid move
          if (gameState.whiteTurn) {
            keyColor[btnPos] = TFT_RED;
          } else {
            keyColor[btnPos] = TFT_GREEN;
          }
        } else {
          //Serial.print("⬜");  // Empty square
          keyColor[btnPos] = TFT_DARKGREY;
        }
      } else if (state.board[row][col] == BLACK) {
        //Serial.print("⚫");  // Black circle
        keyColor[btnPos] = TFT_BLACK;
      } else {  // WHITE
        //Serial.print("⚪");  // White circle
        keyColor[btnPos] = TFT_WHITE;
      }
      //Serial.print(" ");
    }
  }

  //Serial.println();
  drawKeypad();

  // Display current player and piece counts
  int blackCount = gameLogic.countPieces(state, BLACK);
  int whiteCount = gameLogic.countPieces(state, WHITE);

  tft.drawFastHLine(0, RESULT_LINE_Y, SCREEN_WIDTH, TFT_YELLOW);
  tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
  tft.setTextSize(2);
  tft.drawString("BLACK:", 5, RESULT_DATA_Y);
  tft.setTextColor(TFT_GREEN, TFT_DARKGREY);
  tft.drawString(String(blackCount) + "   ", 90, RESULT_DATA_Y);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.drawString("WHITE:", 120, RESULT_DATA_Y);
  tft.setTextColor(TFT_RED, TFT_DARKGREY);
  tft.drawString(String(whiteCount) + "   ", 205, RESULT_DATA_Y);
}


// Function to get a move from human player
// This is using the touch screen
OthelloMove getHumanMoveTouch() {
  OthelloMove move;
  byte currentPiece = gameState.whiteTurn ? WHITE : BLACK;

  move.row = touchedButton / 8;
  move.col = touchedButton % 8;
  Serial.printf("key %d just pressed row: %d col: %d\n", touchedButton, move.row, move.col);

  if (gameLogic.isValidMove(gameState, move.row, move.col, currentPiece)) {
    Serial.println("This is a valid move");
  } else {
    Serial.println("This move is INVALID");
  }

  return move;
}

// Function to get a move from human player
OthelloMove getHumanMoveSerial() {
  OthelloMove move;
  bool validMove = false;
  byte currentPiece = gameState.whiteTurn ? WHITE : BLACK;

  while (!validMove) {
    // Prompt for input
    Serial.println("Enter row and column (e.g., '3 4'):");

    // Wait for input
    while (!Serial.available()) {
      delay(100);
    }

    // Read the row and column
    move.row = Serial.parseInt();
    move.col = Serial.parseInt();

    // Clear the input buffer
    while (Serial.available()) {
      Serial.read();
    }

    // Check if the position is valid
    if (move.row < ROWS && move.col < COLS) {
      // Check if the move is legal
      if (gameLogic.isValidMove(gameState, move.row, move.col, currentPiece)) {
        validMove = true;
      } else {
        Serial.println("Invalid move. Try another position.");
      }
    } else {
      Serial.println("Invalid position. Please enter row (0-7) and column (0-7).");
    }
  }

  return move;
}

// Function to get AI move
OthelloMove getAIMove() {
  Serial.println("AI is thinking...");
  tft.setTextColor(TFT_SKYBLUE, TFT_DARKGREY);
  tft.setTextSize(2);
  tft.drawString("AI is thinking...", 5, STATUS_DATA_Y);

  unsigned long startTime = millis();
  OthelloMove move = minimaxAI.findBestMove(gameState);
  unsigned long endTime = millis();

  Serial.print("AI chose position: ");
  Serial.print(move.row);
  Serial.print(", ");
  Serial.println(move.col);

  Serial.print("Nodes searched: ");
  Serial.println(minimaxAI.getNodesSearched());

  Serial.print("Time: ");
  Serial.print((endTime - startTime) / 1000.0);
  Serial.println(" seconds");
  tft.drawString("AI is done !           ", 5, STATUS_DATA_Y);

  return move;
}

// Function to check for game over
bool checkGameOver() {
  if (gameLogic.isTerminal(gameState)) {
    displayBoard(gameState);

    // Determine the winner
    int blackCount = gameLogic.countPieces(gameState, BLACK);
    int whiteCount = gameLogic.countPieces(gameState, WHITE);
    tft.setTextColor(TFT_SKYBLUE, TFT_DARKGREY);
    tft.setTextSize(2);
    if (blackCount > whiteCount) {
      Serial.println("BLACK wins!");
      tft.drawString("BLACK wins!", 5, STATUS_DATA_Y);
    } else if (whiteCount > blackCount) {
      Serial.println("WHITE wins!");
      tft.drawString("WHITE wins!", 5, STATUS_DATA_Y);
    } else {
      Serial.println("Game ended in a draw!");
      tft.drawString("Game ended in a draw!", 5, STATUS_DATA_Y);
    }

    Serial.println("Enter 'r' to restart or 'm' to change mode.");
    return true;
  }

  return false;
}

// Function to handle game setup and restart
void setupGameTouch() {
  gameState.init();

  Serial.println("\n=== OTHELLO / REVERSI ===");
  Serial.println("Game Modes:");
  Serial.println("1. Human (Black) vs. AI (White)");
  Serial.println("2. AI vs. AI");
  Serial.println("Select mode (1-2):");
  // reset all fonts and sizes
  tft.setFont(&fonts::Font0);
  tft.setTextSize(1);
  drawGameName();
  drawModeButtons();
}

// old code, not use anymore
void setupGameSerial() {
  gameState.init();

  Serial.println("\n=== OTHELLO / REVERSI ===");
  Serial.println("Game Modes:");
  Serial.println("1. Human (Black) vs. AI (White)");
  Serial.println("2. AI vs. AI");
  Serial.println("Select mode (1-2):");

  drawGameName();
  drawModeButtons();
  //drawDifficultyButtons();


  while (!Serial.available()) {
    delay(100);
  }

  char choice = Serial.read();

  // Clear the input buffer
  while (Serial.available()) {
    Serial.read();
  }

  if (choice == '2') {
    gameMode = MODE_AI_VS_AI;
    Serial.println("AI vs. AI mode selected.");
  } else {
    gameMode = MODE_HUMAN_VS_AI;
    Serial.println("Human vs. AI mode selected.");
    Serial.println("You play as Black, AI plays as White.");
  }
}

//------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;  // Wait for serial port to connect
  }

  Serial.println("ESP32 CYD LovyanGFX Othello with Minimax V08");

  //------------------------------------------------------------------------------------------
  // initialize the display

  // LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS initialisation failed!");
    Serial.println("formatting file system");
    LittleFS.format();
    LittleFS.begin();
    //while (1) yield();  // Stay here twiddling thumbs waiting
  }

  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(0);  // In my case '2' is better as the terminals are at the top

  // Calibrate the touch screen and retrieve the scaling factors
  touch_calibrate_LovyanGFX();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  // Draw keypad background
  tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);
  initKeyColors();

  //------------------------------------------------------------------------------------------
  // start the game logic

  randomSeed(analogRead(0));
  setupGameTouch();
}

void loop() {

  // select the mode and difficulty
  if (isSettingsMode) {
    uint16_t t_x = 0, t_y = 0;  // To store the touch coordinates
    // Pressed will be set true is there is a valid touch on the screen
    bool pressed = tft.getTouch(&t_x, &t_y);
    // Check if any button coordinate boxes contain the touch coordinates

    for (uint8_t b = 0; b < 2; b++) {
      if (pressed && modeButton[b].contains(t_x, t_y)) {
        modeButton[b].press(true);  // tell the button it is pressed
      } else {
        modeButton[b].press(false);  // tell the button it is NOT pressed
      }
    }

    // Check if any button has changed state
    for (uint8_t b = 0; b < 2; b++) {

      if (modeButton[b].justReleased()) modeButton[b].drawButton();  // draw normal

      if (modeButton[b].justPressed()) {
        Serial.printf("modeButton %d just pressed\n", b);
        // human vs ai
        gameMode = MODE_HUMAN_VS_AI;  // default
        if (b == 0) {
          // human vs ai
          gameMode = MODE_HUMAN_VS_AI;
          Serial.println("Human vs. AI mode selected.");
          Serial.println("You play as Black, AI plays as White.");
          tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);
          isSettingsMode = false;
        } else if (b == 1) {
          // ai vs ai
          gameMode = MODE_AI_VS_AI;
          Serial.println("AI vs. AI mode selected.");
          tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);
          isSettingsMode = false;
        }
        //modeButton[b].drawButton(true);  // draw invert
        delay(10);  // UI debouncing
      }
    }

  }  // if settingsMode end
  else {
    // game mode

    if (isWaitingForHumanMove) {
      uint16_t t_x = 0, t_y = 0;  // To store the touch coordinates
      // Pressed will be set true is there is a valid touch on the screen
      bool pressed = tft.getTouch(&t_x, &t_y);
      // Check if any key coordinate boxes contain the touch coordinates
      for (uint8_t b = 0; b < NUMBER_OF_KEYS; b++) {
        if (pressed && key[b].contains(t_x, t_y)) {
          key[b].press(true);  // tell the button it is pressed
        } else {
          key[b].press(false);  // tell the button it is NOT pressed
        }
      }

      // Check if any key has changed state
      for (uint8_t b = 0; b < NUMBER_OF_KEYS; b++) {

        if (key[b].justReleased()) key[b].drawButton();  // draw normal

        if (key[b].justPressed()) {
          Serial.printf("key %d just pressed\n", b);
          if (keyColor[b] == TFT_GREEN) {
            // these are the only allowed touches
            Serial.printf("green button %d just pressed\n", b);
            touchedButton = b;
            isButtonTouched = true;
            // Get and apply move based on game mode and current player
            OthelloMove move;

            move = getHumanMoveTouch();
            // Apply the move
            gameLogic.applyMove(gameState, move);

            isWaitingForHumanMove = false;
          }
          key[b].drawButton(true);  // draw invert
          delay(10);                // UI debouncing
        }
      }
    } else {
      clearStatus();
      // Display the current board state
      displayBoard(gameState);     // Serial Monitor
      displayBoardTft(gameState);  // TFT Display

      // Check for game over
      if (checkGameOver()) {
        isSettingsMode = true;
        delay(5000);
        tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);
        setupGameTouch();
        return;
      }

      // Get current player piece
      byte currentPiece = gameState.whiteTurn ? WHITE : BLACK;

      // Check if current player has valid moves
      bool hasValidMove = gameLogic.hasValidMoves(gameState, currentPiece);

      tft.setTextColor(TFT_SKYBLUE, TFT_DARKGREY);
      tft.setTextSize(2);
      tft.setTextWrap(true, true);

      if (!hasValidMove) {
        // Current player has no valid moves
        Serial.println("No valid moves for current player, skipping turn");
        tft.drawString("No valid moves for current player, skipping turn", 5, STATUS_DATA_Y);

        // Switch turns
        gameState.whiteTurn = !gameState.whiteTurn;

        // Check if the other player also has no valid moves (game over)
        currentPiece = gameState.whiteTurn ? WHITE : BLACK;
        if (!gameLogic.hasValidMoves(gameState, currentPiece)) {
          Serial.println("Both players have no valid moves. Game over!");
          tft.drawString("Both players have no valid moves. Game over!", 5, STATUS_DATA_Y);
          checkGameOver();
          isSettingsMode = true;
          tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);
          setupGameTouch();
          delay(5000);
          return;
        }

        // Display updated board after turn skip
        displayBoard(gameState);     // Serial Monitor
        displayBoardTft(gameState);  // TFT Display
      } else {
        clearStatus();
      }

      // Get and apply move based on game mode and current player
      OthelloMove move;

      if (gameMode == MODE_HUMAN_VS_AI) {
        if (!gameState.whiteTurn) {
          // Human's turn (Black)
          tft.setTextColor(TFT_SKYBLUE, TFT_DARKGREY);
          tft.setTextSize(2);
          tft.drawString("Make a move", 5, STATUS_DATA_Y);
          isWaitingForHumanMove = true;
        } else {
          // AI's turn (White)
          move = getAIMove();
          delay(1000);  // Small delay to make AI moves visible
        }
      } else {
        // AI vs. AI mode
        move = getAIMove();
        delay(2000);  // Longer delay to observe the game
      }

      // Apply the move for AI moves only
      if (!isWaitingForHumanMove) {
        gameLogic.applyMove(gameState, move);
      }
    }
  }
}
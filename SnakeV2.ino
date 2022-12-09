#include <Adafruit_GFX.h>
#include <RGBmatrixPanel.h>
#include <LinkedList.h>


#define CLK 8
#define OE  9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3
#define WIDTH 32
#define HEIGHT 16
#define messageSpeed 2


RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

const short initialSnakeLength = 3;


void setup() {
    Serial.begin(115200);
    initialize();
}

const long interval = 6000;
unsigned long previousMillis = 0;
const uint16_t COLORS[3] = {matrix.Color333(3, 0, 4), matrix.Color333(0, 4, 4), 0x9480};
short outlineColor = 0;

void loop() {
    generateFood();
    calculateSnake();
    updateColor();
    handleGameStates();
    delay(50);
}

struct Point {
    short row = 0, col = 0;

    Point(short row = 0, short col = 0) : row(row), col(col) {}
};

bool win = false;
bool gameOver = false;

Point snake;
LinkedList <Point> snakeList = LinkedList<Point>();
Point food[3] = {Point(-1, -1), Point(-1, -1), Point(-1, -1)}; // 3 different food colors

int snakeLength = initialSnakeLength; // chosen by the user in the config section
int snakeDirection = 1;

// direction constants
#define up 1
#define right 2
#define down 3 // 'down - 2' must be 'up'
#define left 4 // 'left - 2' must be 'right'

// snake body segments storage




// --------------------------------------------------------------- //
// -------------------------- functions -------------------------- //
// --------------------------------------------------------------- //

void updateColor() {
    // Shift the color value to the next color
    unsigned long currentMillis = millis();
    unsigned long diff = currentMillis - previousMillis;
    if (previousMillis == 0 || diff >= interval) {
        previousMillis = currentMillis;
        outlineColor = (outlineColor + 1) % 3;
        matrix.drawRect(0, 0, WIDTH, HEIGHT, COLORS[outlineColor]);
    } else if (diff >= interval - 1000) {

        if (diff >= interval - 250) {
            matrix.drawRect(0, 0, WIDTH, HEIGHT, COLORS[outlineColor]);

        } else if (diff >= interval - 500) {
            matrix.drawRect(0, 0, WIDTH, HEIGHT, 0);

        } else if (diff >= interval - 750) {
            matrix.drawRect(0, 0, WIDTH, HEIGHT, COLORS[outlineColor]);

        } else {
            matrix.drawRect(0, 0, WIDTH, HEIGHT, 0);

        }
    }
}

/**
 * generates food
 */
void generateFood() {
    if (snakeLength >= WIDTH * HEIGHT - 4) {
        win = true;
        return;
    }
    Point *foo;
    for (int i = 0; i < 3; i++) {
        foo = &food[i];
        if (foo->row == -1 || foo->col == -1) {
            int col, row;
            do {
                col = random(WIDTH - 2) + 1;
                row = random(HEIGHT - 2) + 1;
            } while (isInvalidFood(col, row));
            foo->col = col;
            foo->row = row;
            matrix.drawPixel(foo->col, foo->row, getFoodColor(i));
        }
    }
}

int getFoodColor(int index) {
    return COLORS[index];
}

short evaluate(short x, short y, short nextDir) {
    switch (nextDir) {
        case up:
            y--;
            break;
        case right:
            x++;
            break;
        case down:
            y++;
            break;
        case left:
            x--;
            break;
        default:
            break;
    }

    /*
     * Instant-Death: (Very Negative)
     */
    if (x == 0 || x >= WIDTH - 1) return -100;
    if (y == 0 || y >= HEIGHT - 1) return -100;
    if (isOccupied(x, y)) return -100;
    if (abs(nextDir - snakeDirection) == 2) return -100;
    /*
     * Bad Position: (Scaling On Position to Border)
     */
    short bestDistance = 0;
    for (int i = 0; i < 3; i++) {
        Point *foo = &food[i];
        int foodX = abs(foo->col - x);
        int foodY = abs(foo->row - y);
        if (foodX == 0 && foodY == 0) {
            if (i == outlineColor) {
                return 1000;
            } else {
                return -100;
            }
        } else {
            if (i == outlineColor) {
                short distanceSq = sqrt((30 - foodX) * (30 - foodX) + (14 - foodY) * (14 - foodY)) * 7;
                bestDistance = max(bestDistance, distanceSq);
            }
        }
    }
    return bestDistance;
}

int dirs[4] = {0};

void processDirection() {
    dirs[0] = evaluate(snake.col, snake.row, 1);
    dirs[1] = evaluate(snake.col, snake.row, 2);
    dirs[2] = evaluate(snake.col, snake.row, 3);
    dirs[3] = evaluate(snake.col, snake.row, 4);
    int e = max(dirs[0], max(dirs[1], max(dirs[2], dirs[3])));
    for (int i = 0; i < 4; ++i) {
        if (dirs[i] == e) {
            snakeDirection = i + 1;
            break;
        }
    }
}

// calculate snake movement data
void calculateSnake() {
    processDirection();
    switch (snakeDirection) {
        case up:
            snake.row--;
            break;

        case right:
            snake.col++;
            break;

        case down:
            snake.row++;
            break;

        case left:
            snake.col--;
            break;

        default:
            snake.row--;
            break;
    }

    if (isOccupied(snake.col, snake.row)) {
        gameOver = true;
        return;
    }
    // check if the food was eaten
    Point *foo;
    for (int i = 0; i < 3; i++) {
        foo = &food[i];
        if (snake.row == foo->row && snake.col == foo->col) {
            if (outlineColor == i) {
                matrix.drawPixel(foo->col, foo->row, matrix.Color333(0, 0, 0));
                foo->row = foo->col = -1; // reset food
                snakeLength++;
            } else { // gameOver
                gameOver = true;
                return;
            }
            break;
        }
    }
    snakeList.add(snake);
    matrix.drawPixel(snake.col, snake.row, matrix.Color333(0, 7, 0));
    int listSize = snakeList.size();
    if (listSize > snakeLength) {
        Point c = snakeList.shift();
        matrix.drawPixel(c.col, c.row, matrix.Color333(0, 0, 0));
    }
}

bool isInvalidFood(int col, int row) {
    if (isOccupied(col, row)) return true;
    for (Point foo: food) {
        if (foo.col == col && foo.row == row) {
            return true;
        }
    }
    return false;
}

bool isOccupied(int col, int row) {
    if (col == 0 || col >= WIDTH - 1) return true;
    if (row == 0 || row >= HEIGHT - 1) return true;
    int h;
    int listSize = snakeList.size();
    Point c;
    for (h = 0; h < listSize; h++) {
        c = snakeList.get(h);
        if (c.col == col && c.row == row) {
            return true;
        }
    }
    return false;
}

void handleGameStates() {
    if (gameOver || win) {
        unrollSnake();
        showScoreMessage(snakeLength - initialSnakeLength);
        win = gameOver = false;
        snake.row = random(HEIGHT - 2) + 1;
        snake.col = random(WIDTH - 2) + 1;
        snakeList.add(snake);
        Point *foo;
        for (int i = 0; i < 3; i++) {
            foo = &food[i];
            foo->row = foo->col = -1;
        }
        snakeLength = initialSnakeLength;
        snakeDirection = random(4) + 1;
        matrix.fillScreen(0);
        updateColor();
    }
}


void unrollSnake() {
    for (Point foo: food) {
        matrix.drawPixel(foo.col, foo.row, matrix.Color333(0, 0, 0));
    }
    delay(800);
    int h;
    int listSize = snakeList.size();
    Point c;
    for (int i = 0; i < 3; i++) {
        matrix.fillScreen(matrix.Color333(0, 7, 0));
        for (h = 0; h < listSize; h++) {
            c = snakeList.get(h);
            matrix.drawPixel(c.col, c.row, matrix.Color333(0, 0, 0));
        }
        delay(100);
        matrix.fillScreen(0);
        for (h = 0; h < listSize; h++) {
            c = snakeList.get(h);
            matrix.drawPixel(c.col, c.row, matrix.Color333(0, 7, 0));
        }
        delay(140);
    }
    delay(750);
    int size;
    while (snakeList.size() > 0) {
        c = snakeList.shift();
        matrix.drawPixel(c.col, c.row, matrix.Color333(0, 0, 0));
        delay(60 + (200 / snakeList.size()));
    }
    delay(500);
}

void initialize() {
    matrix.begin();
    randomSeed(analogRead(A5));
    snake.row = random(HEIGHT - 2) + 1;
    snake.col = random(WIDTH - 2) + 1;
    snakeList.clear();
    snakeList.add(snake);
    updateColor();
}
// --------------------------------------------------------------- //

// --------------------------------------------------------------- //
// -------------------------- messages --------------------------- //

const PROGMEM bool gameOverMessage[8][90] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                             {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

const PROGMEM bool digits[][8][8] = {{{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 1, 1, 1, 0}, {0, 1, 1, 1, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 1, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 0, 1, 1, 0}, {0, 0, 0, 1, 1, 1, 0, 0}, {0, 0, 0, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 0, 1, 1, 1, 0, 0}, {0, 0, 1, 0, 1, 1, 0, 0}, {0, 1, 0, 0, 1, 1, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 0}, {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 0, 0, 1, 1, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 0, 0}, {0, 0, 0, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 0, 0, 1, 1, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}},
                                     {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 1, 0}, {0, 0, 0, 0, 0, 1, 1, 0}, {0, 1, 1, 0, 0, 1, 1, 0}, {0, 0, 1, 1, 1, 1, 0, 0}}};


void showScoreMessage(int score) {
    if (score < 0 || score > 99) return;
    int second = score % 10;
    int first = (score / 10) % 10;

    [&] {
        for (int d = 0; d < sizeof(gameOverMessage[0]) + 2 * sizeof(digits[0][0]); d++) {
            for (int col = 0; col < 32; col++) {
                delay(messageSpeed);
                for (int row = 0; row < 8; row++) {
                    if (d <= sizeof(gameOverMessage[0]) - 32) {
                        if (pgm_read_byte(&(gameOverMessage[row][col + d])) == 1)
                            matrix.drawPixel(col, row, matrix.Color333(0, 7, 0));
                        else
                            matrix.drawPixel(col, row, matrix.Color333(0, 0, 0));
                    }

                    int c = col + d - sizeof(gameOverMessage[0]) + 6; // move 6 px in front of the previous message
                    if (score < 10) c += 8;

                    if (c >= 0 && c < 8) {
                        if (first > 0) {
                            if (pgm_read_byte(&(digits[first][row][c])) == 1)
                                matrix.drawPixel(col, row, matrix.Color333(0, 7, 0));
                            else
                                matrix.drawPixel(col, row, matrix.Color333(0, 0, 0));
                        }
                    } else {
                        c -= 8;
                        if (c >= 0 && c < 8) {
                            if (pgm_read_byte(&(digits[second][row][c])) == 1)
                                matrix.drawPixel(col, row, matrix.Color333(0, 7, 0));
                            else
                                matrix.drawPixel(col, row, matrix.Color333(0, 0, 0));
                        }
                    }
                }
            }
        }
    }();

    delay(1200);
    matrix.fillScreen(0);

}
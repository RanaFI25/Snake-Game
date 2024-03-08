#define F_CPU 1000000UL
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <lcd.h>
#include <nokia5110.h>
#define True 1
#define False 0
#define XMaxLimit 83
#define YMaxLimit 47
#define XMinLimit 0
#define YMinLimit 0
static int score = 00;
uint8_t font[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f,
                    0x6f};
volatile uint8_t framebuffer2[4]; // use to buffer contents of display
static uint8_t play = True;
static uint8_t newGame = True;
struct Snake *snake;
struct Food *food;
static int downPressed = False;
static int upPressed = False;
static int leftPressed = False;
static int rightPressed = False;
ISR(TIMER0_OVF_vect) // the code for the interrupt service routine
{
    static uint8_t current_digit = 0;
    PORTB = 0xff;       // all segments off
    current_digit += 1; // step one digit
    current_digit %= 4; // and keep between 0-3
    PORTC = (PORTC & 0b00001111) | (0b00010000 << current_digit);
    PORTB = framebuffer2[current_digit];
}
void display(uint16_t value) // print a 4-digit number
{
    uint16_t A = value;
    uint8_t i;
    for (i = 0; i < 4; i++)
    {
        framebuffer2[3 - i] = ~font[A % 10];
        A = A / 10;
    }
}
// Every Part of the snake contains value for all this variable like x,y for
curr pos.struct Part
{
    int x, y;
    int xd, yd;
    int xv, yv;
    struct Part *next;
    struct Part *prev;
};
// Snake
struct Snake
{
    struct Part *head;
    struct Part *end;
};
// creates a new part of Snake and gives value to all varibles
struct Part *createNewPart(int xPos, int yPos, int xDirection, int yDirection, int xSpeed, int ySpeed)
{
    struct Part *p = (struct Part *)calloc(1, sizeof(struct Part)); // allocate
    memory if (p == NULL)
    {
        exit(-1);
    }
    p->x = xPos;
    p->y = yPos;
    p->xd = xDirection;
    p->yd = yDirection;
    p->xv = xSpeed;
    p->yv = ySpeed;
    return p;
};
struct Food
{
    int x, y;
};
struct Food *createFood(int xPos, int yPos)
{
    struct Food *p = (struct Food *)calloc(1, sizeof(struct Food)); // allocate
    memory if (p == NULL)
    {
        exit(-1);
    }
    p->x = xPos;
    p->y = yPos;
    return p;
}
ISR(INT0_vect)
{
    if (!(PINC & 0b00000100) && (snake->head->xd == 1 || snake->head->xd ==
                                                             -1))
    { // down
        downPressed = True;
    }
    if (!(PINC & 0b00000001) && (snake->head->xd == 1 || snake->head->xd ==
                                                             -1))
    { // up
        upPressed = True;
    }
    if (!(PINC & 0b00000010) && (snake->head->yd == 1 || snake->head->yd ==
                                                             -1))
    { // right
        rightPressed = True;
    }
    if (!(PINC & 0b00001000) && (snake->head->yd == 1 || snake->head->yd ==
                                                             -1))
    { // left
        leftPressed = True;
    }
}
struct Food *generate_Food(void)
{
    // between 5 and 80. x
    //  y between 5 and 44.
    int xMaxLimitFood = 80;
    int yMaxLimitFood = 44;
    int xMinLimitFood = 5;
    int yMinLimitFood = 5;
    int nRandonNumberX = rand() % ((xMaxLimitFood)-xMinLimitFood) +
                         xMinLimitFood;
    int nRandonNumberY = rand() % ((yMaxLimitFood)-yMinLimitFood) +
                         yMinLimitFood;
    food = createFood(nRandonNumberX, nRandonNumberY);
    return food;
}
// check if food is at same place as any snake body part
int food_location(struct Part **head, struct Food *currFood)
{
    struct Part *pHeader = *head;
    int foodX = currFood->x;
    int foodY = currFood->y;
    int x, y;
    int samePlace = False;
    while (pHeader != NULL)
    {
        x = pHeader->x;
        y = pHeader->y;
        if (x == foodX && y == foodY)
        {
            samePlace = True;
            return samePlace;
        }
        pHeader = pHeader->next;
    }
    return samePlace;
}
// Add a part to the snake at the end of it(the list).
int addPartsSnake(struct Snake **snake, struct Part *part)
{
    struct Snake *pSnake = *snake;
    struct Part *pHeader = pSnake->head;
    if (snake != NULL)
    {
        while (pHeader->next != NULL)
        {
            pHeader = pHeader->next;
        }
        pHeader->next = part;
        part->prev = pHeader;
        pSnake->end = part;
        return 1;
    }
    return 0;
}
struct Snake *newSnake(struct Part *head, struct Part *end)
{
    struct Snake *p = (struct Snake *)calloc(1, sizeof(struct Snake));
    if (head == NULL || end == NULL || p == NULL)
    {
        return NULL;
    }
    p->head = head;
    p->end = end;
    return p;
};
// print the snake's head and its parts.
void displaySnake(struct Part **head)
{
    struct Part *pHeader = *head;
    int x, y;
    while (pHeader != NULL)
    {
        x = pHeader->x;
        y = pHeader->y;
        NOKIA_setpixel(x, y);
        pHeader = pHeader->next;
    }
}
void clear_snake(struct Part **head)
{
    struct Part *pHeader = *head;
    int x, y;
    while (pHeader != NULL)
    {
        x = pHeader->x;
        y = pHeader->y;
        NOKIA_clearpixel(x, y);
        pHeader = pHeader->next;
    }
}
void borderX(int y, int size)
{
    for (int i = 0; i < size; i++)
    {
        NOKIA_setpixel(i, y);
    }
}
void borderY(int x, int size)
{
    for (int i = 0; i < size; i++)
    {
        NOKIA_setpixel(x, i);
    }
}
void init()
{
    NOKIA_init(0);
    borderX(0, 83);
    borderX(47, 83);
    borderY(83, 47);
    borderY(0, 47);
    NOKIA_update(); // transfer framebuffer to the Nokia
    display
        // DDRC = 0b00000000;
        DDRC = 0b11110000; // upper B-pins as output
    // DDRC = 0b00000001;
    PORTC |= 0b00001111;
    DDRD |= 0b00000011; // PD0 and PD1 for debugging
    EICRA = (0 << ISC11) | (0 << ISC10) | (1 << ISC01) | (1 << ISC00);
    EIMSK = (0 << INT1) | (1 << INT0);
    DDRB = 0b11111111; // all D-pins as output
    // setting up TIMER0
    TCCR0A = (0 << COM0A1) | (0 << COM0A0) | (0 << COM0B1) | (0 << COM0B0) | (0 << WGM01) | (0 << WGM00);
    TCCR0B = (0 << WGM02) | (0 << CS02) | (1 << CS01) | (0 << CS00);
    TIMSK0 = (0 << OCIE0B) | (0 << OCIE0A) | (1 << TOIE0);
    sei();
}
void moveSnake(struct Snake **s)
{
    struct Snake *snake = *s;
    struct Part *pEnd = snake->end;
    if (s != NULL)
    {
        while (pEnd->prev != NULL)
        {
            pEnd->x = pEnd->prev->x;
            pEnd->y = pEnd->prev->y;
            if (pEnd->x == snake->head->x && pEnd->y == snake->head->y &&
                pEnd->prev->prev != NULL)
            {
                play = False;
                exit(1);
            }
            pEnd = pEnd->prev;
        }
    }
}
void Limit(struct Part **head)
{
    struct Part *tHead = *head;
    if (tHead->x >= XMaxLimit || tHead->x <= XMinLimit || tHead->y >= YMaxLimit || tHead->y <= YMinLimit)
    {
        play = False;
    }
}
int main(void)
{
    srand(time(NULL));
    while (1)
    {
        while (play)
        {
            if (newGame)
            {
                init();
                score = 0;
                snake =
                    newSnake(createNewPart(25, 5, 1, 0, 1, 1), createNewPart(24, 5, 1, 0, 1, 0)); // create
                a snake
                    addPartsSnake(&snake, snake->end); // add head and end
                for (int i = 0; i < 3; i++)
                {
                    addPartsSnake(&snake, createNewPart(snake->end->x, snake->end->y, snake->end->x d, snake->end->yd, snake->end->xv, snake->end->yv)); // add to the snake
                }
                food = generate_Food(); // create first apple
                NOKIA_setpixel(food->x, food->y);
                displaySnake(&snake->head);
                newGame = False;
            }
            // needs interrupt
            if (downPressed)
            { // down
                snake->head->yd = 1;
                snake->head->xd = 0;
                downPressed = False;
            }
            if (upPressed)
            { // up
                snake->head->yd = -1;
                snake->head->xd = 0;
                upPressed = False;
            }
            if (rightPressed)
            { // right
                snake->head->yd = 0;
                snake->head->xd = 1;
                rightPressed = False;
            }
            if (leftPressed)
            { // left
                snake->head->yd = 0;
                snake->head->xd = -1;
                leftPressed = False;
            }
            if (snake->head->x == food->x && snake->head->y == food->y)
            {                         //
                eats food.free(food); // remove food
                PORTD |= 0b00000010;  // make sound with buzzer.
                score += 100;
                addPartsSnake(&snake, createNewPart(snake->end->x, snake->end->y, snake->end->x d, snake->end->yd, snake->end->xv, snake->end->yv)); // add to the snake //grow
                snake
                    food = generate_Food(); // create new food
                int sameLocation = food_location(&snake->head, food);
                while (sameLocation)
                { // while food is at same location as
                    snake body part generate new food.food = generate_Food();
                    sameLocation = food_location(&snake->head, food);
                }
                NOKIA_setpixel(food->x, food->y);
            }
            display(score);
            clear_snake(&snake->head);
            snake->head->x += snake->head->xd * snake->head->xv;                      // update
            snake - head x - pos snake->head->y += snake->head->yd * snake->head->yv; // update
            snake - head y - pos moveSnake(&snake);
            displaySnake(&snake->head);
            _delay_ms(100);
            // PORTC = 0b00000000; // turn off the buzzer
            PORTD &= 0b11111101;
            NOKIA_update();
            Limit(&snake->head);
        }
        NOKIA_print(0, 0, "Game Over.", 0); // print message in Nokia
        framebuffer
            NOKIA_print(0, 8, "Press -> button", 0); // print message in Nokia
        framebuffer
            NOKIA_print(0, 16, "to play again.", 0); // print message in Nokia
        framebuffer
        NOKIA_update();
        if (!(PINC & 0b00000010)) // right
            play = True;
        newGame = True;
        NOKIA_clear();
    }
}

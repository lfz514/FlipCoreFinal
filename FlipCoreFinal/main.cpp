#include "mbed.h"
#include "SegDisp/QuadSevenSegment.h"

//pin definitions
DigitalOut rs(PA_9);
DigitalOut en(PC_7);
DigitalOut d4(PB_0);
DigitalOut d5(PA_7);
DigitalOut d6(PA_6);
DigitalOut d7(PA_5);

///button definitions
DigitalIn btnJump(PB_14, PullUp);  //grav button
DigitalIn btnStart(PA_8, PullUp);  // start/restart button

//custom ascii chars 
const char floor_char[8]      = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F};
const char roof_char[8]       = {0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const char run1_char[8]       = {0x0E,0x0E,0x04,0x0E,0x15,0x0E,0x12,0x1F};
const char run2_char[8]       = {0x0E,0x0E,0x04,0x0E,0x15,0x0E,0x09,0x1F};
const char run1_inv[8]        = {0x1F,0x12,0x0E,0x15,0x0E,0x04,0x0E,0x0E};
const char run2_inv[8]        = {0x1F,0x09,0x0E,0x15,0x0E,0x04,0x0E,0x0E};
const char obstacle_char[8]   = {0x04,0x04,0x04,0x05,0x15,0x0E,0x04,0x04};
const char stalactite_char[8] = {0x1F,0x1F,0x1F,0x0E,0x0E,0x04,0x04,0x00};

bool GravInvert = false;
const int playerPos = 3; //defines  player pos
Timer gameTimer; //time played variable 
volatile int score = 0; //shared  between thredas 

Thread segThread;

void segDisplay()  //updates 7 seg score module
{
    while (true) 
    {
        ScoreVal(score);
    }
}


void commandSend(uint8_t cmd) ///sends 8 bit cmd in 2 nibbles, high then low

{
    rs = 0;
    d4 = (cmd >> 4) & 1;//high
    d5 = (cmd >> 5) & 1;
    d6 = (cmd >> 6) & 1;
     d7 = (cmd >> 7) & 1;
    en = 1; wait_us(1);
    en = 0; wait_us(50);//latch
    d4 = cmd & 1; //low
    d5 = (cmd >> 1) & 1;
    d6 = (cmd >> 2) & 1; 
    d7 = (cmd >> 3) & 1;
    en = 1; wait_us(1); 
    en = 0; wait_us(50);
}

void sendData(char data) {  //sends 8 bit data to LCD in 2 nibbles,  high then low
    rs = 1;
    d4 = (data >> 4) & 1; //high
    d5 = (data >> 5) & 1;
    d6 = (data >> 6) & 1; 
    d7 = (data >> 7) & 1;
    en = 1; wait_us(1); //latch
    en = 0; wait_us(50);
    d4 = data & 1;  //low
    d5 = (data >> 1) & 1;
    d6 = (data >> 2) & 1; 
    d7 = (data >> 3) & 1;
    en = 1; wait_us(1); 
    en = 0; wait_us(50);
}

void createChar(uint8_t location, const char charmap[]) 
{
    commandSend(0x40 + (location * 8));//char gen address (0x40) to write new custom char
    for (int i = 0; i < 8; i++) //write row of custom char
    {
        sendData(charmap[i]);
    }
    commandSend(0x80);//return to normal memory mode
}

void setCursor(int col, int row) 
{
    commandSend((row == 0) ? 0x80 + col : 0xC0 + col);//if row = 0, cursor to column col top row, if row = 1, cursor to column col at bottom row
}//0x80 sets DDRAM addr to row 0, 0xC0 to row 1

void clearScreen() 
{
    commandSend(0x01);//set cursor to 0,0 and clear screen
    wait_us(2000);
}

void initLCD() 
{
    wait_us(50000); //wait 50ms for power on sequence as per datasheet
    rs = 0;//cmd mode
    d4 = 1; d5 = 1; d6 = 0; d7 = 0; // high nibble of 0x30 (reads 0x3), to set 8 bit mode first, standard procedure to send 3x to be sure system is stable before setting 4bit mode
    en = 1; wait_us(1); en = 0; wait_us(5000);
    en = 1; wait_us(1); en = 0; wait_us(5000);
    en = 1; wait_us(1); en = 0; wait_us(150);
    d4 = 0; d5 = 1; d6 = 0; d7 = 0; //send upper nibble of 0x20 (0x2) to enter 4 bit mode
    en = 1; wait_us(1); en = 0; wait_us(150); //latch

    commandSend(0x28);//2 lines, 4 bit, 
    commandSend(0x0C); //display on, cursor and blink off
    commandSend(0x06);//increment cursor, no shift
    commandSend(0x01);//clear, return to 0,0
    wait_us(2000);//wait 2ms (>1.6ms required)
    //set char gen address, write the bitmap data to create custom char, load into CGRAM locations 0-7  
    createChar(0, floor_char);
    createChar(1, roof_char);
    createChar(2, run1_char);
    createChar(3, run2_char);
    createChar(4, run1_inv);
    createChar(5, run2_inv);
    createChar(6, obstacle_char);
    createChar(7, stalactite_char);
}

void waitStart() 
{
    clearScreen();//call clear function, sets home loc
    setCursor(0, 0);//home location for cursor manual - trust issues
    const char* msg1 = "READY";//store READY in msg1, with term char at end of array due to '""'
    const char* msg2 = "PRESS START";//store PRESS START in msg2, term char here too
    for (int i = 0; msg1[i]; i++) sendData(msg1[i]);//iterate through msg1 until reach term character
    setCursor(0, 1);// next row
    for (int i = 0; msg2[i]; i++) sendData(msg2[i]); //iterate through msg2

    wait_us(2500000);
    clearScreen();//clear again
}

void gameLoop() 
{
    gameTimer.reset();//reset timer
    gameTimer.start();//start timer
    score = 0;//reset score
    float speedFactor = 1.0f;//score multiplier set
    int frame = 0;//var for frame number
    GravInvert = false; //start on floor

    //obstacle setup
    const int maxObs = 4;//avoid overcrowding, max positions for obstacles = 4 
    int cactusPos[maxObs] = {-1, -1, -1, -1};//array for horisontal cactus positions, initialise all positions to -1 (off screen, innactive)
    int stalacPos[maxObs] = {-1, -1, -1, -1};///same for stalac
    int cacspawnDelay = 10 + (rand() % 10);//random spawn delay
    int stalspawnDelay = 10 + (rand() % 10);
    //initialise and start score timer
    Timer scoreTimer;
    scoreTimer.start();
    //main game loop, runs until game over
    while (true) {
        bool buttonCurrent = btnJump.read(); //reads current state and stores last state of button, unpressed = 1
        static bool last_btn = 1; 
        if (last_btn == 1 && buttonCurrent == 0) GravInvert = !GravInvert;//if falling edge, gravity invert
        last_btn = buttonCurrent;//update button state 

        //depending on gravity inversion state, select which char to use (run and if on floor or ceiling)
        char player_char = (frame % 2 == 0) ? (GravInvert ? 4 : 2) : (GravInvert ? 5 : 3); 

        for (int i = 0; i < maxObs; i++) //create scroll effect
        {
            if (cactusPos[i] >= 0) cactusPos[i]--;
            if (stalacPos[i] >= 0) stalacPos[i]--;
        }

        if (cacspawnDelay-- <= 0) //only attempt spawn if spawn delay is less or equal 0
        {
            for (int i = 0; i < maxObs; i++)  //loop through max obst
            {
                if (cactusPos[i] < 0) //check if slot is innactive
                {
                    int pos = 16;//spawn at 16, just off screen (pos 0-15)
                    bool ok = true;//bool flag, true = safe to spawn, to prevent accidental impossible or stacked spawning, false = occupied
                    for (int j = 0; j < maxObs; j++)//loop through stalac pos options
                    if (stalacPos[j] == pos || stalacPos[j] == pos - 1)//check for issues spawning
                    ok = false;//flip to unsafe if any conflicts
                    if (ok) cactusPos[i] = pos; //if pass collision test, can spawn at 16
                    cacspawnDelay = 10 + (rand() % 10);//reset spawn timer
                    break;//after spawn, exit loop
                }
            }
        }

        if (stalspawnDelay-- <= 0) //same logic appplies to stalac
        {
            for (int i = 0; i < maxObs; i++) 
            {
                if (stalacPos[i] < 0) 
                {
                    int pos = 16;
                    bool ok = true;
                    for (int j = 0; j < maxObs; j++)
                    if (cactusPos[j] == pos || cactusPos[j] == pos - 1)
                    ok = false;
                    if (ok) stalacPos[i] = pos;
                    stalspawnDelay = 10 + (rand() % 10);
                    break;
                }
            }
        }

        for (int i = 0; i < maxObs; i++) //loop through all obst slots
        {
            //if on floor, and player char is inside cactus pos, or on roof and player pos = stal pos
            if ((!GravInvert && playerPos == cactusPos[i]) || (GravInvert && playerPos == stalacPos[i])) 
            {
                clearScreen();
                setCursor(4, 0);//move the cursor and print GAME OVER
                const char* msg = "Game Over";
                for (int i = 0; msg[i]; i++) sendData(msg[i]);

                setCursor(0, 1);// move to bottom row, first col
                char buffer[17]; //assign buffer for score display, 16 + room for term character
                snprintf(buffer, 17, "Score: %d", score); //format string to buffer 
                for (int i = 0; buffer[i]; i++) sendData(buffer[i]);//iterate through to display the string

                wait_us(2500000);  // wait 2.5s
                clearScreen();//clear the screen
                return;//exit game loop
            }
        }

        setCursor(0, 0);//home pos cursor set
        for (int i = 0; i < 16; i++) {//search top row
            bool drawn = false; //flag for if something is drawn in this column
            for (int j = 0; j < maxObs; j++) //check each stalac
            {
                if (stalacPos[j] == i) //if stal is in current col
                {
                    sendData(7);//draw stal (custom char number 7)
                    drawn = true;//drawn flag true
                    break;//exit loop
                }
            }
            if (!drawn) sendData((GravInvert && i == playerPos) ? player_char : 1);//if nothing is drawn, and this col = player pos, spawn player, otherwise roof char
        }

        setCursor(0, 1);//second row
        for (int i = 0; i < 16; i++) //scan each col
        {
            bool drawn = false;//reset drawn flagg
            for (int j = 0; j < maxObs; j++) //search bottom row
            
            {
                if (cactusPos[j] == i) //if cactus is on current col
                {
                    sendData(6);//draw a cactus
                    drawn = true;// flag update
                    break;//exit loop
                }
            }
            if (!drawn) sendData((!GravInvert && i == playerPos) ? player_char : 0);//if nothing is spawned here, on floor, and player pos = current col, spawn player, otherwise draw floor
        }

        // Score logic
        if (scoreTimer.read() >= 0.1f / speedFactor) //scan if enough time has passed to increase score value, this shrinks over time to speed up score
        {
            score += 1;//add 1
            speedFactor += 0.01f;//increase score mult by small amount, float not double
            scoreTimer.reset();//reset for next check
        }

        frame++;
        wait_us(150000 / speedFactor);
    }
}


int main() 
{
    initLCD(); //init LCD
    segThread.start(segDisplay);  //start the updater

    while (true) 
    {
        waitStart();//wait for start
        gameLoop();//run game
    }
}

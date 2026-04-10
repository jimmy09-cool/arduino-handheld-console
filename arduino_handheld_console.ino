#include <LedControl.h>
#include <EEPROM.h>

LedControl lc = LedControl(51, 52, 53, 1);

// Controls
#define JOY_X A0
#define JOY_Y A1
#define JOY_BTN 22
#define BUZZER 8

// State
enum State { MENU, SNAKE, PONG, DODGE };
State currentState = MENU;

int menuIndex = 0;
unsigned long lastMoveTime = 0;
unsigned long lastUpdate = 0;
bool blinkState = false;

int score = 0;
int highScore = 0;

// ---------- FONT ----------
byte font[][5] = {
  {0x7E,0x11,0x11,0x11,0x7E},
  {0x7F,0x49,0x49,0x49,0x41},
  {0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,0x08,0x14,0x22,0x41},
  {0x7F,0x02,0x04,0x02,0x7F},
  {0x7F,0x10,0x08,0x04,0x7F},
  {0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},
  {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31},
  {0x7F,0x40,0x20,0x10,0x0F}
};

// ---------- SOUND ----------
void beep(int f,int d){ tone(BUZZER,f,d); }
void clickSound(){ beep(1000,40); }
void eatSound(){ beep(1500,70); delay(70); beep(2000,70); }
void bounceSound(){ beep(800,40); }
void startSound(){ beep(1000,80); delay(80); beep(1400,80); }
void exitSound(){ beep(500,100); }
void gameOverSound(){ beep(400,200); delay(200); beep(200,300); }

// ---------- SNAKE ----------
int snakeX[64], snakeY[64];
int snakeLength, dirX, dirY, foodX, foodY;

// ---------- PONG ----------
int paddleY, ballX, ballY, ballDX, ballDY;

// ---------- DODGE ----------
int playerX;
int obstacleX[8];
int obstacleY[8];
int obstacleCount;

// ---------- SETUP ----------
void setup() {
  pinMode(JOY_BTN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  lc.shutdown(0,false);
  lc.setIntensity(0,8);
  lc.clearDisplay(0);

  randomSeed(analogRead(0));

  highScore = EEPROM.read(0);
  if(highScore > 200) highScore = 0;

  bootAnim();
}

// ---------- LOOP ----------
void loop(){
  switch(currentState){
    case MENU: runMenu(); break;
    case SNAKE: runSnake(); break;
    case PONG: runPong(); break;
    case DODGE: runDodge(); break;
  }
}

// ---------- BOOT ----------
void bootAnim(){
  for(int i=0;i<8;i++){ lc.setRow(0,i,255); delay(40); }
  lc.clearDisplay(0);
}

// ---------- SCROLL ----------
void scrollText(int letters[], int len){
  for(int s=0;s<len*6+8;s++){
    lc.clearDisplay(0);
    for(int i=0;i<len;i++){
      for(int c=0;c<5;c++){
        int x=8+(i*6+c)-s;
        if(x>=0&&x<8){
          byte col=font[letters[i]][c];
          for(int r=0;r<7;r++){
            lc.setLed(0,r,x,col&(1<<r));
          }
        }
      }
    }
    delay(60);
  }
}

// ---------- SCORE ----------
void showScore(){
  for(int i=0;i<score;i++){
    lc.clearDisplay(0);
    lc.setLed(0,3,3,true);
    delay(120);
    lc.clearDisplay(0);
    delay(120);
  }
}

void saveHighScore(){
  if(score > highScore){
    highScore = score;
    EEPROM.update(0, highScore);
  }
}

// ---------- GAME OVER ----------
void gameOverScreen(){
  gameOverSound();
  saveHighScore();

  int go[] = {2,0,4,1,6,10,1,8};
  scrollText(go,8);

  delay(200);
  showScore();

  lc.clearDisplay(0);
  for(int i=0;i<highScore && i<8;i++){
    lc.setLed(0,0,i,true);
    delay(80);
  }

  delay(400);
  currentState = MENU;
}

// ---------- MENU ----------
void runMenu(){
  int y=analogRead(JOY_Y);

  if(millis()-lastMoveTime>200){
    if(y<400){menuIndex--;clickSound();lastMoveTime=millis();}
    if(y>600){menuIndex++;clickSound();lastMoveTime=millis();}
  }

  if(menuIndex<0)menuIndex=2;
  if(menuIndex>2)menuIndex=0;

  if(digitalRead(JOY_BTN)==LOW){
    delay(200);

    if(menuIndex==0){
      int w[]={9,5,0,3,1};
      scrollText(w,5);
      startSound();
      startSnake();
    }

    if(menuIndex==1){
      int w[]={7,6,5,2};
      scrollText(w,4);
      startSound();
      startPong();
    }

    if(menuIndex==2){
      int w[]={3,6,2,2,1};
      scrollText(w,5);
      startSound();
      startDodge();
    }
  }

  blinkState=(millis()%500<250);
  drawMenu();
}

// ---------- MENU DRAW (UNCHANGED STYLE) ----------
void drawMenu(){
  lc.clearDisplay(0);

  if(menuIndex==0){
    int off=(millis()/200)%4;
    for(int i=0;i<4;i++){
      if(!blinkState&&i==3)continue;
      lc.setLed(0,3,2+((i+off)%4),true);
    }
  } 
  else if(menuIndex==1){
    int b=(millis()/200)%6+1;
    for(int i=0;i<3;i++){
      if(!blinkState&&i==2)continue;
      lc.setLed(0,2+i,1,true);
    }
    lc.setLed(0,b,5,true);
  }
  else {
    // simple dodge icon (falling pixel)
    int y = (millis()/200)%8;
    lc.setLed(0,y,4,true);
  }
}

// ---------- SNAKE ----------
void startSnake(){
  currentState=SNAKE;
  score=0;

  snakeLength=3;
  snakeX[0]=3;snakeY[0]=4;
  snakeX[1]=2;snakeY[1]=4;
  snakeX[2]=1;snakeY[2]=4;

  dirX=1;dirY=0;
  spawnFood();
}

void runSnake(){
  int x=analogRead(JOY_X), y=analogRead(JOY_Y);

  if(x<400&&dirX!=1){dirX=-1;dirY=0;}
  if(x>600&&dirX!=-1){dirX=1;dirY=0;}
  if(y<400&&dirY!=1){dirX=0;dirY=-1;}
  if(y>600&&dirY!=-1){dirX=0;dirY=1;}

  if(millis()-lastUpdate>250){
    moveSnake();
    lastUpdate=millis();
  }

  drawSnake();

  if(digitalRead(JOY_BTN)==LOW){
    exitSound();
    delay(300);
    currentState=MENU;
  }
}

void moveSnake(){
  for(int i=snakeLength;i>0;i--){
    snakeX[i]=snakeX[i-1];
    snakeY[i]=snakeY[i-1];
  }

  snakeX[0]+=dirX;
  snakeY[0]+=dirY;

  if(snakeX[0]<0)snakeX[0]=7;
  if(snakeX[0]>7)snakeX[0]=0;
  if(snakeY[0]<0)snakeY[0]=7;
  if(snakeY[0]>7)snakeY[0]=0;

  if(snakeX[0]==foodX&&snakeY[0]==foodY){
    snakeLength++;
    score++;
    eatSound();
    spawnFood();
  }

  for(int i=1;i<snakeLength;i++){
    if(snakeX[0]==snakeX[i]&&snakeY[0]==snakeY[i]){
      gameOverScreen();
    }
  }
}

void spawnFood(){
  foodX=random(0,8);
  foodY=random(0,8);
}

void drawSnake(){
  lc.clearDisplay(0);
  for(int i=0;i<snakeLength;i++){
    lc.setLed(0,snakeY[i],snakeX[i],true);
  }
  lc.setLed(0,foodY,foodX,true);

  for(int i=0;i<score && i<8;i++){
    lc.setLed(0,0,i,true);
  }
}

// ---------- PONG ----------
void startPong(){
  currentState=PONG;
  score=0;

  paddleY=3;
  ballX=4;ballY=4;
  ballDX=1;ballDY=1;
}

void runPong(){
  int y=analogRead(JOY_Y);
  if(y<400)paddleY--;
  if(y>600)paddleY++;
  paddleY=constrain(paddleY,0,5);

  if(millis()-lastUpdate>200){
    ballX+=ballDX;
    ballY+=ballDY;

    if(ballY<=0||ballY>=7){
      ballDY*=-1;
      bounceSound();
    }

    if(ballX==1&&ballY>=paddleY&&ballY<=paddleY+2){
      ballDX=1;
      score++;
      bounceSound();
    }

    if(ballX<=0){
      gameOverScreen();
      return;
    }

    if(ballX>=7)ballDX=-1;

    lastUpdate=millis();
  }

  drawPong();

  if(digitalRead(JOY_BTN)==LOW){
    exitSound();
    delay(300);
    currentState=MENU;
  }
}

void drawPong(){
  lc.clearDisplay(0);

  for(int i=0;i<3;i++){
    lc.setLed(0,paddleY+i,0,true);
  }

  lc.setLed(0,ballY,ballX,true);

  for(int i=0;i<score && i<8;i++){
    lc.setLed(0,0,i,true);
  }
}

// ---------- DODGE ----------
void startDodge(){
  currentState=DODGE;
  score=0;

  playerX=3;
  obstacleCount=3;

  for(int i=0;i<obstacleCount;i++){
    obstacleX[i]=random(0,8);
    obstacleY[i]=random(-7,0);
  }
}

void runDodge(){
  int x=analogRead(JOY_X);

  if(x<400) playerX--;
  if(x>600) playerX++;

  playerX=constrain(playerX,0,7);

  if(millis()-lastUpdate>200){
    for(int i=0;i<obstacleCount;i++){
      obstacleY[i]++;

      if(obstacleY[i]>7){
        obstacleY[i]=0;
        obstacleX[i]=random(0,8);
        score++;
      }

      if(obstacleY[i]==7 && obstacleX[i]==playerX){
        gameOverScreen();
        return;
      }
    }
    lastUpdate=millis();
  }

  drawDodge();

  if(digitalRead(JOY_BTN)==LOW){
    exitSound();
    delay(300);
    currentState=MENU;
  }
}

void drawDodge(){
  lc.clearDisplay(0);

  lc.setLed(0,7,playerX,true);

  for(int i=0;i<obstacleCount;i++){
    if(obstacleY[i]>=0 && obstacleY[i]<8){
      lc.setLed(0,obstacleY[i],obstacleX[i],true);
    }
  }

  for(int i=0;i<score && i<8;i++){
    lc.setLed(0,0,i,true);
  }
}

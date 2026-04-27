/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         : main.c
  * @brief          :Carnival of Crystals RPG - CPE 2200 Term Project
  ******************************************************************************
  * @attention
  *
  * This game presents the user with a math question. It is played over the serial interface.
  * The user can reset the game with the on-board reset button of the NUCLEO board.
  * Baud Rate = 115200
  *
  * Copyright (c) 2023 STMicroelectronics.
  * Copyright (c) 2023 Dr. Billy Kihei, for CPE 2200
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dialogue.h"
#include "stm32_init.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PD */
#define SCORE_FLASH_ADDR 0x0807F000
/* USER CODE END PD */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    char name[20];
    uint16_t points;
} ScoreEntry;

typedef enum {
    STATE_TITLE, STATE_SCORES, STATE_DIFFICULTY, STATE_CHAR_SELECT,
    STATE_INTRO, STATE_HUB, STATE_LAZER_TAG, STATE_FASHION,
    STATE_PIE_CONTEST, STATE_COASTER_CRASH, STATE_BASEMENT,
    STATE_BEDROOM, STATE_INFIRMARY, STATE_FINAL_BOSS, STATE_END, STATE_ENGINEROOM, STATE_SAVE_SCORE,
	STATE_POST_GAME, STATE_QUIT
} GameState_t;
/* USER CODE END PTD */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;
GameState_t currentState = STATE_TITLE;
volatile uint8_t dataAvail = 0;
char rx_buff[1];
char name_temp[20];
uint8_t name_index = 0;
static uint8_t inputLocked = 0;
volatile uint32_t rBuffer = 0;


uint16_t totalPoints = 0;
uint8_t selectedDifficulty = 0;
uint8_t inventory[2] = {0, 0};
ScoreEntry highScores[5];
int scoreCount = 0;
uint8_t playerChar = 0;
uint8_t fashionPoints = 0;



char maze_map[100] = "Visited: Start";

/* USER CODE BEGIN PFP */
extern int assembly_mash_counter(int timer_val);
void UART_Print(const char* str);
void SortScores(void);
void UART_Printf(const char* fmt, ...);


/* USER CODE END PFP */

int main(void)
{
	char mathPrompt[128];
   static uint8_t needsNewQuestion = 1;
	    static uint8_t lazerStep = 1;
	    static uint16_t lazerPoints = 0;
	    static uint8_t fashionStep = 0;
	    static uint8_t chosenHat = 0;
	    static uint8_t fashionScore = 0;
	    static uint8_t pieStep = 0;
	    static uint32_t startTime = 0;
	    static int correctAns = 0;
	    static int questionsSolved = 0;
	    static char ops_easy[] = {'+', '-'};
	    static char ops_hard[] = {'+', '-', '*', '/'};
	    static char pieAnsBuf[12];
	    static uint8_t pieAnsIdx = 0;
	    static uint8_t battleScore = 0;
	    static uint8_t results = 0;
        static uint8_t checkedBed = 0;
        static uint8_t checkedDesk = 0;
        static uint8_t checkedShelf = 0;
    	static uint8_t battlePoints = 0;
    	static uint8_t infirmaryCleared = 0;
    	static uint32_t stateLockTime = 0;
    	static uint8_t bossStep = 0;
    	static uint8_t prevBossStep = 225;






	HAL_Init();
	uint32_t seed = 0;
	seed ^= HAL_GetTick();
	seed ^= *(volatile uint32_t*)0x1FFF7A10; // unique STM32 device ID (low word)
	seed ^= *(volatile uint32_t*)0x1FFF7A14; // device ID (mid word)

	srand(seed);
	SystemClock_Config();
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();

	memcpy(highScores, (void*)SCORE_FLASH_ADDR, sizeof(highScores));

	    if (highScores[0].points == 0xFFFF) {
	        memset(highScores, 0, sizeof(highScores));
	        scoreCount = 0;
	    } else {

	        scoreCount = 0;
	        for(int i=0; i<5; i++) {
	            if(highScores[i].points != 0) scoreCount++;
	        }
	    }

	HAL_UART_Receive_IT(&huart2, (uint8_t*)rx_buff, 1);
	UART_Print("\033[2J\033[H");



  while (1)
  {
	  if (inputLocked && (HAL_GetTick() - stateLockTime >= 3000)) {
	      inputLocked = 0;
	      currentState = STATE_COASTER_CRASH;
	  }

	static GameState_t prevState = (GameState_t)255;
	uint8_t stateChanged = (currentState != prevState);
	prevState = currentState;

	static uint8_t prevFashionStep = 255;
	static uint8_t prevPieStep = 255;

    switch(currentState) {
        case STATE_TITLE:
        	if (stateChanged) {
        		UART_Print(TitleArt);
        		UART_Print("\r\nMAIN MENU\r\n-1. Start Game\r\n-2. Past Scores\r\n-3. Quit\r\n");
        	}
            if(dataAvail) {
                if(rx_buff[0] == '1') currentState = STATE_DIFFICULTY;
                else if(rx_buff[0] == '2') currentState = STATE_SCORES;
                else if(rx_buff[0] == '3') currentState = STATE_QUIT;
                dataAvail = 0;
            }
            break;

        case STATE_SCORES:
        	if (stateChanged) {
        		UART_Print("\r\n=== PAST SCORES ===\r\n");
        		SortScores();
        		for(int i=0; i<scoreCount; i++) {
        			char scoreLine[50];
        			sprintf(scoreLine, "%d pts..... %s\r\n", highScores[i].points, highScores[i].name);
        			UART_Print(scoreLine);
        		}
        		if(scoreCount == 0) UART_Print("(No scores recorded yet)\r\n");
        		UART_Print("\r\n-1. Clear Scores | -2. Back to Main Menu\r\n");
        	}
        	if (dataAvail) {

        	    char c = rx_buff[0];
        	    dataAvail = 0;

        	    if (c == '2') {
        	        currentState = STATE_TITLE;
        	    }

        	    else if (c == '1') {
        	        memset(highScores, 0, sizeof(highScores));
        	        scoreCount = 0;
        	        saveScoresToFlash();
        	        UART_Print("Scores Cleared!\r\n");
        	    }
        	}
            break;

        case STATE_DIFFICULTY:
        	if (stateChanged) {
        		UART_Print("\r\nChoose your Difficulty:\r\n-1. Easy\r\n-2. Hard\r\n");
        	}
            if(dataAvail) {
                selectedDifficulty = (rx_buff[0] == '2') ? 1 : 0;
                currentState = STATE_CHAR_SELECT;
                dataAvail = 0;
            }
            break;

        case STATE_CHAR_SELECT:
        	if (stateChanged) {
        		UART_Print("\r\nChoose your Character:\r\n");
        		UART_Print("-1. Drical: A half giant, half dragonborn sorcerer, coming from royalty with a fashion sense to prove how blue that blood is.\r\n");
        		UART_Print("-2. Sotlas: A snake-like rouge, loyal to his loved ones, but is scarce to show how much he cares.\r\n");
        		UART_Print("-3. Noel: A young, tech savvy elf, chosen by a goddess to save the home he’s come to love using his warlock powers. A Champion.\r\n");
        	}

            if(dataAvail) {
                    if(rx_buff[0] == '1') playerChar = 1;
                    else if(rx_buff[0] == '2') playerChar = 2;
                    else if(rx_buff[0] == '3') playerChar = 3;
                    currentState = STATE_INTRO;
                    dataAvail = 0;
            }
            break;

        case STATE_INTRO:
        	if (stateChanged) {
        		UART_Print(IntroParagraph);
        		currentState = STATE_HUB;
        	}
            break;



        case STATE_HUB:
        	if (stateChanged) {
        		UART_Print(CarnivalArt);
        		UART_Print("\r\nWhere would you like to head to?\r\n-1. Lazer Tag\r\n-2. Fashion Contest\r\n-3. Let's look somewhere else [no return]\r\n");
        	}
            if(dataAvail) {
                if(rx_buff[0] == '1') currentState = STATE_LAZER_TAG;
                if(rx_buff[0] == '2') currentState = STATE_FASHION;
                if(rx_buff[0] == '3') {currentState = STATE_PIE_CONTEST;
                pieStep = 0;
                needsNewQuestion = 1;}
                dataAvail = 0;

            }
            break;

        case STATE_LAZER_TAG:
                    if (stateChanged) {
                        UART_Print("\r\nYou get strapped into a harness and given a laser gun, before stepping into the maze-like arena.\r\n");
                        lazerStep = 1;
                        lazerPoints = 0;
                        memset(maze_map, 0, sizeof(maze_map));
                        strcpy(maze_map, "Visited: Start");
                        UART_Print("\r\nInfront of you, there are three options: left, forward, and right. Which way do you go?\r\n");
                        UART_Print("1.left | 2.forward | 3.right | 4.check map | 5.give up\r\n\r\n");
                    }

                    if (dataAvail) {
                        char move = rx_buff[0];
                        dataAvail = 0;

                        if (move == '5') {
                            UART_Print("\r\nYou can’t help but surrender, calling out for a carnie to come assist you out. You earned 0 points…\r\n");
                            lazerPoints = 0;
                            lazerStep = 1;
                            currentState = STATE_HUB;
                        }
                        else if (move == '4') {
                            UART_Print("\r\nMAP: ");
                            UART_Print(maze_map);
                            UART_Print("\r\n1.left | 2.forward | 3.right | 4.check map | 5.give up\r\n\r\n");
                        }
                        else if (move == '1' || move == '2' || move == '3') {

                            if (strlen(maze_map) > 85) {
                                strcpy(maze_map, "Visited: ...rest of map too long...");
                            }


                            if (selectedDifficulty == 0) {
                                if (lazerStep == 1) {
                                    if (move == '2') {
                                        UART_Print("\r\nYou pass someone as you barrel forward, managing to hit their target! Seems like the right way. \r\n");
                                        lazerPoints++; lazerStep++; strcat(maze_map, " -> F(O)");
                                    } else if (move == '1') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> L(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> R(X)");
                                    }
                                }
                                else if (lazerStep == 2) {
                                    if (move == '3') {
                                        UART_Print("\r\nYou pass someone as you barrel forward, managing to hit their target! Seems like the right way. \r\n");
                                        lazerPoints++; lazerStep++; strcat(maze_map, " -> R(O)");
                                    } else if (move == '1') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> L(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> F(X)");
                                    }
                                }
                                else if (lazerStep == 3) {
                                    if (move == '3') {
                                    	lazerPoints++;
                                        UART_Printf("\r\nYou made it out, earning %d points!\r\n", lazerPoints);
                                        totalPoints += lazerPoints; lazerStep = 1; currentState = STATE_HUB;
                                    } else if (move == '2') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> F(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> L(X)");
                                    }
                                }
                            }


                            else if (selectedDifficulty == 1) {
                                if (lazerStep == 1) {
                                    if (move == '1') {
                                        UART_Print("\r\nYou pass someone as you barrel forward, managing to hit their target! Seems like the right way. \r\n");
                                        lazerPoints++; lazerStep++; strcat(maze_map, " -> L(O)");
                                    } else if (move == '2') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> F(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> R(X)");
                                    }
                                }
                                else if (lazerStep == 2) {
                                    if (move == '2') {
                                        UART_Print("\r\nYou pass someone as you barrel forward, managing to hit their target! Seems like the right way. \r\n");
                                        lazerPoints++; lazerStep++; strcat(maze_map, " -> F(O)");
                                    } else if (move == '3') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> R(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> L(X)");
                                    }
                                }
                                else if (lazerStep == 3) {
                                    if (move == '1') {
                                        UART_Print("\r\nYou pass someone as you barrel forward, managing to hit their target! Seems like the right way. \r\n");
                                        lazerPoints++; lazerStep++; strcat(maze_map, " -> L(O)");
                                    } else if (move == '2') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> F(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> R(X)");
                                    }
                                }
                                else if (lazerStep == 4) {
                                    if (move == '3') {
                                        UART_Print("\r\nYou pass someone as you barrel forward, managing to hit their target! Seems like the right way. \r\n");
                                        lazerPoints++; lazerStep++; strcat(maze_map, " -> R(O)");
                                    } else if (move == '2') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> F(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> L(X)");
                                    }
                                }
                                else if (lazerStep == 5) {
                                    if (move == '2') {
                                    	lazerPoints++;
                                        UART_Printf("\r\nYou made it out, earning %d points!\r\n", lazerPoints);
                                        totalPoints += lazerPoints; lazerStep = 1; currentState = STATE_HUB;
                                    } else if (move == '3') {
                                        UART_Print("\r\nYou were sure of your answer, until you’re ambushed by the enemy! They shoot you, and you’re forced to retreat.\r\n");
                                        if (lazerPoints > 0) lazerPoints--; strcat(maze_map, " -> R(X)");
                                    } else {
                                        UART_Print("\r\nYou find yourself at a dead end and make your way back.\r\n");
                                        strcat(maze_map, " -> L(X)");
                                    }
                                }
                            }


                            if (currentState == STATE_LAZER_TAG) {
                                UART_Print("\r\nWhere would you like to go next?\r\n");
                                UART_Print("1.left | 2.forward | 3.right | 4.check map | 5.give up\r\n");
                            }
                        }
                    }
                    break;


        case STATE_FASHION:
                    if (stateChanged) {
                        UART_Print("\r\nYou cant fight off your curiosity as you make your way to the dress up stage, writing your name on the contestant list, before being escorted to a changing tent.\r\nTry to match the best outfit, from accessories to pants!\r\n");
                        fashionScore = 0;
                        fashionStep = 0;
                        prevFashionStep = 255;
                    }

                    switch(fashionStep) {
                        case 0:
                            if (fashionStep != prevFashionStep) {
                                if (selectedDifficulty == 0) {
                                    UART_Print("\r\nYour choices for hats: (no points given this round, free)\r\n1. Top Hat\r\n2. Cap 'n' Bells\r\n3. Tiara\r\n");
                                } else {
                                    UART_Print("\r\nTry to match the style of your character!\r\n");
                                    UART_Print("1. A crown fit for royalty.\r\n2. A hood to keep your features a mystery.\r\n3. Headphones to listen to your favorite music.\r\n");
                                }
                                prevFashionStep = fashionStep;
                            }

                            if(dataAvail) {
                                chosenHat = rx_buff[0];
                                if(selectedDifficulty == 1 && chosenHat == (playerChar + '0')) fashionScore++;
                                fashionStep = 1;
                                dataAvail = 0;
                            }
                            break;

                        case 1:
                            if (fashionStep != prevFashionStep) {
                                if (selectedDifficulty == 0) {
                                    UART_Print("\r\nNow, match the top: (+1 point for matching the hat)\r\n1. Waistcoat\r\n2. Fitted Bodice\r\n3. Motley Tunic\r\n");
                                } else {
                                    UART_Print("\r\nMatch the top: \r\n1. A light, breathable tunic for your scales.\r\n2. A corset to really exaggerate your form.\r\n3. A tank top, showing off all your markings of a Champion.\r\n");
                                }
                                prevFashionStep = fashionStep;
                            }

                            if(dataAvail) {
                                char move = rx_buff[0];
                                if(selectedDifficulty == 0) {

                                    if(chosenHat == '1' && move == '1') fashionScore++;
                                    else if(chosenHat == '3' && move == '2') fashionScore++;
                                    else if(chosenHat == '2' && move == '3') fashionScore++;
                                } else {
                                    char expectedTop = (playerChar == 1) ? '2' : (playerChar == 2) ? '1' : '3';
                                    if(move == expectedTop) fashionScore++;
                                }
                                fashionStep = 2;
                                dataAvail = 0;
                            }
                            break;

                        case 2:
                            if (fashionStep != prevFashionStep) {
                                if (selectedDifficulty == 0) {
                                    UART_Print("\r\nAnd last, the bottoms: (+1 point for matching the hat)\r\n1. Particolored Breeches\r\n2. Full-A-line Skirt\r\n3. Trousers\r\n");
                                } else {
                                    UART_Print("\r\nMatch the bottoms: \r\n1. Cargo pants with large pockets to fit all your tech.\r\n2. jeans with patches all over from when your mom repaired them.\r\n3. Dress pants long enough to fit you.\r\n");
                                }
                                prevFashionStep = fashionStep;
                            }

                            if(dataAvail) {
                                char move = rx_buff[0];
                                if(selectedDifficulty == 0) {

                                    if (chosenHat == '2' && move == '1') fashionScore++;
                                    else if (chosenHat == '3' && move == '2') fashionScore++;
                                    else if (chosenHat == '1' && move == '3') fashionScore++;
                                } else {
                                    char expectedBottom = (playerChar == 1) ? '3' : (playerChar == 2) ? '2' : '1';
                                    if(move == expectedBottom) fashionScore++;
                                }
                                fashionStep = 3;
                                dataAvail = 0;
                            }
                            break;

                        case 3:
                            if (fashionStep != prevFashionStep) {
                                uint16_t earned = 0;
                                if (fashionScore == 0) {
                                    UART_Print("\r\nAs you stand in front of the judges, they give you strange looks at your attire. Seems like you didn’t impress, but they give you a point for trying.\r\n");
                                    earned = 1;
                                } else if ((selectedDifficulty == 0 && fashionScore == 1) || (selectedDifficulty == 1 && fashionScore < 3)) {
                                    UART_Print("\r\nAs you stand in front of the judges, they give you small smiles, but they just aren’t able to give you first place. You do get second, though, and earn 3 points.\r\n");
                                    earned = (selectedDifficulty == 1) ? 3 : 2;
                                } else {
                                    UART_Print("\r\nAs you stand in front of the judges, they look up at you in awe. They each hold up their board and display all 10’s. Congratulations, you won first place and gained 5 points!\r\n");
                                    earned = (selectedDifficulty == 1) ? 5 : 3;
                                }

                                totalPoints += earned;
                                prevFashionStep = fashionStep;
                                fashionScore = 0;
                                fashionStep = 0;
                                currentState = STATE_HUB;
                            }

                            dataAvail = 0;
                            break;
                            }
                    break;







                    case STATE_PIE_CONTEST:
                    {


                        static char pieAnsBuf[12];
                        static uint8_t questionsSolved = 0;
                        static uint8_t prevPieStep = 255;

                        if (stateChanged) {
                            pieStep = 0;
                            prevPieStep = 255;
                            needsNewQuestion = 1;

                            srand(HAL_GetTick() ^ pieStep ^ questionsSolved);

                            startTime = HAL_GetTick();
                            questionsSolved = 0;

                            correctAns = 0;
                            pieAnsIdx = 0;
                            memset(pieAnsBuf, 0, sizeof(pieAnsBuf));
                        }

                        if (pieStep == 0) {

                            if (pieStep != prevPieStep) {
                                UART_Print(Pie);
                                UART_Print(PieArt);
                                UART_Print("\r\nHit y when ready. ");
                                prevPieStep = pieStep;
                            }

                            if (dataAvail && rx_buff[0] == 'y') {
                                UART_Print("\r\nGO!\r\n");
                                startTime = HAL_GetTick();
                                questionsSolved = 0;
                                pieStep = 1;
                                needsNewQuestion = 1;
                                dataAvail = 0;
                            }

                            dataAvail = 0;
                        }

                        else if (pieStep == 1) {

                            uint32_t timeLimit = (selectedDifficulty == 0) ? 15000 : 10000;

                            if (HAL_GetTick() - startTime > timeLimit) {
                                pieStep = 2;
                            }

                            if (needsNewQuestion) {
                                needsNewQuestion = 0;

                                int a, b;
                                int type = rand() % (selectedDifficulty ? 3 : 2);

                                if (selectedDifficulty == 0) {
                                    a = (rand() % 30) + 1;
                                    b = (rand() % 30) + 1;

                                    if (rand() % 2 == 0) {
                                        correctAns = a + b;
                                        sprintf(mathPrompt, "\r\nSlices: %d + %d = ", a, b);
                                    } else {
                                        if (b > a) { int t = a; a = b; b = t; }
                                        correctAns = a - b;
                                        sprintf(mathPrompt, "\r\nSlices: %d - %d = ", a, b);
                                    }
                                }
                                else {
                                    if (type == 0) {
                                        a = (rand() % 10) + 1;
                                        b = (rand() % 10) + 1;
                                        correctAns = a * b;
                                        sprintf(mathPrompt, "\r\nSlices: %d * %d = ", a, b);
                                    }
                                    else if (type == 1) {
                                        a = (rand() % 30) + 1;
                                        b = (rand() % 30) + 1;
                                        correctAns = a + b;
                                        sprintf(mathPrompt, "\r\nSlices: %d + %d = ", a, b);
                                    }
                                    else {
                                        a = (rand() % 30) + 1;
                                        b = (rand() % 30) + 1;
                                        if (b > a) { int t = a; a = b; b = t; }
                                        correctAns = a - b;
                                        sprintf(mathPrompt, "\r\nSlices: %d - %d = ", a, b);
                                    }
                                }

                                UART_Print(mathPrompt);
                            }

                            if (dataAvail) {
                                char c = rx_buff[0];
                                dataAvail = 0;

                                if (c == '\r' || c == '\n') {

                                    if (pieAnsIdx > 0) {
                                        int playerAns = atoi(pieAnsBuf);

                                        if (playerAns == correctAns) {
                                            UART_Print(" -> Correct!");
                                            questionsSolved++;
                                        } else {
                                            UART_Print(" -> Wrong!");
                                        }
                                    }

                                    correctAns = 0;
                                    memset(pieAnsBuf, 0, sizeof(pieAnsBuf));
                                    pieAnsIdx = 0;
                                    needsNewQuestion = 1;
                                }

                                else if ((c >= '0' && c <= '9') || (c == '-' && pieAnsIdx == 0)) {
                                    if (pieAnsIdx < sizeof(pieAnsBuf) - 1) {
                                        pieAnsBuf[pieAnsIdx++] = c;
                                        char out[2] = { c, '\0' };
                                        UART_Print(out);
                                    }
                                }

                                else if (c == '\b' || c == 127) {
                                    if (pieAnsIdx > 0) {
                                        pieAnsIdx--;
                                        pieAnsBuf[pieAnsIdx] = '\0';
                                        UART_Print("\b \b");
                                    }
                                }
                            }
                        }

                        else if (pieStep == 2) {

                            if (pieStep != prevPieStep) {

                                if (questionsSolved <= 2) {
                                    UART_Print("\r\nTry as you may you’re still not able to come out in first place. You ended up landing in third place, earning 1 point…\r\n");
                                    totalPoints += 1;
                                }
                                else if (questionsSolved <= 4) {
                                    UART_Print("\r\nTry as you may, you’re still not able to come out in first place. You ended up landing in second place, earning 2 points. Not bad!\r\n");
                                    totalPoints += 2;
                                }
                                else {
                                    UART_Print("\r\nAs the competition starts bailing out around you, you end up gaining more confidence and speed while scarfing down these pies. In the end, you placed first, raising your fist in victory and earning 3 points!\r\n");
                                    totalPoints += 3;
                                }

                                inputLocked = 1;
                                stateLockTime = HAL_GetTick();
                                prevPieStep = pieStep;
                            }

                            dataAvail = 0;
                        }

                        break;
                    }

            case STATE_COASTER_CRASH:
                if (stateChanged) {
                    UART_Print(RollerCoaster);
                    UART_Print("\r\nNot wanting to harm innocent civilians, you decide it is best to make an opening, then run to cover. And "
                               "considering basic zombie logic, you probably shouldn’t get too close to them. What do you do?\r\n");

                    if (selectedDifficulty == 0) {
                        UART_Print("\r\n1. Use a sword to jab your way through.\r\n2. Chant a spell to draw them back.\r\n3. Ram through the crowd behind a shield\r\n4. Draw your bow and arrow to take them down.\r\n");
                    } else {
                        UART_Print("\r\nWhat are zombies weak too? \r\n1. Fire damage\r\n2. Necrotic damage\r\n3. Radiant damage\r\n4. Poison damage\r\n");
                    }
                }

                if (dataAvail) {
                    battleScore = 0;

                    if (selectedDifficulty == 0) {
                        if (rx_buff[0] == '1') battleScore = 0;
                        else if (rx_buff[0] == '2') battleScore = 3;
                        else if (rx_buff[0] == '3') battleScore = 1;
                        else if (rx_buff[0] == '4') battleScore = 2;
                    } else {
                        if (rx_buff[0] == '1') battleScore = 3;
                        else if (rx_buff[0] == '2') battleScore = 2;
                        else if (rx_buff[0] == '3') battleScore = 4;
                        else if (rx_buff[0] == '4') battleScore = 1;
                    }

                    UART_Printf("\r\nYou stick with your choice and manage to make your way through the hoard with your crew, earning %d points.\r\n", battleScore);
                    totalPoints += battleScore;
                    currentState = STATE_BASEMENT;
                    UART_Print(Basement);
                    dataAvail = 0;
                }
                break;

        case STATE_BASEMENT:
        	if (stateChanged) {
        		UART_Print("\r\nWhich room would you like to enter?\r\n");
        		UART_Print("\r\n1. Bedroom | 2. Infirmary | 3. Engine Room\r\n");
        	}
            if(dataAvail) {
                if(rx_buff[0] == '1') currentState = STATE_BEDROOM;
                if(rx_buff[0] == '2') {
                	if (infirmaryCleared) {
                		UART_Print("\r\nYou locked the door behind you, remember? Best not to go back in there.\r\n");
                	} else {
                		currentState = STATE_INFIRMARY;
                	}
                }
                if(rx_buff[0] == '3') currentState = STATE_ENGINEROOM;
                dataAvail = 0;
            }
            break;

        case STATE_BEDROOM:

            if (stateChanged) {
                UART_Print("\r\n1You knock on the door, expecting the ringleader to be inside, but it seems to be empty. You make your way inside to search for clues.\r\n");
                UART_Print("\r\nWhere would you like to search?\r\n");
                UART_Print("\r\n1. Bookshelf\r\n2. Bed\r\n3. Desk\r\n4. Exit\r\n");
            }

            if (dataAvail) {

                char c = rx_buff[0];
                dataAvail = 0;

                if (c == '1') {

                    if (checkedShelf == 0) {
                        UART_Print("\r\nYou search the shelves for anything useful, but it just looks like a personal collection. The dust makes you sneeze.\r\n");
                        checkedShelf++;
                    } else {
                        UART_Print("\r\nYou don’t feel the need to sneeze again.\r\n");
                    }

                }

                else if (c == '2') {

                    if (checkedBed == 0) {
                        UART_Print(Bed);
                        inventory[0] = 1;
                        checkedBed++;
                    } else {
                        UART_Print("\r\nRifling through someone’s bed seems invasive.\r\n");
                    }

                }

                else if (c == '3') {

                    if (checkedDesk == 0) {
                        UART_Print("\r\nWalking over to the desk, you see some paper strewn on top. Not understanding any of the business documents, you decide to leave it alone and move on to the drawers. Opening the lowest one, you find a book labeled ‘College.’ \r\nFlipping open the pages, ");
                        UART_Print(Desk);
                        checkedDesk++;
                    } else {
                        UART_Print("\r\nWalking back over to the desk, you open the book to read over the passages again. ");
                        UART_Print(Desk);
                    }

                }

                else if (c == '4') {
                    currentState = STATE_BASEMENT;
                }

                UART_Print("\r\nWhere would you like to search?\r\n1. Bookshelf\r\n2. Bed\r\n3. Desk\r\n4. Exit\r\n");
            }

            break;

        case STATE_INFIRMARY:

        	if (stateChanged) {
        		UART_Print(Dragon);
        		UART_Print(DragonArt);
        		UART_Print("\r\nHow do you want to do this?\r\n");
        		if (selectedDifficulty == 0) {
        			UART_Print("\r\n-1. Use a sword to pierce through the scales\r\n-2. Chant a spell to rain down on him.\r\n-3. Sheild yourself from the fiery attacks\r\n-4. Draw your bow and arrow to take them down.\r\n");
        		} else {
        			UART_Print("\r\nWhats effective against dragons? \r\n1. Physical damage\r\n2. Lightning damage\r\n3. Psychic damage\r\n4. Fire damage\r\n");
        		}
        	}

                	if (dataAvail) {
                		battleScore = 0;

                		if (selectedDifficulty == 0) {
                			if (rx_buff[0] == '1') battleScore = 3;
                			else if (rx_buff[0] == '2') battleScore = 0;
                			else if (rx_buff[0] == '3') battleScore = 2;
                			else if (rx_buff[0] == '4') battleScore = 1;
                		} else {
                			if (rx_buff[0] == '1') battleScore = 3;
                			else if (rx_buff[0] == '2') battleScore = 2;
                			else if (rx_buff[0] == '3') battleScore = 4;
                			else if (rx_buff[0] == '4') battleScore = 1;
                		}

                		UART_Printf("\r\nThe battle is challenging, most of your attacks not getting through thick skin, but with enough chipping, the dragon falls to his knees in defeat. You earned %d points.", battleScore);
    	                totalPoints += battleScore;
                        UART_Print(Medicine);
                        inventory[1] = 1;
                        infirmaryCleared = 1;
                        currentState = STATE_BASEMENT;
                        dataAvail = 0;
                	}
                    break;

        case STATE_ENGINEROOM:
            if (stateChanged) {
                UART_Print("\r\nYou make your way up to the door, but as you go to open it, the handle jerks in response. Locked.\r\n"
                           "Looks like you need a key. And you should probably find a cure...\r\n");



            if (inventory[0] == 1 && inventory[1] == 1) {
                UART_Print("\r\n1. Unlock door\r\n2. Go Back\r\n");
            } else {
                UART_Print("\r\n1. Go back\r\n");
            }
            }

            if (dataAvail) {
                char c = rx_buff[0];
                dataAvail = 0;

                if (inventory[0] == 1 && inventory[1] == 1) {
                    if (c == '1') currentState = STATE_FINAL_BOSS;
                    else if (c == '2') currentState = STATE_BASEMENT;
                } else {
                    if (c == '1') currentState = STATE_BASEMENT;
                }
            }
            break;

        case STATE_FINAL_BOSS: {

            static uint32_t bossTimer = 0;
            static uint32_t pauseTimer = 0;
            static uint8_t mashPoints = 0;
            static uint8_t phaseActive = 0;

            if (stateChanged) {
                bossStep = 0;
                results = 0;
                prevBossStep = 255;
                phaseActive = 0;
                rBuffer = 0;
            }

            if (bossStep == 0) {

                if (bossStep != prevBossStep) {
                    UART_Print(EngineRoom);
                    UART_Print("\r\nHow do you attack?\r\n-1. Resist (MASH 'r')\r\n-2. Jump behind them\r\n");
                    prevBossStep = bossStep;
                }

                if (dataAvail) {
                    char c = rx_buff[0];
                    dataAvail = 0;

                    if (c == '1') {
                        mashPoints = 0;
                        rBuffer = 0;
                        bossTimer = HAL_GetTick();
                        phaseActive = 1;
                        UART_Print("\r\nGO!\r\n");
                    } else {
                        bossStep = 1;
                    }
                }

                if (phaseActive) {

                    if (HAL_GetTick() - bossTimer < 5000) {
                        mashPoints = rBuffer;
                    } else {
                        phaseActive = 0;
                        rBuffer = 0;

                        results += (mashPoints >= (selectedDifficulty ? 30 : 20)) ? 3 : 2;

                        UART_Print("\r\nGood mashing!\r\n");
                        pauseTimer = HAL_GetTick();
                        bossStep = 1;
                    }
                }
            }

            else if (bossStep == 1) {

                if (bossStep != prevBossStep) {
                    UART_Print("\r\nAnother teammate jumps in to save your friend, how do you respond?\r\n-1. Pull your ally in\r\n-2. Resist (MASH 'r')\r\n");
                    prevBossStep = bossStep;
                }

                if (dataAvail) {
                    char c = rx_buff[0];
                    dataAvail = 0;

                    if (c == '2') {
                        mashPoints = 0;
                        rBuffer = 0;
                        bossTimer = HAL_GetTick();
                        phaseActive = 1;
                        UART_Print("\r\nGO!\r\n");
                    } else {
                        bossStep = 2;
                    }
                }

                if (phaseActive) {

                    if (HAL_GetTick() - bossTimer < 5000) {
                        mashPoints = rBuffer;
                    } else {
                        phaseActive = 0;
                        rBuffer = 0;

                        results += (mashPoints >= (selectedDifficulty ? 35 : 25)) ? 3 : 2;

                        UART_Print("\r\nGood mashing!\r\n");
                        pauseTimer = HAL_GetTick();
                        bossStep = 2;
                    }
                }
            }

            else if (bossStep == 2) {

                if (bossStep != prevBossStep) {
                    UART_Print("\r\nAfter fighting them off, your last ally aims at The General. What do you do?\r\n-1. Resist (MASH 'r')\r\n-2. Take them out first.\r\n");
                    prevBossStep = bossStep;
                }

                if (dataAvail) {
                    char c = rx_buff[0];
                    dataAvail = 0;

                    if (c == '1') {
                        mashPoints = 0;
                        rBuffer = 0;
                        bossTimer = HAL_GetTick();
                        phaseActive = 1;
                        UART_Print("\r\nGO!\r\n");
                    } else {
                        bossStep = 3;
                    }
                }

                if (phaseActive) {

                    if (HAL_GetTick() - bossTimer < 5000) {
                        mashPoints = rBuffer;
                    } else {
                        phaseActive = 0;
                        rBuffer = 0;

                        results += (mashPoints >= (selectedDifficulty ? 40 : 30)) ? 3 : 2;

                        UART_Print("\r\nGood mashing!\r\n");
                        pauseTimer = HAL_GetTick();
                        bossStep = 3;
                    }
                }
            }

            else if (bossStep == 3) {

                if (HAL_GetTick() - pauseTimer >= 1500) {

                    UART_Print("\r\n=== BOSS COMPLETE ===\r\n");

                    totalPoints += results;

                    if (results >= 8)
                        UART_Print(GoodEnd);
                    else
                        UART_Print(BadEnd);

                    currentState = STATE_END;
                }
            }

            break;
        }

        case STATE_END:
        	if (stateChanged) {
        		UART_Print("\r\n=== GAME OVER ===\r\n");
        		char finalPts[64];
        		sprintf(finalPts, "Final Score: %d points\r\n", totalPoints);
        		UART_Print(finalPts);
        		UART_Print("Want to save your score?\r\n-1. Yes\r\n-2. No\r\n");
        	}

            if(dataAvail) {
                if(rx_buff[0] == '1') {
                    currentState = STATE_SAVE_SCORE;
                    UART_Print("\r\nPlease insert your name and press ENTER: ");
                    name_index = 0; //
                    memset(name_temp, 0, sizeof(name_temp));
                } else {
                	currentState = STATE_POST_GAME;
                }
                dataAvail = 0;
            }
            break;

        case STATE_SAVE_SCORE:
        {
            if (dataAvail) {
                char c = rx_buff[0];
                dataAvail = 0;

                if (c == '\r' || c == '\n') {
                    name_temp[name_index] = '\0';

                    if (scoreCount < 5) {
                        strcpy(highScores[scoreCount].name, name_temp);
                        highScores[scoreCount].points = totalPoints;
                        scoreCount++;
                    } else {
                        SortScores();
                        if (totalPoints > highScores[4].points) {
                            strcpy(highScores[4].name, name_temp);
                            highScores[4].points = totalPoints;
                        }
                    }

                    UART_Print("\r\nScore Saved!\r\n");
                    SortScores();
                    saveScoresToFlash();
                    currentState = STATE_POST_GAME;
                }

                else if (c == 127 || c == '\b') {
                    if (name_index > 0) {
                        name_index--;
                        name_temp[name_index] = '\0';
                        UART_Print("\b \b");
                    }
                }

                else if (c >= 32 && c <= 126) {
                    if (name_index < 19) {
                        name_temp[name_index++] = c;
                        char out[2] = {c, '\0'};
                        UART_Print(out);
                    }
                }
            }
        }
        break;
        case STATE_POST_GAME:
        	if (stateChanged) {
        		UART_Print("\r\nWould you like to restart the adventure, or quit?\r\n-1. Restart\r\n-2. Quit\r\n");
        	}
        	if (dataAvail) {
        		if (rx_buff[0] == '1') {

        		    totalPoints = 0;
        		    selectedDifficulty = 0;
        		    playerChar = 0;

        		    inventory[0] = 0;
        		    inventory[1] = 0;

        		    infirmaryCleared = 0;

        		    checkedBed = 0;
        		    checkedDesk = 0;
        		    checkedShelf = 0;

        			currentState = STATE_TITLE;
        		} else if (rx_buff[0] == '2') {
        			currentState = STATE_QUIT;
        		}
        		dataAvail = 0;
        	}
        	break;

        case STATE_QUIT:
        	if (stateChanged) {
        		UART_Print("\r\nSee you later! You can restart the game by pressing the reset button.\r\n");

    }
            break;
    }
  }
}



/* Helper Functions */
void UART_Print(const char* str) {
    uint16_t len = (uint16_t)strlen(str);
    HAL_UART_Transmit(&huart2, (uint8_t*)str, len, HAL_MAX_DELAY);
}


void UART_Printf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    UART_Print(buf);}

void SortScores(void) {

    for(int i=0; i<scoreCount-1; i++) {
        for(int j=0; j<scoreCount-i-1; j++) {
            if(highScores[j].points < highScores[j+1].points) {
                ScoreEntry temp = highScores[j];
                highScores[j] = highScores[j+1];
                highScores[j+1] = temp;
            }
        }
    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    char c = rx_buff[0];

    if (c == 'r') {
        rBuffer++;
        dataAvail = 0;
    } else {
        dataAvail = 1;
    }

    HAL_UART_Receive_IT(&huart2, (uint8_t*)rx_buff, 1);
}

void saveScoresToFlash(void) {
    FLASH_EraseInitTypeDef erase;
    uint32_t pageError;

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = 254;
    erase.NbPages = 1;

    if (HAL_FLASHEx_Erase(&erase, &pageError) != HAL_OK) {
        UART_Print("Flash Erase Error!\r\n");
        HAL_FLASH_Lock();
        return;
    }


    uint64_t *src = (uint64_t*)highScores;
    uint32_t address = SCORE_FLASH_ADDR;


    int num_chunks = sizeof(highScores) / 8;
    if (sizeof(highScores) % 8 != 0) num_chunks++;

    for (int i = 0; i < num_chunks; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, src[i]) == HAL_OK) {
            address += 8;
        } else {
            UART_Print("Flash Write Error!\r\n");
            break;
        }
    }

    HAL_FLASH_Lock();
}


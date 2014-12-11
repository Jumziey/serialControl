#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <SDL/SDL.h>

const int deadzone = 6000;
const int debounce = 0; //In microseconds, not needed for xbox360 contr.
const int shoulder[2] = {2,5};
const int shoulders = 2;

typedef struct buttonHistory {
	int state; //1 For pressed, 0 for depressed
	struct timeval time;
} buttonHistory;

SDL_Joystick* init() {
	SDL_Joystick *joy;
	
	if(SDL_Init(SDL_INIT_JOYSTICK) < 0) {
		fprintf(stderr,"Could not initialize SDL Joystick: %s\n", SDL_GetError());
		return NULL;
	}
	
	fprintf(stderr,"joysticks found: %d\n", SDL_NumJoysticks());
	
	joy = SDL_JoystickOpen(0);
	if(joy == NULL) {
		fprintf(stderr,"Could not open joystick\n");
		return NULL;
	}

	return joy;
}

void checkJoystick(SDL_Joystick *joy) {
	printf("Axes: %d\n",SDL_JoystickNumAxes(joy));
	printf("Balls: %d\n",SDL_JoystickNumBalls(joy));
	printf("Hats: %d\n",SDL_JoystickNumHats(joy));
	printf("Buttons: %d\n",SDL_JoystickNumButtons(joy));
	return;
}


void getJoystickInput(SDL_Joystick* joy, buttonHistory buttonStory[], int *shoulderStory) {
	int i,j,n;
	struct timeval new,diff;
	
	SDL_JoystickUpdate();
	
	for(i=0; i<=SDL_JoystickNumButtons(joy); i++) {
		n = SDL_JoystickGetButton(joy, i);
		if(buttonStory[i].state != n) {
			gettimeofday(&new, NULL);
			timersub(&new, &(buttonStory[i].time), &diff);
			if(diff.tv_usec>debounce || diff.tv_sec>=1) {
				buttonStory[i].state = n;
				fprintf(stderr,"Button %d goes to state %d\n", i, n);
			}
		}
	}
			
			
	
	for(i=0; i<SDL_JoystickNumAxes(joy); i++) {
		for(j=0; j<shoulders; j++) {
			if(i == shoulder[j]) {
				n = SDL_JoystickGetAxis(joy, i);
				if(n == 0 && shoulderStory[i] == 0)
					goto DONE;
				n += 32768;
				shoulderStory[i] = 1;
				if( abs(n)>deadzone)
					fprintf(stderr,"Shoulder Axis %d is %d\n", i, n);
				goto DONE;
			}
		}
		n = SDL_JoystickGetAxis(joy, i);
		if( abs(n)>deadzone)
			fprintf(stderr,"Axis %d is %d\n", i, n);
		DONE:;
	}
}

int main() {
	int i;
	buttonHistory *buttonStory;
	int *shoulderStory;
	SDL_Joystick *joy;
	
	joy = init();
	if(joy == NULL)
		exit(1);
	checkJoystick(joy);
	
	int axes = SDL_JoystickNumAxes(joy);
	shoulderStory = calloc(axes, sizeof(int));
	
	int buttons = SDL_JoystickNumButtons(joy);
	buttonStory = calloc(buttons, sizeof(struct buttonHistory));
	for(i=0; i<=SDL_JoystickNumButtons(joy); i++) {
		gettimeofday(&buttonStory[i].time, NULL);
		buttonStory[i].state = 0;
	}
	
	if(SDL_JoystickEventState(SDL_IGNORE) < 0) {
		fprintf(stderr, "Cannot enter event state: %s\n", SDL_GetError());
		exit(1);
	}
	
	while(1) {
		getJoystickInput(joy, buttonStory, shoulderStory);
		SDL_Delay(1);
	}
	/*
	SDL_JoystickClose(joy);
	SDL_Quit();
	*/
	return 0;
}
 

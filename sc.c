#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#include <limits.h>

const int deadzone = 1800;
const int axisRes = 180; //Max axis +- resolution... Maximum possible value is SHRT_INT
const int debounce = 0; //In microseconds, not needed for xbox360 contr.
//const int shoulder[2] = {2,5};
//const int shoulders = 2;
const int hatState[] = {SDL_HAT_CENTERED, SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHTUP, SDL_HAT_RIGHTDOWN, SDL_HAT_LEFTUP, SDL_HAT_LEFTDOWN};
const int hatStates = 9;
double axisMinChange;

typedef struct button {
	int state; //1 For pressed, 0 for depressed
	struct timeval *lastUpdate;
} button;

typedef struct shoulder {
	int used;
	int value;
} shoulder;

typedef struct hat {
	int state;
} hat;

typedef struct axis {
	int value;
} axis;

typedef struct ctrlState {
	button	*buttons;
	axis						*axes;
	shoulder			*shoulders;
	hat						*hats;
	int						numButtons;
	int						numAxes;
	int						numShoulders;
	int						*shoulderNr;
	int						numHats;
	int						debounce;
	int						deadzone;
} ctrlState;

send(char type, int num, int val) {
	fprintf(stderr, "%c%d: %d\n",type,num,val);
}

ctrlState* init(SDL_Joystick **joy) {
	ctrlState *joyState;
	int i;
	

	
	if(SDL_Init(SDL_INIT_JOYSTICK) < 0) {
		fprintf(stderr,"Could not initialize SDL Joystick: %s\n", SDL_GetError());
		return NULL;
	}
	
	fprintf(stderr,"joysticks found: %d\n", SDL_NumJoysticks());
	
	*joy = SDL_JoystickOpen(0);
	if(joy == NULL) {
		fprintf(stderr,"Could not open joystick\n");
		return NULL;
	}
	
	joyState = calloc(sizeof(ctrlState), 1);
	joyState->numAxes = SDL_JoystickNumAxes(*joy);
	joyState->numHats = SDL_JoystickNumHats(*joy);
	joyState->numButtons = SDL_JoystickNumButtons(*joy);
	joyState->axes = calloc(joyState->numAxes, sizeof(axis));
	joyState->hats = calloc(joyState->numHats, sizeof(hat));
	joyState->buttons = calloc(joyState->numButtons, sizeof(button));
	for(i=0; i<=joyState->numButtons; i++) {
		joyState->buttons[i].lastUpdate = malloc(sizeof(struct timeval));
		gettimeofday(joyState->buttons[i].lastUpdate, NULL);
		joyState->buttons[i].state = 0;
	}
	return joyState;
}

void checkJoystick(SDL_Joystick *joy) {
	printf("Axes: %d\n",SDL_JoystickNumAxes(joy));
	printf("Balls: %d\n",SDL_JoystickNumBalls(joy));
	printf("Hats: %d\n",SDL_JoystickNumHats(joy));
	printf("Buttons: %d\n",SDL_JoystickNumButtons(joy));
	return;
}


int getJoystickInput(SDL_Joystick* joy, ctrlState *joyState) {
	int i,j,n;
	struct timeval *new,diff;
	int changed = 0;
	int newState;
	
	new = malloc(sizeof(struct timeval));
	
	SDL_JoystickUpdate();
	
	for(i=0; i<=joyState->numButtons; i++) {
		n = SDL_JoystickGetButton(joy, i);
		if(joyState->buttons[i].state != n) {
			gettimeofday(new, NULL);
			timersub(new, joyState->buttons[i].lastUpdate, &diff);
			if(diff.tv_usec>joyState->debounce || diff.tv_sec>=1) {
				joyState->buttons[i].state = n;
				send('b',i,n);
				memcpy(joyState->buttons[i].lastUpdate, new, sizeof(struct timeval));
				changed = 1;
				
			}
		}
	}
	free(new);
			
	
	for(i=0; i<joyState->numAxes; i++) {
		for(j=0; j<joyState->numShoulders; j++) {
			if(i == joyState->shoulderNr[j]) {
				n = SDL_JoystickGetAxis(joy, i);
				if(n == 0 && joyState->shoulders[i].used == 0)
					goto DONE;
				n += 	SHRT_MAX;
				joyState->shoulders[i].used = 1;
				if( abs(n)>joyState->deadzone) {
					if(abs(joyState->shoulders[i].value - n/axisMinChange)>1) {
						joyState->shoulders[i].value = n/axisMinChange;
						send('a',i,n/axisMinChange);
						changed = 1;
					}
				} else if(joyState->shoulders[i].value != 0) {
						joyState->shoulders[i].value = 0;
						send('a',i,0);
						changed = 1;
				}
				goto DONE;
			}
		}
		n = SDL_JoystickGetAxis(joy, i);
		if(abs(n)>joyState->deadzone) {
			if(abs(joyState->axes[i].value - n/axisMinChange)>1) {
				joyState->axes[i].value = n/axisMinChange;
				send('a',i,n/axisMinChange);
				changed = 1;
			}
		} else if(joyState->axes[i].value != 0) {
			joyState->axes[i].value = 0;
			send('a',i,0);
			changed = 1;
		}
		DONE:;
	}
	


	for(i=0; i<SDL_JoystickNumHats(joy); i++) {
		newState = SDL_JoystickGetHat(joy, i);
		for(j=0; j<hatStates; j++) {
			if(newState == hatState[j] && joyState->hats[i].state != newState) {
				joyState->hats[i].state = newState;
				send('h',i,newState);
				changed =1;
			}
		}
	}
			
		
	return(changed);
}

int main() {
	int i, changed;
	SDL_Joystick *joy;
	ctrlState *joyState;

	
	joyState = init(&joy);
	if(joy == NULL)
		exit(1);
	checkJoystick(joy);
	
	//Stuff that should be done in init when conf parse is created.
	axisMinChange = SHRT_MAX/axisRes;
	printf("axisMinChange: %f\n", axisMinChange);
	printf("Int_max %d, axisRes %d \n", SHRT_MAX , axisRes);
	joyState->numShoulders = 2;
	joyState->shoulders = calloc(joyState->numShoulders, sizeof(shoulder));
	joyState->shoulderNr = calloc(joyState->numAxes, sizeof(axis));
	joyState->deadzone = deadzone;
	joyState->debounce = debounce;
	
	joyState->shoulderNr[0] = 2;
	joyState->shoulderNr[1] = 5;
	
	//dunno where this should go
	if(SDL_JoystickEventState(SDL_IGNORE) < 0) {
		fprintf(stderr, "Cannot enter event state: %s\n", SDL_GetError());
		exit(1);
	}
	
	
	while(1) {
		changed = getJoystickInput(joy, joyState);
		SDL_Delay(1);
	}
	printf("wazza?\n");
	
	//Should maybe write a free func to free everything
	//To easily check for memleaks
	SDL_JoystickClose(joy);
	SDL_Quit();
	
	return 0;
}
 

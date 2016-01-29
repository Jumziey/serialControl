#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <SDL/SDL.h>
#include <limits.h>
#include <confuse.h> //Config parser
#include <limits.h>
#include <signal.h>
#include "arduino-serial-lib.h"

#include <unistd.h>

const int hatState[] = {SDL_HAT_CENTERED, SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHTUP, SDL_HAT_RIGHTDOWN, SDL_HAT_LEFTUP, SDL_HAT_LEFTDOWN};
const int hatStates = 9;

//Some globals i haven't decided if im lazy to have them here
//Or if its actually beautiful (fuck you if you hate globals by principle)
double axisMinChange;
int portFd;
int testRun;

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
	button				*buttons;
	axis					*axes;
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
	if(testRun) {
		fprintf(stderr, "%c%d:%d\n",type,num,val);
	} else {
		char* buf = calloc(20, sizeof(char));
		sprintf(buf, "%c%d:%d\n",type,num,val);
		serialport_write(portFd, buf);
	}
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
				if(n == 0 && joyState->shoulders[j].used == 0)
					goto DONE;
				n += 	SHRT_MAX;
				joyState->shoulders[j].used = 1;
				if( abs(n)>joyState->deadzone) {
					if(abs(joyState->shoulders[j].value - n/axisMinChange)>1) {
						joyState->shoulders[j].value = n/axisMinChange;
						send('s',j,n/axisMinChange);
						changed = 1;
					}
				} else if(joyState->shoulders[j].value != 0) {
						joyState->shoulders[j].value = 0;
						send('s',j,0);
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

typedef struct ctrlConf {
	int testRun;
	int deadzone;
	int axisRes;
	int debounce;
	int numShoulders;
	int* shoulderNr;
	char* port;
} ctrlConf;

ctrlConf*
readConf(char* conf) {
	int i;
	ctrlConf* joyConf;
	
	joyConf = malloc(sizeof(ctrlConf));
	
	//Define config
	cfg_opt_t opts[] = 
	{
		CFG_INT("testRun", 0, CFGF_NONE),
		CFG_INT("deadzone", 0, CFGF_NONE),
		CFG_INT("axisRes", 0, CFGF_NONE),
		CFG_INT("debounce", 0, CFGF_NONE),
		CFG_INT_LIST("shoulderNr", NULL, CFGF_NONE),
		CFG_END()
	};
	cfg_t *cfg;

	//Parse config
	cfg = cfg_init(opts, CFGF_NONE);
	if(cfg_parse(cfg, "360.conf") == CFG_PARSE_ERROR)
		return NULL;
	
	//Assign config values
	joyConf->testRun = (int)cfg_getint(cfg, "testRun");
	joyConf->deadzone = (int)cfg_getint(cfg, "deadzone");
	joyConf->axisRes = (int)cfg_getint(cfg, "axisRes");
	joyConf->debounce = (int)cfg_getint(cfg, "debounce");
	joyConf->numShoulders = (int)cfg_size(cfg, "shoulderNr");
	
	joyConf->shoulderNr = calloc(joyConf->numShoulders, sizeof(int));
	for(i=0; i<joyConf->numShoulders; i++)
		joyConf->shoulderNr[i] = cfg_getnint(cfg, "shoulderNr", i);

	return joyConf;
}



void shutdown(int sig) {
	close(portFd);
	printf("IM SHUTTING DOWN!\n");
	sleep(2);
	exit(0);
}
	

int main(int argc, char **argv) {
	int i, changed, c;
	char* confFile;
	SDL_Joystick *joy;
	ctrlState *joyState;
	ctrlConf *joyConf;
	
	if(argc != 2) {
		fprintf(stderr, "takes one and only one argument... THE CONFIG FILE!!!\n");
		exit(1);
	}
	confFile = argv[1];
	
	joyState = init(&joy);
	if(joy == NULL)
		exit(1);
	checkJoystick(joy);

	if(!(joyConf = readConf(confFile))) {
		exit(1);
	}

	//Stuff that should be done in init when conf parse is created.
	axisMinChange = SHRT_MAX/joyConf->axisRes;
	printf("axisMinChange: %f\n", axisMinChange);
	joyState->numShoulders = joyConf->numShoulders;
	joyState->shoulders = calloc(joyState->numShoulders, sizeof(shoulder));
	joyState->shoulderNr = calloc(joyState->numShoulders, sizeof(int));
	joyState->deadzone = joyConf->deadzone;
	joyState->debounce = joyConf->debounce;
	
	joyState->shoulderNr[0] = joyConf->shoulderNr[0];
	joyState->shoulderNr[1] = joyConf->shoulderNr[1];
	
	//dunno where this should go
	if(SDL_JoystickEventState(SDL_IGNORE) < 0) {
		fprintf(stderr, "Cannot enter event state: %s\n", SDL_GetError());
		exit(1);
	}
	
	//Establish serial contact if this is not a testRun
	testRun = joyConf->testRun;
	if(!testRun) {
		if((portFd = serialport_init("/dev/ttyACM0", 9600))==-1)
			exit(1);
		
		char* buf = calloc(20, sizeof(char));
		int notConnected = 1;
		while(notConnected){
			printf("connecting...\n");
			serialport_write(portFd, "#");
			serialport_read_until(portFd, buf, 'C', 20, 3000);
			if(!strcmp(buf, "C"))
				notConnected = 0;
			}
		printf("connected\n");
	}

	//Just to make sure the program disconnects
	signal(SIGINT, shutdown);
	
	while(1) {
		changed = getJoystickInput(joy, joyState);
		SDL_Delay(1);
	}
	
	//Should maybe write a free func to free everything
	//To easily check for memleaks
	SDL_JoystickClose(joy);
	SDL_Quit();
	
	return 0;
}

#include <windows.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <shellapi.h>
#include <stdio.h>
#include "proc_info.h"

// horrible global variables
static uint32_t credit = 3600*1000; // remaining non-work time, ms; user is given 1 hour for free at startup
static bool good = true; // whether the user is good for now
static float work_per_play = 2; // how many times more time user should spend working to afford same amount of play time

static struct {
	char* class;
	char* text;
	char* path;
} *whitelist = NULL;
static size_t whitelist_size = 0;

static void CALLBACK foreground_changed(HWINEVENTHOOK ev_hook, DWORD event, HWND new_foreground, LONG object, LONG child, DWORD thread, DWORD time) {
	// FIXME: do I need that much info?
	char executable[2048] = {0}, text[2048] = {0}, class[2048] = {0};

	if (get_window_info(new_foreground, class, 2048, text, 2048, executable, 2048) != TRUE)
		return; // FIXME: actually, assume it's whitelisted

	//recalculate the "good time" credit
	//if whitelisted: stop any kill timers & sounds, set good=true
	//else: set good=false, set timer for (remaining-5min) to sound alarm
}

// TODO: alarm sound callback: remember GetForegroundWindow() and set 5m timer to kill it

// TODO: kill callback: check non-whitelistedness of GetForegroundWindow() and EndTask it

// TODO: bool is_whitelisted(HWND)

static char* fgets_alloc(FILE* fh) {
	size_t len = 2048; // unlikely to have strings that long
	char* line = malloc(len);
	if (!line) abort();

	if (!fgets(line,len,fh)) { // EOF before reading anything
		free(line);
		return NULL;
	}

	while (line[strlen(line)-1] != '\n') { // no \n at the end => we didn't get the whole string
		len *= 2;
		line = realloc(line, len);
		if (!line) abort();
		if (!fgets(line,len,fh)) break; // EOF => we can stop
	}
	if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0'; // FIXME: O(2n)

	return line;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		MessageBox(NULL,"Please pass path to whitelist.txt as the only command line argument.",NULL,0);
		return 1;
	}

	FILE* whitelist_fh = fopen(argv[1], "r");
	if (!whitelist_fh) abort();

	while (char* line = fgets_alloc(whitelist_fh)) {
		whitelist_size++;
		whitelist = realloc(whitelist,whitelist_size*sizeof(*whitelist));
		if (!whitelist) abort();

		whitelist[whitelist_size-1].class = line;
		whitelist[whitelist_size-1].text = strchr(line,'\t')+1;
		if (whitelist[whitelist_size-1].text) abort();
		whitelist[whitelist_size-1].path = strchr(text,'\t')+1;
		if (whitelist[whitelist_size-1].path) abort();

		*(whitelist[whitelist_size-1].text - 1) = '\0';
		*(whitelist[whitelist_size-1].path - 1) = '\0';
	}
	fclose(fh);

	HWINEVENTHOOK hook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
		NULL,
		foreground_changed,
		0,
		0,
		WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS
	);
	if (!hook) abort();

	MSG msg;
	BOOL ret;
	while (0 != (ret = GetMessage(&msg, NULL, 0, 0))) {
		if (ret == -1)	{
			abort();
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UnhookWinEvent(hook);

	for (size_t i = 0; i < whitelist_size; i++) free(whitelist[i].class);
	free(whitelist);

	return 0;
}
/*
PlaySound("threat", GetModuleHandle(NULL), SND_RESOURCE);
*/


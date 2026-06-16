#ifndef WEBHANDLERS_H
#define WEBHANDLERS_H
#include <WebServer.h>
#include "Pet.h"
#include "MultiPet.h"
#include "Stats.h"
void registerHandlers(WebServer &server, MultiPetState &multiPet, GameStats &stats);
extern bool showWakeMessage;
extern unsigned long wakeMessageStartTime;
extern String previousState;
WebServer* getServer();
#endif

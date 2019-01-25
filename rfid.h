#ifndef RFID_H_
#define RFID_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfrc522.h"

void initialize_rfid();
char* read_id();

#endif /*RFID_H_*/

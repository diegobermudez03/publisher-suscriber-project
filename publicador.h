#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_LINEAS 100

int segundos_esperar;

int main(int argc, char** argsv);


#endif 

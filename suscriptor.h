#ifndef SUSCRIPTOR_H
#define SUSCRIPTOR_H

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

void ObtenerSuscripciones(char* noticias);
void ObtenerPipe(char* pipe_resultado, char* nombre_pipe, int pid);
void EscucharSuscripcion(char* nombre_pipe);
int main(int argc, char** argsv);

#endif 

#ifndef PUBLICADOR_SUSCRIPTOR_H
#define PUBLICADOR_SUSCRIPTOR_H

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

#define MAX_SUSCRIPTORES 100

struct Suscriptor {
    int pid;
    char categorias[5];
    char* nombre_pipe;
    int fd_pipe;
};

//todas estas variables globales son debido a la concurrencia del programa
//son variables que multiples threads deberan manejar
struct Suscriptor suscriptores[MAX_SUSCRIPTORES]; //máximo 100 suscriptores
int total_suscriptores; //control de cuantos suscriptores tenemos
int abiertos;  //control de publicadores activos
int segundos_esperar;  //segundos a esperar indicados por la flag

//decriptores de estos pipes son globales para que cualquier thread pueda cerrarlos
int fd_sus_solicitud;
int fd_sus_respuesta;
int fd_publicadores;

//semaforos para control de zonas criticas
sem_t mutex_abiertos;
sem_t mutex_noticias;

void DeserializarMensajeSuscriptor(const char* input, char letters[], int* number);
char* GenerarInvitacion(int pid, char* nombre_pipe);
void* ManejarSuscriptores(void* arg);
void* ManejarPublicacion(void* arg);
void* EscucharPublicadores(void* arg);
int main(int argc, char** argsv);

#endif 

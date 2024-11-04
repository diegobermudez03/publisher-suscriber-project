#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>      
#include <semaphore.h>  
#include <time.h>
#include <string.h>

int segundos_esperar;


int main(int argc, char** argsv){
    //validacion de argumentos
    char* nombre_publicadores = NULL;
    char* nombre_suscriptores = NULL;
    int seg_espera = -1;
    if(argc < 7){
        perror("Numero de argumentos invalido");
        exit(1);
    }
    for(int i = 1; i < 7; i++){
        if(strcmp(argsv[i], "-p") == 0){
            printf("si es igual");
            nombre_publicadores = argsv[i+1];
        }
        if(strcmp(argsv[i], "-s") == 0){
            nombre_suscriptores = argsv[i+1];
        }
        if(strcmp(argsv[i], "-t") == 0){
            seg_espera = atoi(argsv[i+1]);
        }
    }
    if(nombre_publicadores == NULL || nombre_suscriptores == NULL || seg_espera == -1){
        perror("Faltan flags");
        exit(1);
    }
    segundos_esperar = seg_espera;

    //creando pipes
    mkfifo(nombre_publicadores, 0666);
    int pub_pipe = open(nombre_publicadores, O_RDONLY);

    mkfifo(nombre_suscriptores, 0666);
    int sus_pipe = open(nombre_suscriptores, O_WRONLY);

    if(pub_pipe == -1 || sus_pipe == -1){
        close(pub_pipe);
        close(sus_pipe);
        perror("Error creando los pipes")
        exit(1);
    }


    //creando hilo que handlea los suscriptores
    pthread_t  hilo_suscriptores;
    pthread_create(&hilo_suscriptores, NULL, ListenToPublishers, (void *)&pub_pipe);
   
}


void* ListenToPublishers(void* arg) {
    int fd_pipe = *(int*)arg;
    char buffer_entrada[500];

    while (1) {
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_read = read(fd_pipe, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_read > 0) {
            buffer_entrada[bytes_read] = '\0';
            
            //Se crea una copia del buffer
            char* buffer_copia = malloc(bytes_read + 1);
            strncpy(buffer_copia, buffer_entrada, bytes_read + 1);
            //se crea un hilo con esa copia del buffer
            pthread_t hilo_publicacion;
            pthread_create(&hilo_publicacion, NULL, ManejarPublicacion, (void*)buffer_copia);
            pthread_detach(hilo_publicacion); 
        }
    }
    close(fd_pipe);
    return NULL;
}rn NULL;
}

void* ManejarPublicacion(void* arg){

}
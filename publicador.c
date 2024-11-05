#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>      
#include <semaphore.h>  
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>


int segundos_esperar;

int main(int argc, char** argsv){
    //validacion de argumentos
    char* nombre_pipe = NULL;
    char* nombre_archivo = NULL;
    int seg_espera = -1;
    const char *pipe_prefix = "/tmp/";
    if(argc < 7){
        perror("Numero de argumentos invalido");
        exit(1);
    }
    for(int i = 1; i < 7; i++){
        if(strcmp(argsv[i], "-p") == 0){
            nombre_pipe = malloc(strlen(argsv[i+1])+ 6);
            strcpy(nombre_pipe, pipe_prefix);
            strcat(nombre_pipe, argsv[i+1]);
        }
        if(strcmp(argsv[i], "-f") == 0){
            nombre_archivo = argsv[i+1];
        }
        if(strcmp(argsv[i], "-t") == 0){
            seg_espera = atoi(argsv[i+1]);
        }
    }
    if(nombre_pipe == NULL || nombre_archivo == NULL || seg_espera == -1){
        perror("Faltan flags");
        exit(1);
    }
    segundos_esperar = seg_espera;

    //abrir pipe
    int fd_pipe =  open(nombre_pipe, O_WRONLY);
    if(fd_pipe == -1){
        perror("error abriendo pipe");
        exit(1);
    }

    //enviar mensaje "abierto" por el pipe, esto indica al sistema que se unio un publicador
    char* mensaje_abierto = "abierto";
    write(fd_pipe, mensaje_abierto, strlen(mensaje_abierto) + 1);

    //abriendo archivo para lectura
    FILE* archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    char* linea = NULL;
    size_t longitud = 0;
    ssize_t leido;

    //se lee el archivo linea a linea y se envian al sistema, esperando entre noticia
    //y noticia el tiempo estipulado por la bandera
    while ((leido = getline(&linea, &longitud, archivo)) != -1) {
        write(fd_pipe, linea, leido + 1);
        printf("\nNoticia enviada %s\n", linea);
        sleep(segundos_esperar);
    }

    //luego de leer todo el archivo se envia un mensaje de cerrado al sistema
    //esto es para que el sepa que un publicador acaba de terminar, y pueda llevar control de si hay publicadores vivos
    char* mensaje_cerrado = "cerrado";
    write(fd_pipe, mensaje_cerrado, strlen(mensaje_cerrado) + 1);

    free(linea);
    fclose(archivo);
    close(fd_pipe);

    return 0;
}

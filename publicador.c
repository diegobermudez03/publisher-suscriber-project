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

#define MAX_LINEAS 100

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

    //abriendo archivo para lectura
    FILE* archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(1);
    }

    char* linea = NULL;
    size_t longitud = 0;
    ssize_t leido;
    char* lineas_validas[MAX_LINEAS];
    int total_lineas = 0;

    //leer el archivo linea a linea y verificar que el formato sea el correcto
    while ((leido = getline(&linea, &longitud, archivo)) != -1) {
        //se verifica la letra inicial y los :
        if ((linea[0] == 'A' || linea[0] == 'P' || linea[0] == 'E' || linea[0] == 'S' || linea[0] == 'C') && linea[1] == ':' && strlen(linea) <= 83) {
            //Si la linea es valida, entonces se añade a las lineas validads
            lineas_validas[total_lineas] = malloc(strlen(linea) + 1);
            strcpy(lineas_validas[total_lineas], linea);
            total_lineas++;
        } else {
            //si la linea no es valida, entonces se muestra el error y finaliza la ejecucion
            perror("El archivo no cumple con el formato");
            free(linea);
            fclose(archivo);
            close(fd_pipe);
            exit(1);
        }
    }

    fclose(archivo);
    free(linea);

    //enviar mensaje "abierto" por el pipe, esto indica al sistema que se unio un publicador
    char* mensaje_abierto = "abierto";
    write(fd_pipe, mensaje_abierto, strlen(mensaje_abierto) + 1);

    //se envian todas las lineas por el pipe, por cada linea se duerme el tiempo estipulado por la flag
    for (int i = 0; i < total_lineas; i++) {
        write(fd_pipe, lineas_validas[i], strlen(lineas_validas[i]) + 1);
        printf("\nNoticia enviada %s\n", lineas_validas[i]);
        sleep(segundos_esperar);
        free(lineas_validas[i]);
    }

    //luego de enviar todas las noticias se envia un mensaje de cerrado
    // esto es para que el sepa que un publicador acaba de terminar, y pueda llevar control de si hay publicadores vivos
    char* mensaje_cerrado = "cerrado";
    write(fd_pipe, mensaje_cerrado, strlen(mensaje_cerrado) + 1);

    close(fd_pipe);

    return 0;
}
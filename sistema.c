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


struct Noticia{
    char nombre;
    char* nombre_pipe;
    int fd_pipe;
};

struct Noticia noticias[5];
int segundos_esperar;

void DeserializarMensajeSuscriptor(const char* input, char letters[], int* number) {
    int i = 0;
    int letter_index = 0;
    memset(letters, 0, 5);
    *number = 0;
    while (input[i] != '\0') {
        if (isalpha(input[i])) {
            if (letter_index < 5) {
                letters[letter_index++] = input[i];
            }
        } else if (isdigit(input[i])) {
            *number = atoi(&input[i]);
            break;
        }
        i++;
    }
}

char* GenerarInvitacion(const char not[5], int pid, struct Noticia noticias[5]) {
    char* resultado = malloc(1024); 
    sprintf(resultado, "%d", pid);
    for (int i = 0; i < 5; i++) {
        if (not[i] != '\0') {
            for (int j = 0; j < 5; j++) {
                if (not[i] == noticias[j].nombre) {
                    strcat(resultado, " ");
                    strcat(resultado, noticias[j].nombre_pipe);
                }
            }
        }
    }
    printf("Se termino de generar invitacion\n");
    return resultado;
}


void* ManejarSuscriptores(void* arg){
    char* nombre_pipe_base = (char*)arg;

    char nombre_pipe_solicitud[100];
    snprintf(nombre_pipe_solicitud, 100, "%ssolicitud", nombre_pipe_base);
    char nombre_pipe_respuesta[100];
    snprintf(nombre_pipe_respuesta, 100, "%srespuesta", nombre_pipe_base);
   
    unlink(nombre_pipe_solicitud);
    if (mkfifo(nombre_pipe_solicitud, 0666) == -1) {
        perror("Error creando el pipe de solicitud");
        exit(1);
    }

    unlink(nombre_pipe_respuesta);
    if (mkfifo(nombre_pipe_respuesta, 0666) == -1) {
        perror("Error creando el pipe de respuesta");
        exit(1);
    }

    int fd_solicitud = open(nombre_pipe_solicitud, O_RDONLY);
    if(fd_solicitud == -1){
        perror("Error abriendo pipe de solicitud");
        exit(1);
    }

    int fd_respuesta = open(nombre_pipe_respuesta, O_WRONLY);
    if(fd_respuesta == -1){
        perror("Error abriendo pipe de respuesta");
        exit(1);
    }

    char buffer_entrada[20];

    while(1){
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd_solicitud, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            printf("recibiendo solicitud %s \n", buffer_entrada);
            buffer_entrada[bytes_leidos] = '\0';
            char letras[5];
            int pid;
            DeserializarMensajeSuscriptor(buffer_entrada, letras, &pid);
            char* mensaje = GenerarInvitacion(letras, pid, noticias);
            write(fd_respuesta, mensaje, strlen(mensaje) + 1);
        }
    }
    close(fd_solicitud);
    close(fd_respuesta);
    return NULL;
}



void* ManejarPublicacion(void* arg){
    char* buffer = (char*)arg;


    free(buffer);
    return NULL;
}


void* EscucharPublicadores(void* arg) {
    char* nombre_pipe = (char*)arg;
    int fd_pipe =  open(nombre_pipe, O_RDONLY);
    char buffer_entrada[500];
    time_t tiempo_inicio, tiempo_actual;

    //para que el pipe sea no bloqueante y poder contabilizar
    fcntl(fd_pipe, F_SETFL, O_NONBLOCK);

    time(&tiempo_inicio);
    while (1) {
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd_pipe, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            //para reiniciar el contador
            time(&tiempo_inicio);

            buffer_entrada[bytes_leidos] = '\0';
            char* buffer_copia = malloc(bytes_leidos + 1);
            strncpy(buffer_copia, buffer_entrada, bytes_leidos + 1);
            
            pthread_t hilo_publicacion;
            pthread_create(&hilo_publicacion, NULL, ManejarPublicacion, (void*)buffer_copia);
        }

        time(&tiempo_actual);
        if (difftime(tiempo_actual, tiempo_inicio) >= segundos_esperar) {
            break;
        }

        //para hacer pequeñas pausas y no estar super pegado al CPU
        usleep(100000);
    }
    close(fd_pipe);
    return NULL;
}



int main(int argc, char** argsv){
    char tipos_noticias[] = {'A', 'E', 'C', 'P', 'S'};
    //validacion de argumentos
    char* nombre_publicadores = NULL;
    char* nombre_suscriptores = NULL;
    int seg_espera = -1;
    const char *pipe_prefix = "/tmp/";
    if(argc < 7){
        perror("Numero de argumentos invalido");
        exit(1);
    }
    for(int i = 1; i < 7; i++){
        if(strcmp(argsv[i], "-p") == 0){
            nombre_publicadores = malloc(strlen(argsv[i+1])+ 6);
            strcpy(nombre_publicadores, pipe_prefix);
            strcat(nombre_publicadores, argsv[i+1]);
        }
        if(strcmp(argsv[i], "-s") == 0){
            nombre_suscriptores = malloc(strlen(argsv[i+1])+ 6);
            strcpy(nombre_suscriptores, pipe_prefix);
            strcat(nombre_suscriptores, argsv[i+1]);
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

    //inicializar pipes noticias
    for(int i = 0; i < 5; i++){
        noticias[i].nombre = tipos_noticias[i];
        noticias[i].nombre_pipe = malloc(7);
        snprintf(noticias[i].nombre_pipe, 7, "/tmp/%c", tipos_noticias[i]);
        unlink(noticias[i].nombre_pipe);    //para eliminar si ya existe, por alguna razon
        if (mkfifo(noticias[i].nombre_pipe, 0666) == -1) {
            perror("Error creando el pipe de noticia");
            exit(1);
        }
        int fd_pipe = open(noticias[i].nombre_pipe, O_RDWR);
        if(fd_pipe == -1){
            perror("Error abriendo  pipe para una categoria");
            exit(1);
        }
        noticias[i].fd_pipe = fd_pipe;
    } 
    
    //creando pipes
    unlink(nombre_publicadores);
    mkfifo(nombre_publicadores, 0666);

    //creando hilo que handlea los publicadores
    pthread_t  hilo_publicadores;
    pthread_create(&hilo_publicadores, NULL, EscucharPublicadores, (void *)nombre_publicadores);

    //creando hilo que handlea los nuevos suscriptores
    pthread_t  hilo_suscriptores;
    pthread_create(&hilo_suscriptores, NULL, ManejarSuscriptores, (void *)nombre_suscriptores);

    //join de threads
    pthread_join(hilo_publicadores, NULL);
    pthread_join(hilo_suscriptores, NULL);

    //liberar memoria
    free(nombre_publicadores);
    free(nombre_suscriptores);
    for(int i = 0; i < 5;i++){
        free(noticias[i].nombre_pipe);
        close(noticias[i].fd_pipe);
    }

    return 0;
}


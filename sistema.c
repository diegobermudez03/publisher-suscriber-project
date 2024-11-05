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

struct Suscriptor {
    int pid;
    char categorias[5];
    char* nombre_pipe;
    int fd_pipe;
};

struct Suscriptor suscriptores[100]; // Máximo 100 suscriptores
int total_suscriptores = 0;
int segundos_esperar;
int abiertos = 0;
int fd_sus_solicitud;
int fd_sus_respuesta;
int fd_publicadores;
sem_t mutex_abiertos;
sem_t mutex_noticias;

void DeserializarMensajeSuscriptor(const char* input, char letters[], int* number) {
    int i = 0;
    int letter_index = 0;
    memset(letters, 0, 5);
    *number = 0;
    while (input[i] != '\0' && isdigit(input[i])) {
        i++;
    }
    if (i > 0) {
        *number = atoi(input);
    }
    while (input[i] != '\0') {
        if (isalpha(input[i])) {
            if (letter_index < 5) {
                letters[letter_index++] = input[i];
            }
        }
        i++;
    }
}

char* GenerarInvitacion(int pid, char* nombre_pipe) {
    char* resultado = malloc(1024); 
    sprintf(resultado, "%d %s", pid, nombre_pipe);
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

    fd_sus_solicitud = open(nombre_pipe_solicitud, O_RDONLY);
    if(fd_sus_solicitud == -1){
        perror("Error abriendo pipe de solicitud");
        exit(1);
    }

    fd_sus_respuesta = open(nombre_pipe_respuesta, O_WRONLY);
    if(fd_sus_respuesta == -1){
        perror("Error abriendo pipe de respuesta");
        exit(1);
    }

    char buffer_entrada[20];

    while(1){
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd_sus_solicitud, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            printf("----recibiendo solicitud %s \n", buffer_entrada);
            buffer_entrada[bytes_leidos] = '\0';
            char letras[5];
            int pid;
            DeserializarMensajeSuscriptor(buffer_entrada, letras, &pid);

            // Crear pipe único para el suscriptor
            struct Suscriptor nuevo_suscriptor;
            nuevo_suscriptor.pid = pid;
            strncpy(nuevo_suscriptor.categorias, letras, 5);
            nuevo_suscriptor.nombre_pipe = malloc(100);
            snprintf(nuevo_suscriptor.nombre_pipe, 100, "/tmp/suscriptor%d", pid);
            unlink(nuevo_suscriptor.nombre_pipe);
            if (mkfifo(nuevo_suscriptor.nombre_pipe, 0666) == -1) {
                perror("Error creando el pipe para el suscriptor");
                exit(1);
            }
            nuevo_suscriptor.fd_pipe = open(nuevo_suscriptor.nombre_pipe, O_RDWR);
            if (nuevo_suscriptor.fd_pipe == -1) {
                perror("Error abriendo pipe para el suscriptor");
                exit(1);
            }

            // Agregar el suscriptor a la lista
            sem_wait(&mutex_noticias);
            suscriptores[total_suscriptores++] = nuevo_suscriptor;
            sem_post(&mutex_noticias);
            // Enviar invitación al suscriptor
            char* mensaje = GenerarInvitacion(pid, nuevo_suscriptor.nombre_pipe);
            printf("\n----invitacion enviada %s\n\n", mensaje);
            write(fd_sus_respuesta, mensaje, strlen(mensaje) + 1);
            free(mensaje);
        }
    }
    close(fd_sus_solicitud);
    close(fd_sus_respuesta);
    return NULL;
}

void* ManejarPublicacion(void* arg) {
    char* buffer = (char*)arg;

    // Verificar si el mensaje es "abierto" o "cerrado"
    if (strcmp(buffer, "abierto") == 0) {
        sem_wait(&mutex_abiertos);
        abiertos++;
        sem_post(&mutex_abiertos);
    } else if (strcmp(buffer, "cerrado") == 0) {
        sem_wait(&mutex_abiertos);
        abiertos--;
        int abiertos_local = abiertos;
        sem_post(&mutex_abiertos);

        if (abiertos_local == 0) {
            sleep(segundos_esperar);

            sem_wait(&mutex_abiertos);
            if (abiertos == 0) {
                sem_wait(&mutex_noticias);
                for (int i = 0; i < total_suscriptores; i++) {
                    char mensaje_fin[100];
                    snprintf(mensaje_fin, sizeof(mensaje_fin), "0:Se acabaron las noticias :(");
                    write(suscriptores[i].fd_pipe, mensaje_fin, strlen(mensaje_fin) + 1);
                    close(suscriptores[i].fd_pipe);
                }
                close(fd_publicadores);
                close(fd_sus_respuesta);
                close(fd_sus_solicitud);
                sem_post(&mutex_noticias);
                sem_post(&mutex_abiertos);
                free(buffer);
                exit(0);
            }
            sem_post(&mutex_abiertos);
        }
    } else {
        char llave = buffer[0];

        sem_wait(&mutex_noticias);
        for (int i = 0; i < total_suscriptores; i++) {
            for (int j = 0; j < 5; j++) {
                if (suscriptores[i].categorias[j] == llave) {
                    write(suscriptores[i].fd_pipe, buffer, strlen(buffer) + 1);
                    printf("Enviando noticia a suscriptor %d: %s\n", suscriptores[i].pid, buffer);
                    break;
                }
            }
        }
        sem_post(&mutex_noticias);
    }
    free(buffer);
    return NULL;
}

void* EscucharPublicadores(void* arg) {
    char* nombre_pipe = (char*)arg;
    fd_publicadores =  open(nombre_pipe, O_RDONLY);
    char buffer_entrada[500];

    while (1) {
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd_publicadores, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            char* buffer_copia = malloc(bytes_leidos + 1);
            strncpy(buffer_copia, buffer_entrada, bytes_leidos + 1);
            
            pthread_t hilo_publicacion;
            pthread_create(&hilo_publicacion, NULL, ManejarPublicacion, (void*)buffer_copia);
        }
    }
    close(fd_publicadores);
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
    
    //creando pipes
    unlink(nombre_publicadores);
    mkfifo(nombre_publicadores, 0666);

    sem_init(&mutex_abiertos, 0, 1);
    sem_init(&mutex_noticias, 0, 1);


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

    return 0;
}


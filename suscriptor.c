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
#include <unistd.h>

void ObtenerSuscripciones(char* noticias){
    int contador = 0;
    while(1){
        char ch;
        printf("\nIngresar letra o 0: ");
        ch = getchar();

        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        if(ch == '0'){
            if(noticias[0] == 'x'){
                printf("Seleccione al menos 1 categoria\n");
            }else{
                break;
            }
        }else{
            int ya_estaba = 0;
            for(int i = 0; i < 5; i++){
                if(noticias[i] == ch){
                    printf("Ya esta seleccionada esa categoria\n");
                    ya_estaba = 1;
                    break;
                }
            }
            if(ya_estaba == 0){
                if(ch > 96) ch -= 32;
                if(ch != 'A' && ch != 'E' && ch != 'C' && ch != 'P' && ch != 'S'){
                    printf("Opcion  no valida\n");
                    continue;
                }
                noticias[contador] = ch;
                contador++;
                if(contador == 5) break;
            }
        }
    }
}

void ObtenerPipe(char* pipe, int fd, int pid) {
    char buffer_entrada[200];
    while(1) {
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            int numero;
            char* endptr;
            numero = (int)strtol(buffer_entrada, &endptr, 10);
            // Si el número al inicio del mensaje es nuestro pid, es la respuesta que esperábamos
            if(numero == pid) {
                printf("\nRespuesta recibida %s\n\n", buffer_entrada);
                char* pipe_start = strchr(endptr, '/');
                if (pipe_start != NULL) {
                    strncpy(pipe, pipe_start, strlen(pipe_start) + 1);
                }
                break;
            }
        }
    }
}

void EscucharSuscripcion(char* nombre_pipe){
    int fd = open(nombre_pipe, O_RDONLY);
    if (fd == -1) {
         perror("Error con suscription");
         return;
    }
    char buffer_entrada[81];
    while(1){
         memset(buffer_entrada, 0, sizeof(buffer_entrada));
         ssize_t bytes_leidos = read(fd, buffer_entrada, sizeof(buffer_entrada) - 1);
         if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            if (strncmp(buffer_entrada, "0:", 2) == 0) {
                printf("%s\n", buffer_entrada + 2);
                close(fd);
                return;
            } else {
                printf("****Recibiendo noticia: %s \n", buffer_entrada);
            }
         }
    }
}

int main(int argc, char** argsv){
    char* nombre_pipe = NULL;
    char noticias[] = {'x', 'x', 'x', 'x', 'x'};
    if(argc < 3){
        perror("Numero de argumentos invalido");
        exit(1);
    }
    if(strcmp(argsv[1], "-s") != 0){
        perror("Flag no valida");
        exit(1);
    }
    nombre_pipe = argsv[2];

    // Crear nombres para los pipes de solicitud y respuesta
    char nombre_pipe_solicitud[100];
    snprintf(nombre_pipe_solicitud, 100, "/tmp/%ssolicitud", nombre_pipe);
    char nombre_pipe_respuesta[100];
    snprintf(nombre_pipe_respuesta, 100, "/tmp/%srespuesta", nombre_pipe);

    // Abrir el pipe de solicitud para escritura
    int fd_solicitud = open(nombre_pipe_solicitud, O_WRONLY);
    if(fd_solicitud == -1){
        perror("Error al abrir el pipe de solicitud");
        exit(1);
    }

    printf("Escribe la letra de los topicos a suscribirse:\n");
    printf("A: Noticia de Arte\n");
    printf("E: Noticia de Farandula y Espectaculos\n");
    printf("C: Noticia de Ciencia\n");
    printf("P: Noticia de Politica\n");
    printf("S: Noticia de Sucesos\n");
    printf("0: YA NO MAS\n\n");

    ObtenerSuscripciones(noticias);

    // Crear el mensaje de suscripcion con el PID y las categorias seleccionadas
    pid_t pid = getpid();
    char* mensaje = malloc(64);
    sprintf(mensaje, "%d", pid);
    for (int i = 0; i < 5; i++) {
        if (noticias[i] != 'x') {
            strncat(mensaje, &noticias[i], 1);
        }
    }
    write(fd_solicitud, mensaje, strlen(mensaje) + 1);
    
    close(fd_solicitud);
    free(mensaje);

    // Ahora esperamos a una respuesta del sistema, que contendra el nombre del pipe unico para este suscriptor
    char pipe_suscripcion[100];

    // Abrir el pipe de respuesta para lectura
    int fd_respuesta = open(nombre_pipe_respuesta, O_RDONLY);
    if(fd_respuesta == -1){
        perror("Error al abrir el pipe de respuesta");
        exit(1);
    }

    ObtenerPipe(pipe_suscripcion, fd_respuesta, pid);
    close(fd_respuesta);

    // Escuchar las noticias a traves del pipe unico para este suscriptor
    EscucharSuscripcion(pipe_suscripcion);

    return 0;
}

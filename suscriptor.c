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

pthread_t hilos_tema[5];

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

void ObtenerPipes(char** pipes, int fd, int pid){
    char buffer_entrada[200];
    while(1){
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            int numero;
            char* endptr;
            numero = (int)strtol(buffer_entrada, &endptr, 10);
            //si el numero al inicio del mensaje es nuestro pid es porque esta es la respuesta que esperabamos
            if(numero == pid){
                printf("\nRespusta recibida %s\n\n", buffer_entrada);
                char* posicion = endptr;
                char* pipe_temp;
                int index = 0;
                while ((pipe_temp = strtok(posicion, " ")) != NULL && index < 5) {
                    pipes[index++] = strdup(pipe_temp);
                    posicion = NULL;
                }
                break;
            }
        }
    }
}

void* EscucharSuscripcion(void* arg){
    char* nombre_pipe = (char*)arg;
    int fd = open(nombre_pipe, O_RDONLY);
    if (fd == -1) {
         perror("Error con suscription");
         return NULL;
    }
    char buffer_entrada[81];
    while(1){
         memset(buffer_entrada, 0, sizeof(buffer_entrada));
         ssize_t bytes_leidos = read(fd, buffer_entrada, sizeof(buffer_entrada) - 1);
         if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            if (strncmp(buffer_entrada, "0:", 2) == 0) {
                printf("%s\n", buffer_entrada + 2);
                return NULL;
            } else {
                printf("Recibiendo noticia: %s \n", buffer_entrada);
            }
         }
    }
    return NULL;
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

    int contador = 0;
    ObtenerSuscripciones(noticias);

    //despues de obtener todas las categorias a suscribirse entonces creamos el mensaje a enviar para suscribirnos
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

    //Ahora esperamos a una respuesta del sistema, hasta que encontremos la que sea para nosotros, identificamos esto por el PID
    char buffer_entrada[200];
    char* pipes_suscripciones[5] = {"x", "x", "x", "x", "x"};

    // Abrir el pipe de respuesta para lectura
    int fd_respuesta = open(nombre_pipe_respuesta, O_RDONLY);
    if(fd_respuesta == -1){
        perror("Error al abrir el pipe de respuesta");
        exit(1);
    }


    ObtenerPipes(pipes_suscripciones, fd_respuesta, pid);
    close(fd_respuesta);

    int hilo_index = 0;
    for(int i = 0; i < 5; i++){
        if(strcmp(pipes_suscripciones[i],"x") != 0){
            pthread_create(&hilos_tema[hilo_index++], NULL, EscucharSuscripcion, (void *)pipes_suscripciones[i]);
        }
    }

    for(int i = 0; i < hilo_index; i++){
        pthread_join(hilos_tema[i], NULL);
    }

    return 0;
}

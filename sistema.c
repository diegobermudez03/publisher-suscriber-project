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

#include "sistema.h"

//funcion que se encarga de deserializar el mensaje recibido por los suscriptores, se encarga de 
//extraer el pid y las categorias
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

//funcion que se encarga de generar la invitacion a los suscriptores
//simplemente junta el pid con el nombre del pipe
char* GenerarInvitacion(int pid, char* nombre_pipe) {
    char* resultado = malloc(1024); 
    sprintf(resultado, "%d %s", pid, nombre_pipe);
    return resultado;
}


//funcion que se encarga de manejar a los nuevos suscriptores, es decir, solicitudes de suscripcion
void* ManejarSuscriptores(void* arg){
    //con el nombre base del pipe recibido por la flag, se crean y abren los pipes
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

    //se escucha indefinidamente por el pipe de solicitudes
    char buffer_entrada[20];
    while(1){
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd_sus_solicitud, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            printf("----recibiendo solicitud %s \n", buffer_entrada);
            buffer_entrada[bytes_leidos] = '\0';
            char letras[5];
            int pid;
            //se deserializa el mensaje, que es, obtener el pid y las categorias a las que se quiere suscribir
            DeserializarMensajeSuscriptor(buffer_entrada, letras, &pid);

            //se crea un pipe personal para ese nuevo suscriptor
            struct Suscriptor nuevo_suscriptor;
            nuevo_suscriptor.pid = pid;
            strncpy(nuevo_suscriptor.categorias, letras, 5);
            nuevo_suscriptor.nombre_pipe = malloc(100);
            snprintf(nuevo_suscriptor.nombre_pipe, 100, "/tmp/suscriptor%d", pid);
            unlink(nuevo_suscriptor.nombre_pipe);
            if (mkfifo(nuevo_suscriptor.nombre_pipe, 0666) == -1) {
                perror("Error creando el pipe para el suscriptor");
            }
            nuevo_suscriptor.fd_pipe = open(nuevo_suscriptor.nombre_pipe, O_RDWR);
            if (nuevo_suscriptor.fd_pipe == -1) {
                perror("Error abriendo pipe para el suscriptor");
            }

            //ZONA CRITICA, se agrega el nuevo suscriptor al areglo de suscriptores globales
            sem_wait(&mutex_noticias);
            suscriptores[total_suscriptores++] = nuevo_suscriptor;
            sem_post(&mutex_noticias);

            //se genera y envia la invitacion al suscriptor
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


//funcion que se encarga de manejar las publicaciones
//se crea en un nuevo thread para cada publicacion recibida
void* ManejarPublicacion(void* arg) {
    char* buffer = (char*)arg;  //el argumento es el contenido del mensaje

    //verifica si el mensaje es que se abrio un nuevo publicador
    //si es asi entonces se afecta el contador de publicadores sumando uno, como es zona critica se usa un semaforo para control
    if (strcmp(buffer, "abierto") == 0) {
        sem_wait(&mutex_abiertos);
        abiertos++;
        sem_post(&mutex_abiertos);
    } else if (strcmp(buffer, "cerrado") == 0) {
        //si el mensaje indica que se cerro un publicador, se actualiza la variable, como es global entonces es zona critica
        //se usa semaforo
        sem_wait(&mutex_abiertos);
        abiertos--;
        int abiertos_local = abiertos;
        sem_post(&mutex_abiertos);

        //si despues de que le restamos 1 al control, vemos que ahora esta en 0
        //quiere decir que no hay publicadores activos, asi que es deber de este thread esperar a ver si entran nuevos
        if (abiertos_local == 0) {
            sleep(segundos_esperar); //dormimos el tiempo estipulado por la flag

            sem_wait(&mutex_abiertos);
            //si despues de dormir (esperar el numero de segundos), la variable sigue en 0, es que no entraron
            //nuevos publicadores, asi que si es nuestra tarea acabar con todo
            if (abiertos == 0) {
                //ZONA CRITICA, manejo de la info de los suscriptores
                sem_wait(&mutex_noticias);
                for (int i = 0; i < total_suscriptores; i++) {
                    //se le notifica a los suscriptores que se acabaron los mensajes y se cierra el pipe de cada uno
                    char mensaje_fin[100];
                    printf("\nInformando acerca de funalizacion a suscriptor %d", suscriptores[i].pid);
                    snprintf(mensaje_fin, sizeof(mensaje_fin), "0:Se acabaron las noticias :(");
                    write(suscriptores[i].fd_pipe, mensaje_fin, strlen(mensaje_fin) + 1);
                    close(suscriptores[i].fd_pipe);
                }
                //por esto eran variables globales, para poder cerrar esos pipes aca, y se finaliza la ejecucion
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
        //si no era ni abierto ni cerrado entonces era un mensaje normal, por lo que se maneja normal
        char llave = buffer[0];

        //ZONA CRITICA, asi que se usa el semaforo para prevenir afectacion
        sem_wait(&mutex_noticias);
        for (int i = 0; i < total_suscriptores; i++) {
            for (int j = 0; j < 5; j++) {
                //si el usuario esta suscritor a la categoria del mensaje entonces se le envia la noticia a su pipe personal
                if (suscriptores[i].categorias[j] == llave) {
                    write(suscriptores[i].fd_pipe, buffer, strlen(buffer) + 1);
                    printf("Enviando noticia a suscriptor %d: %s\n", suscriptores[i].pid, buffer);
                    break;
                }
            }
        }
        sem_post(&mutex_noticias);  //FIN DE ZONA CRITICA
    }
    free(buffer);
    return NULL;
}


//funcion que escucha a los publicadores, es decir, escucha las publicaciones
//que estos envian
void* EscucharPublicadores(void* arg) {
    char* nombre_pipe = (char*)arg;
    fd_publicadores =  open(nombre_pipe, O_RDONLY);
    if(fd_publicadores == -1){
        perror("Error abriendo pipe de publicadores");
        exit(1);
    }
    char buffer_entrada[500];

    //se lee indefinidamente
    while (1) {
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd_publicadores, buffer_entrada, sizeof(buffer_entrada) - 1);
        //cuando se recibe un mensaje simplemente se crea un thread para manejar esa publicacion
        //esto es porque el proceso de enviar las publicaciones puede tardar tiempo, pero no 
        //queremos dejar de escuchar a las nuevas publicaciones
        if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            //IMPORTANTE, se crea una copia del contenido y es esa la que se le envia al thread
            //para poder seguir recibiendo mensajes y no afectar los threads manejando publicaciones
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
    //validacion de argumentos
    char* nombre_publicadores = NULL;
    char* nombre_suscriptores = NULL;
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
            segundos_esperar = atoi(argsv[i+1]);
        }
    }
    if(nombre_publicadores == NULL || nombre_suscriptores == NULL || segundos_esperar == -1){
        perror("Faltan flags");
        exit(1);
    }

    //se inicializan los semadoros con 1 cupo inicial
    sem_init(&mutex_abiertos, 0, 1);
    sem_init(&mutex_noticias, 0, 1);
    
    //se crea  el pipe de publicadores, solo es 1, por donde los publicadores enviaran sus noticias
    unlink(nombre_publicadores);
    mkfifo(nombre_publicadores, 0666);


    //creando hilo que handlea los publicadores
    pthread_t  hilo_publicadores;
    pthread_create(&hilo_publicadores, NULL, EscucharPublicadores, (void *)nombre_publicadores);

    //creando hilo que handlea las solicitudes de  nuevos suscriptores
    pthread_t  hilo_suscriptores;
    pthread_create(&hilo_suscriptores, NULL, ManejarSuscriptores, (void *)nombre_suscriptores);

    //join de threads para esperar a que los hilos terminen
    pthread_join(hilo_publicadores, NULL);
    pthread_join(hilo_suscriptores, NULL);

    //liberar memoria
    free(nombre_publicadores);
    free(nombre_suscriptores);
    return 0;
}


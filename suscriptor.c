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

#include "suscriptor.h"


//funcion que se encarga de obtener las noticias de suscripcion por pantalla
void ObtenerSuscripciones(char* noticias){
    int contador = 0;
    while(1){
        char ch;
        printf("\nIngresar letra o 0: ");
        ch = getchar();

        int c;
        while ((c = getchar()) != '\n' && c != EOF);    //para ignorar todos los caracteres adicionales, solo se toma el primero
        //si se ingresó 0 pero no se ha suscrito al menos a 1, entonces se indica, si no se acaba
        if(ch == '0'){
            if(noticias[0] == 'x'){
                printf("Seleccione al menos 1 categoria\n");
            }else{
                break;
            }
        }else{
            if(ch > 96) ch -= 32;   //para aceptar tanto minusculas como mayusculas, se convierte a mayuscula si era min
            //se verifica si la categoria ya habia sido seleccionada, si si pues se indica
            int ya_estaba = 0;
            for(int i = 0; i < 5; i++){
                if(noticias[i] == ch){
                    printf("Ya esta seleccionada esa categoria\n");
                    ya_estaba = 1;
                    break;
                }
            }
            //si no estaba seleccionada entonces se verifica que sea valida y si si se agrega
            if(ya_estaba == 0){
                if(ch != 'A' && ch != 'E' && ch != 'C' && ch != 'P' && ch != 'S'){
                    printf("Opcion  no valida\n");
                    continue;
                }
                noticias[contador] = ch;
                contador++;
                if(contador == 5) break;    //si se alcanzaron 5 categorias ya estan todas las posibles, se acaba
            }
        }
    }
}

//funcion que se encarga de esperar la respuesta del sistema a nuestra solicitud, el mensaje deberia incluir
//el nombe del pid creado para nosotros
void ObtenerPipe(char* pipe_resultado, char* nombre_pipe, int pid) {
    //se abre el pipe de respuesta de solicitudses
    int fd = open(nombre_pipe, O_RDONLY);
    if (fd == -1) {
         perror("Error al abrir el pipe de respuesta");
         return;
    }
    char buffer_entrada[200];
    //se lee indefinidamente los mensajes encontrados en el pipe
    while(1) {
        memset(buffer_entrada, 0, sizeof(buffer_entrada));
        ssize_t bytes_leidos = read(fd, buffer_entrada, sizeof(buffer_entrada) - 1);
        if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            int numero;
            char* endptr;
            numero = (int)strtol(buffer_entrada, &endptr, 10);
            //si el numero inicial del mensaje es nuestro PID, entonces esta si es nuestra respuesa
            //si no es, entonces era la de alguien mas y la ignoramos y seguimos esperando
            if(numero == pid) {
                printf("\nRespuesta recibida %s\n\n", buffer_entrada);
                char* pipe_start = strchr(endptr, '/');
                if (pipe_start != NULL) {
                    //guardamos en el pipe_resultado el nombre del pipe recibido por el que debemos escuchar
                    strncpy(pipe_resultado, pipe_start, strlen(pipe_start) + 1);
                }
                close(fd);
                break;
            }
        }
    }
}


//funcion que escucha en el pipe creado para nosotros, por donde se recibiran las noticias
//a las que estamos suscritos
void EscucharSuscripcion(char* nombre_pipe){
    //abrimos nuestro pipe personal por el que nos enviaran las noticias
    int fd = open(nombre_pipe, O_RDONLY);
    if (fd == -1) {
         perror("Error con suscription");
         return;
    }
    char buffer_entrada[84];    //buffer de 84 pues el maximo de una noticia (el texto) es de 80 + "A: " y \0

    while(1){
         memset(buffer_entrada, 0, sizeof(buffer_entrada));
         ssize_t bytes_leidos = read(fd, buffer_entrada, sizeof(buffer_entrada) - 1);
         if (bytes_leidos > 0) {
            buffer_entrada[bytes_leidos] = '\0';
            //si el mensaje inicia por "0:" es porque es el mensaje indicando que se acabaron las noticias, entonces mostramos por pantalla
            //cerramos el pipe y dejamos de escuchar
            if (strncmp(buffer_entrada, "0:", 2) == 0) {
                printf("%s\n", buffer_entrada + 2);
                close(fd);
                return;
            } else {
                //si no era mensaje de terminacion simplemente imprimimos la noticia
                printf("****Recibiendo noticia: %s \n", buffer_entrada);
            }
         }
    }
}

int main(int argc, char** argsv){
    //validacion de argumentos
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

    //el nomrbe ingresado por la flag solo es la base, puesto que se necesitan 2 pipes distintos
    //uno de escritura y otro de lectura, tan solo para el handshake de las 2 partes
    char nombre_pipe_solicitud[100];
    snprintf(nombre_pipe_solicitud, 100, "/tmp/%ssolicitud", nombre_pipe);
    char nombre_pipe_respuesta[100];
    snprintf(nombre_pipe_respuesta, 100, "/tmp/%srespuesta", nombre_pipe);

    //se abre el pipe de solicitud, se hace antes de si quiera mostrar el menu
    //puesto que si no se abre con exito no tiene caso intentar
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

    //se obtiene el pid del proceso, este sera el identificador unico o llave por la que se comunicaran
    //y se envia el mensaje junto con las categorias a suscribirse
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

    //ahora nos vamos a escuchar el pipe de respuesta
    char pipe_suscripcion[100];
    ObtenerPipe(pipe_suscripcion, nombre_pipe_respuesta, pid);

    //Si llegamos aca implica que vamos bien, entonces vamos a escuchar el pipe de respuesta que
    //nos envio el sistema para escuchar las noticias
    EscucharSuscripcion(pipe_suscripcion);

    return 0;
}

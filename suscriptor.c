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

    int fd = open(nombre_pipe, O_RDWR);
    if(fd == -1){
        perror("Error al abrir el pipe");
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
    while(1){
        char ch;
        printf("\nIngresar letra o 0: ");
        ch = getchar();
        if(ch == '0'){
            if(noticias[0] == 'x'){
                printf("Seleccione al menos 1 categoria\n");
            }else{
                break;
            }
        }else{
            noticias[contador] = ch;
            contador++;
        }
    }
    

    return 0;
}
#flags de threads y el compilador que es gcc
CC = gcc
CFLAGS = -Wall -pthread -lrt

#las fuentes de codigo
PUBLICADOR_SRC = publicador.c
SUSCRIPTOR_SRC = suscriptor.c
SISTEMA_SRC = sistema.c

#los archivos tipo objeto
PUBLICADOR_OBJ = $(PUBLICADOR_SRC:.c=.o)
SUSCRIPTOR_OBJ = $(SUSCRIPTOR_SRC:.c=.o)
SISTEMA_OBJ = $(SISTEMA_SRC:.c=.o)

#los nombres de los ejecutables, los que se usaran
PUBLICADOR_EXE = publicador
SUSCRIPTOR_EXE = suscriptor
SISTEMA_EXE = sistema

all: $(PUBLICADOR_EXE) $(SUSCRIPTOR_EXE) $(SISTEMA_EXE)

#compilar publicador
$(PUBLICADOR_EXE): $(PUBLICADOR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

#compilar suscriptor
$(SUSCRIPTOR_EXE): $(SUSCRIPTOR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

#somcpilar ssitema
$(SISTEMA_EXE): $(SISTEMA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

#compila los archivos de tipo objeto
%.o: %.c
	$(CC) $(CFLAGS) -c $<

#en caso de clean entonces elimina todo
clean:
	rm -f *.o $(PUBLICADOR_EXE) $(SUSCRIPTOR_EXE) $(SISTEMA_EXE)

.PHONY: all clean

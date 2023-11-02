/**
 * @file
 * @brief Proceso servidor.
 * @author Jesus David Cardenas Sandoval <jcardenass@unicauca.edu.co>
 */
#include <arpa/inet.h>
#include "concurrencia.h"
#include "chat.h"
#include <errno.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFS 20
#define MAX_MESSAGE 200

/**
 * @brief Cliente conectado.
 */
typedef struct {
	pthread_t thread; /*!< ID de hilo (biblioteca) */
    char username[BUFS]; /*!< Nombre de usuario */
	int socket; /*!< Socket del cliente  - accept */
	int id; /*!< Posicion del cliente en el arreglo de clientes*/
} client;

/** @brief Estructua de un mensaje*/
typedef struct {
    status state; /*!< estado del usuario>*/
	char message[MAX_MESSAGE]; /*!< Mensaje a enviar>*/
    char username[BUFS]; /*!< Nombre de usuario>*/
    char usernamePrivate[BUFS]; /*!< Nombre de usuario destino>*/
} message;

/** 
 * @brief Adiciona un nuevo cliente al arreglo de clientes.
 * @param Socket del cliente.
 * @return Posicion del cliente en el arreglo, -1 si no se adicion.
 */
int register_client(int c);

/**
 * @brief Elimina un cliente.
 * @param id Identificador del cliente (posicion en el arreglo de clientes)
 * @return 0 si se elimino correctamente, -1 si ocurrio un error.
 */
int remove_client(int id);

/** 
 * @brief Subrutina manejadora de señal. 
 * @param signum Numero de la señal recibida.
 */
void signal_handler(int signum);

/** 
* @brief Hilo que atiende a un cliente.
* @param arg Cliente (client*)
*/
void * client_handler(void * arg);

/** 
 * @brief SI el estado del mensaje corresponde a un commando , 
 * lo ejecutara , si no actura por defecto. 
 * @param id id del usuario
 * @param socket socket del usuario
 * @param ms mensaje recibido por el servidor
 * @return 0 si se realizo
 */
int executeAction(int id,int socket,message *ms);

/** @brief Indica la terminacion del servidor. */
int finished;

/** @brief Arreglo de clientes */
client * clients;

/** @brief Capacidad del arreglo de clientes */
int maxclients;

/** @brief Cantidad de clientes */
int nclients;

/** @brief Semaforo para garantizar la exclusion mutua*/
semaphore mutex;

/** @brief Semaforo para garantizar la eliminacion*/
semaphore rem;

semaphore lec;



int main(int argc, char *argv[]) {

	int s;
	int c;
	int i;
    int reuse = 1234;
	int serverPort;
	struct sigaction act;
	struct sockaddr_in addr;
	struct sockaddr_in caddr;
	socklen_t clilen;
   


	//Instalar manejadores de las señales SIGINT, SIGTERM
	//Rellenar la estructura con ceros.
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = signal_handler;

	//instalar el manejador para SIGINT
	if (sigaction(SIGINT, &act, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	//instalar el manejador para SIGINT
	if (sigaction(SIGTERM, &act, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}



    //validar arg de lineas de comando
    if(argc != 2){
        fprintf(stderr, "Debe espeficar el puerto del servidor", argv[0]);
        exit(EXIT_FAILURE);
    }

   	serverPort = atoi(argv[1]);
    //Inicializar los semaforos
    sem_init(&mutex, 0, 0);
	sem_init(&rem, 0, 1);
	sem_init(&lec,0,1);
	//1. Obtener un socket
	//s = socket(..)
	//Obtener un socket para IPv4 conexion de flujo
	s = socket(AF_INET, SOCK_STREAM, 0);

	 if (s < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
        //reutilización de la dirección local del socket
    if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(int)) < 0){
        perror("setsockopt");
        close(s);
        exit(EXIT_FAILURE);
    }

	//2. Asociar el socket a una direccion (remota, IPv4)
	//2.1 Configurar la direccion
	// Llenar la direccion con ceros
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	//TODO el puerto es un argumento de linea de comandos
	addr.sin_port = htons(serverPort);
	if (inet_aton("0.0.0.0", &addr.sin_addr) == 0) {
		perror("inet_aton");
		close(s);
		exit(EXIT_FAILURE);
	}
	//2.2 Asociar el socket a la direccion
	if (bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0) {
		perror("bind");
		close(s);
		exit(EXIT_FAILURE);
	}

	//3. Marcar el socket como disponible
	if (listen(s, 10) != 0) {
		perror("listen");
		close(s);
		exit(EXIT_FAILURE);
	}


	//4. Esperar la conexion de un cliente
	//El servidor obtiene una copia del socket conectado al cliente
	printf("Waiting for connections... Press Ctrl + C to finished\n");

	finished = 0;

	maxclients = 0;
	nclients = 0;
	clients = NULL;

	while(!finished) {
		
		clilen = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *)&caddr, &clilen);
		if (c < 0) {
			if (errno == EINTR) {
				continue;
			}else{
				finished = 1;
				continue;;
			}
		}
		up(&mutex);
		//TODO crear un cliente usando una seccion critica
		if (register_client(c) < 0) {
			fprintf(stderr, "Couldn't register the new client!\n");
			close(c);
			continue;
		}
        down(&mutex);
	}

	printf("Server finished.\n");
	//7. Liberar el socket del servidor
	
	message msg;
	msg.state = EXIT;
	//Esperar que los hilos terminen
	for (i = 0; i < maxclients; i++) {
		if (clients[i].socket > 0) {
			if(write(clients[i].socket, &msg, sizeof(message)) == -1){
				perror("write");
			}
			close(clients[i].socket);
		}
	}
 	free(clients);  
	close(s);
	exit(EXIT_SUCCESS);
}

void signal_handler(int signum) {
	finished = 1;
}

int register_client(int socket) {
	int i;
    int result;
	char buf[BUFS];
	client * new_clients; 
	if (nclients == maxclients) {
		//Crecer el arreglo de clientes
		if (maxclients > 0) {
			maxclients = (maxclients * 3)/2;
		}else {
			maxclients = 10;
		}
		
		new_clients = (client*)malloc(maxclients * sizeof(client));
			 
		if (new_clients == NULL) {
			perror("malloc");
			return -1;
		}

		//Rellenar todo el arreglo de ceros.
		memset(new_clients, 0, maxclients * sizeof(client));
	
		for (i = 0; i<maxclients;i++) {
			new_clients[i].socket = -1; //Marcar el socket como NO USADO
            strcpy(new_clients[i].username,""); //Marcar el user en blanco
		}

		//Si existian clientes antes, copiar los datos al nuevo arreglo
		if (nclients > 0) {
			memcpy(new_clients, clients, nclients * sizeof(client));
		}
		free(clients);
		clients = new_clients;
	}

    result = read(socket, buf, BUFS);
    if (result == -1) {
        perror("read");
        return -1;
    }
    
	//validar si el usuario existe
	int num = existsUsername(buf);

	if(write(socket,&num,sizeof(num)) < 0){
        perror("write");
		return -1;
    }

    if(num == -1){
        return -1;
    }
    
    //Pre: El arreglo clients tiene espacios disponibles.
	//Buscar una posicion disponible y registrar el cliente.
	for (i = 0; i<maxclients; i++) {
		if (clients[i].socket == -1) {
			clients[i].socket = socket;
			clients[i].id = i;
            strcpy(clients[i].username,buf); 
			nclients++; //Incrementar la cantidad de clientes registrados
			pthread_create(&clients[i].thread, 
			NULL, 
			client_handler, 
			(void*)&clients[i]
			);
			return i;
			}
		}
	return -1;
}

int existsUsername(char username[BUFS]){
    if(nclients == 0){
        return 1;
    }else {
        for(int i = 0 ; i < maxclients ; i++){
            if(clients[i].socket != -1){
                if(strcmp(clients[i].username,username) == 0){
                    return -1;
                }
            }
        }
    }
    return 1;
}

int remove_client(int id) {
    down(&rem);
	printf("Disconnected %s \n",clients[id].username);
	close(clients[id].socket);
	clients[id].socket = -1;
	strcpy(clients[id].username,"");
	nclients--;
    up(&rem);
	return 0;
}

void * client_handler(void * arg) {
	down(&lec);
	client * cl = (client*)arg;
	int id = cl->id;
	int socket = cl->socket;
	message ms;
	int n;
	//((client*)arg)-> username
	printf("Client %s registered\n", cl->username);
	strcpy(ms.username, cl-> username);
	strcpy(ms.message,"Connected\n");
	ms.state = ONLINE;
	//enviar ms sobre el usuario que acaba de entrar al chat 
	//a todos los usuarios conectados
	if(executeAction(id,socket,&ms) == -1){
		perror("Action failed");
	}
	up(&lec);
	while (1) {
		if(clients[id].socket == -1 ){
			break;
		}
		n = read(socket, &ms, sizeof(message));
        if (n == -1) {
            perror("Error reading from server");
            break;
        }
        if (n == 0) {
			
            break;
		}
		down(&lec);
		//ejecutar accion dependiendo del estado del mensaje   
		if(executeAction(id,socket,&ms) == -1){
			perror("Action failed");
			up(&lec);
			break;
		}
		up(&lec);
    }
}

//ambos reciben sigpipe si la lectura fallo o sea -1 hacer sigpipe
int executeAction(int id,int socket, message *ms){
	switch (ms->state)
	{
		//Comportamiento por defecto, enviar ms a todos los usuarios conectados
		case ONLINE:
		up(&lec);
			for(int i = 0; i < maxclients ; i++){
				if(clients[i].socket != -1){
					if (write(clients[i].socket, ms, sizeof(message)) == -1) {
						down(&lec);
						return -1;
					}
				}
			}
		down(&lec);
		break;

		//Enviar un mensaje privado a un usuario
		case PRIVATE:
			up(&lec);
			int flag = 0;
			ms->state = PRIVATE;
			//enviar mensaje al usuario de destino
			for(int i = 0; i < maxclients ; i++){
				if(clients[i].socket != -1 && clients[i].socket != socket){
					if(strcmp(clients[i].username,ms->usernamePrivate) == 0){
						if (write(clients[i].socket, ms, sizeof(message)) == -1) {
							down(&lec);
							return -1;
						}
						flag = 1;
						break;
					} 
				}
			}
			// validar si lo encontro
			if (!flag){
				ms->state = ONLYME;
				strcpy(ms->message,"Invalid parameters in the command");
				if (write(socket, ms, sizeof(message)) == -1) {
					down(&lec);
					return -1;
			 	}
			}
			ms->state = ONLINE;
			down(&lec);
		break;

		//darle la salida al usuario del servidor
		case EXIT:
			up(&lec);
			ms->state = EXIT;
			if (write(socket, ms, sizeof(message)) == -1) {
				down(&lec);
				return -1;
			}
			strcpy(ms->message,"Disconnected\n");
			ms->state = ONLINE;
			//avisar a todos los usuarios que el usuario que solicto
			//salir del chat, se ha desconectado
			for(int i = 0; i < maxclients ; i++){
				// && clients[i].socket != socket
				if(clients[i].socket != -1){
					if (write(clients[i].socket, ms, sizeof(message)) == -1) {
						down(&lec);
						return -1;
					}
				}
			}
			remove_client(id);
			down(&lec);
		break;
		
		default:
			return 0;
		break;
	}
	return 0;
}







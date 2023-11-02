/**
 * @file
 * @brief Pograma cliente.
 * @author Jesus David Cardenas Sandoval <jcardenass@unicauca.edu.co>
*/
#include <arpa/inet.h>
#include "concurrencia.h"
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
#include "chat.h"

/** @brief Estructua de un mensaje*/
typedef struct {
    status state; /*!< estado del usuario>*/
	char message[MAX_MESSAGE]; /*!< Mensaje a enviar>*/
    char username[BUFS]; /*!< Nombre de usuario>*/
    char usernamePrivate[BUFS]; /*!< Nombre de usuario destino>*/
} message;

/** 
 * @brief Subrutina manejadora de señal. 
 */
void signal_handler();

/** 
 * @brief Cambia el estado del mensaje segun el comando ingresado. 
 * @param command comando a realizar.
 */
void changeStateMs(char linea[MAX_MESSAGE]);

/** 
 * @brief Hilo para recibir mensajes del servidor. 
 * @param socket socket del cliente.
 */
void *receiveMessages(void *arg);

/** 
 * @brief Hilo que se encarga de enviar mensajes al servidor. 
 * @param arg socket del cliente.
 */
void *sendMessages(void *arg);

/** @brief bandera para finalizar*/
int finished;

/** @brief mensaje del usuario*/
message ms;

/** @brief socket del usuario*/
int s;

int main(int argc, char * argv[]) {
    
    struct sockaddr_in addr;
    socklen_t clilen;
    char *serverAddr;
    int serverPort;
    int receivedValue;
    struct sigaction act;
    pthread_t tSend, tRecive;
	//Instalar manejadores de las señales SIGINT, SIGTERM
	//Rellenar la estructura con ceros.
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = signal_handler;

	//instalar el manejador para SIGINT
	if (sigaction(SIGINT, &act, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	//instalar el manejador para SIGTERM
	if (sigaction(SIGTERM, &act, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}


    //validar arg de lineas de comando
    if(argc != 4){
        fprintf(stderr, "Debe espeficar el ip y puerto del servidor, y el nombre de usuario"
        , argv[1]);
        exit(EXIT_FAILURE);
    }
    serverAddr = argv[1];
    serverPort = atoi(argv[2]);
    //validar el tam del nombre de usuario
    if(strlen(argv[3]) > 20 || strlen(argv[3]) == 0){
        fprintf(stderr, "Nombre de usuario invalido"
        , argv[1]);
        exit(EXIT_FAILURE);
    }
    strcpy(ms.username,argv[3]);
    fflush(stdin);
    //1. socket (obtener socket)
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s <0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    //2. connect(configurar y conectar una direccion)
    //Rellenar la direccion con null
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serverPort); 
    //addr.sin_addr.s_addr = INADDR_ANY; //0.0.0.0
    if(inet_aton(serverAddr, &addr.sin_addr) == 0){
        perror("inet_aton");
        close(s);
        exit(EXIT_FAILURE);
    } 


    if(connect(s,(struct sockaddr *)&addr, 
        sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        close(s);
        exit(EXIT_FAILURE);
    }

    //Enviar el nombre de usuario ingresado al servidor
    if(write(s,ms.username,strlen(ms.username)) == -1){
        perror("Write failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    //leer respuesta del servidor
    if(read(s,&receivedValue,sizeof(receivedValue)) < 0){
        perror("read failed");
        close(s);
        exit(EXIT_FAILURE);
    }

    //validar si el usuario existe
    if(receivedValue < 0){
        printf("Username exists: %s \nServer Closing\n" , ms.username);
        close(s);
        exit(EXIT_FAILURE);
    }
    //iniciar bandera y estado de que el cliente esta conectado
    finished = 0;
    ms.state = ONLINE;
    printf("Chat: %s \n",ms.username);
    //crear hilos
    pthread_create(&tSend, NULL, sendMessages, (void*)&s);
    pthread_create(&tRecive, NULL, receiveMessages, (void*)&s);

    pthread_join(tSend, NULL);
    pthread_join(tRecive, NULL);
	//4. Terminar la comunicacion
       
	close(s);
	exit(EXIT_SUCCESS);
}

void changeStateMs(char linea[MAX_MESSAGE]) {
    char lineaCopia[MAX_MESSAGE]; // Copia la cadena original
    char command[15];
    char limit[] = " ";
    strcpy(lineaCopia,linea);
    strcpy(command,strtok(lineaCopia, limit));
    if (strstr(command, "/exit") != NULL) {
        ms.state = EXIT;
    } else if (strstr(command, "/private") != NULL) {
        ms.state = PRIVATE;
        char u[BUFS]; 
        strcpy(u,strtok(NULL, limit));// Extrae el nombre de usuario
        if (u != NULL) {
            strcpy(ms.usernamePrivate, u);
            char m[MAX_MESSAGE];
             strcpy(m,strtok(NULL, ""));// Extrae el mensaje completo
            if (m != NULL) {
                strcpy(ms.message, m);
            }
        } 
    } else {
        printf("Comando no reconocido\n");
    }
}



int isCommand(char linea[MAX_MESSAGE]){
    if (strncmp(linea, "/", 1) == 0){
        return 1;
    }
    return 0;
}

void signal_handler() {
    strcpy(ms.message,"/exit");
    changeStateMs(ms.message);
    if (write(s, &ms, sizeof(message)) == -1) {
        perror("Write failed");
    }
    printf("Closing connection..\n");
    //enviar ms exit al servidor para salir , falta
    exit(EXIT_SUCCESS);
}

void *sendMessages(void *arg){
    int s = *(int*) arg;
    int n;

    while (!feof(stdin)) {

        fflush(stdin);

        if(ms.state == EXIT){
            kill(getpid(),SIGTERM);
        }

        if(fgets(ms.message,MAX_MESSAGE, stdin) == NULL){
            perror("fgets failed");
            break;
        }
        fflush(stdin);
        
        //validar si se ingreso un comando
        if(isCommand(ms.message)){
            //ejecutar comando
            changeStateMs(ms.message);
        }else{
            ms.state = ONLINE;
        }

        fflush(stdin);
        n = write(s, &ms, sizeof(message));
        if (n == -1) {
            perror("Write failed");
            break;
        }
        fflush(stdin);
    }
    // cuando acbe de leer en la salida estandar enviar mensaje
    // para sacar al cliente del servidor
    strcpy(ms.message,"/exit");
    changeStateMs(ms.message);
    if (write(s, &ms, sizeof(message)) == -1) {
        perror("Write failed");
    }
    ms.state = ONLINE;
}

void *receiveMessages(void *arg){
    int s = *(int*) arg;
    message msg;
	int n;
	while (!finished) {
		n = read(s, &msg, sizeof(message));

        if (n == -1) {
            perror("Error reading from server");
            break;
        }

        if (n == 0) {
            //End of file
            break;
        }

        if(msg.state == EXIT){
            finished = 1;
            kill(getpid(),SIGTERM);
            continue;
        }

        if(msg.state == ONLINE){
            printf("%s: %s", msg.username , msg.message);
        }

        if(msg.state == PRIVATE){
            printf("Private[%s]: %s", msg.username , msg.message);
        }

        if(msg.state == ONLYME){
            printf("Chat: %s", msg.message);
        }
        ms.state == ONLINE;
        msg.state == ONLINE;
    }
}
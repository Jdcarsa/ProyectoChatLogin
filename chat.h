/**
 * @file
 * @brief Definiciones para el chat
 * @author Jesus David Cardenas Sandoval <jcardenass@unicauca.edu.co>
*/
#ifndef CHAT_H
#define CHAT_H

#define BUFS 20 // tamanio del buff
#define MAX_MESSAGE 200 //tamanio max del mensaje
#define MAX_LINE 170

// Definición de la enumeración
typedef enum{
    ONLINE,
    PRIVATE,
    EXIT,
    OFFLINE,
    ONLYME
}status;




/** 
 * @brief Detecta si es un comando y llama la funcion para ejecutarlo. 
 * @param linea mensaje ingresado.
 * @return 1 si es un comando
 */
int isCommand(char linea[MAX_MESSAGE]);


/** 
* @brief verifica si existe el usuario del cliente.
* @param username nombre de usuario del cliente.
*/
int existsUsername(char username[BUFS]);

#endif
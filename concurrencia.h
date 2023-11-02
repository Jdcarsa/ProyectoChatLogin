/**
 * @file
 * @brief Definiciones de concurrencia
 * @author Jesus David Cardenas Sandoval <jcardenass@unicauca.edu.co>
*/

#ifndef CONCURRENCIA_H
#define CONCURRENCIA_H
#include<pthread.h>
#include<semaphore.h>

/** @brief Alisas para sem_wait*/
#define down sem_wait

/** @brief Alisas para sem_post*/
#define up sem_post

/** @brief maxima cantidad de microsengundos para esperas aleatorios*/
#define SLEEP_INVERTAL 1000

#ifndef FALSE
/** @brief Alisas para false*/
#define FALSE 0
#endif

#ifndef TRUE
/** @brief Alias para true
 */
#define TRUE 1
#endif

/** @brief Definicion del tipo semaphore*/
typedef sem_t semaphore;

#endif


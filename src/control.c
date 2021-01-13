// C program rocket simulation
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/wait.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <semaphore.h>

#define SHMSZ 4
#define NUM 5
#define SHM_ADDR 233

#define HOST "127.0.0.1"
#define PORT 8080
#define QUEUESIZE 10

#define TANK_LEVEL 1024

#define NUM_PROC_READ 4
#define NUM_PROC_WRITE 5

#define MAX 1024

typedef struct colaTDA{
	unsigned long tamano;
	struct nodo_colaTDA *first;
	struct nodo_colaTDA *last;
} cola;

typedef struct nodo_colaTDA{
	void *elemento;
	struct nodo_colaTDA *siguiente;
} nodo_cola;

typedef struct Proceso
{
   char nombre[10];
   pid_t pid;
   int process_pipe;
} process_id_t;

typedef struct request_info{
   char process_name[10];
   char message[25];
} request_t;

int *param[NUM], *distancia, *nivel, *giro1, *giro2, *alarma;
int shmid[NUM];

const char *semName_tanque = "tanque";
const char *semName_distancia = "distancia";
const char *semName_grados_FB = "gradosFB";
const char *semName_grados_LF = "gradosLF";

sem_t *sem_id_tanque = NULL;
sem_t *sem_id_distancia = NULL;
sem_t *sem_id_grados_1 = NULL;
sem_t *sem_id_grados_2 = NULL;

process_id_t *process_to_read;
process_id_t *process_to_write;

cola *requests;

//Inicializadores
int inicializar_memoria_compartida(void);
void inicializarProcessArrays(void);
int inicializar_Procesos(void);

//Funciones Procesos
void tankSystem(int p);
void mainPropulsor(int p);
void auxPropulsor(int *grados, char *name, char *sem_name, int p);
void giroscopirSensor(int *grados, char *name, int p);
void distanceSensor(int p);

//Funciones Auxiliares
void correct_position(int *grades, bool *encendido);
int make_request(char *process_name, char *message, int p);
void liberar_recursos(void);

//Funciones para cola
cola *crear_cola();
int encolar(cola *mi_cola, void * elemento);
void *decolar(cola *mi_cola);
unsigned long tamano_cola(cola *mi_cola);
int destruir_cola(cola *mi_cola);

/*
int open_clientfd(void);
int open_listenfd(void);
int send_request_to_control(char *message, size_t *tam_buffer);
*/

int main(int argc, char *argv[])
{

   int listenfd, connfd;
   unsigned int clientlen;
   struct sockaddr_in clientaddr;

   if (inicializar_memoria_compartida() == -1)
      exit(-1);

   //Aqui llamar a funcion que inicializa los procesos
   inicializarProcessArrays();
   inicializar_Procesos();

   requests = crear_cola();

   /*Aqui llamar a signal de terminacion para control

   listenfd = open_listenfd();
   */

   while (1)
   {
      
      /*
      clientlen = sizeof(clientaddr);
      if ((connfd = accept(listenfd, NULL, 0)) > 0)
      {

         close(connfd);
      }
          //leo datos
      sleep(1); //solo con propósitos ilustrativos
      printf("Valor actual distancia %d combustible %d giro1 giro2 alarma= %d %d %d \n", *distancia, *nivel, *giro1, *giro2, *alarma);
      */
   }
}

int inicializar_memoria_compartida(void)
{
   int i;

   for (i = 0; i < NUM; i++)
   {
      if ((shmid[i] = shmget(SHM_ADDR + i, SHMSZ, 0666)) < 0)
      {
         perror("shmget");
         return (-1);
      }
      if ((param[i] = shmat(shmid[i], NULL, 0)) == (int *)-1)
      {
         perror("shmat");
         return (-1);
      }
   }

   distancia = param[0];
   nivel = param[1];
   giro1 = param[2];
   giro2 = param[3];
   alarma = param[4];

   return (1);
}

void inicializarProcessArrays(void)
{

   process_to_read = (process_id_t *)malloc(NUM_PROC_READ * sizeof(process_id_t));
   strcpy(process_to_read[0].nombre, "Tanque");
   strcpy(process_to_read[1].nombre, "Distancia");
   strcpy(process_to_read[2].nombre, "GiroFB");
   strcpy(process_to_read[3].nombre, "GiroLF");

   process_to_write = (process_id_t *)malloc(NUM_PROC_WRITE * sizeof(process_id_t));
   strcpy(process_to_write[0].nombre, "Main");
   strcpy(process_to_write[1].nombre, "Front");
   strcpy(process_to_write[2].nombre, "Back");
   strcpy(process_to_write[3].nombre, "Left");
   strcpy(process_to_write[4].nombre, "Right");
}

int inicializar_Procesos(void)
{

   pid_t temporal;

   int p[2];

   if (pipe(p) < 0)
      return 1;

   if (fcntl(p[0], F_SETFL, O_NONBLOCK) < 0)
      return 2;

   //Inicializo Sistema de Tanque

   if ((temporal = fork()) == 0)
   {
      close(p[0]);
      tankSystem(p[1]);
   }
   else if (temporal > 0)
   {
      close(p[1]);
      process_to_read[0].pid = temporal;
      process_to_read[0].process_pipe = p[0];
   }
   else
   {
      perror("Couldn't fork");
      return -1;
   }

   //Inicializo Propulsor Principal

   if (pipe(p) < 0)
      return 1;

   if (fcntl(p[0], F_SETFL, O_NONBLOCK) < 0)
      return 2;

   if ((temporal = fork()) == 0)
   {
      close(p[1]);
      mainPropulsor(p[0]);
   }
   else if (temporal > 0)
   {
      close(p[0]);
      process_to_write[0].pid = temporal;
      process_to_write[0].process_pipe = p[1];
   }
   else
   {
      perror("Couldn't fork");
      return -1;
   }

   //Inicializo Sensor Distancia

   if (pipe(p) < 0)
      return 1;

   if (fcntl(p[0], F_SETFL, O_NONBLOCK) < 0)
      return 2;

   if ((temporal = fork()) == 0)
   {
      close(p[0]);
      distanceSensor(p[1]);
   }
   else if (temporal > 0)
   {
      close(p[1]);
      process_to_read[1].pid = temporal;
      process_to_read[1].process_pipe = p[0];
   }
   else
   {
      perror("Couldn't fork");
      return -1;
   }

   int *giro_temp;
   char *sem_name;

   //Inicializo Propulsores Auxiliares

   for (int i = 0; i < 4; i++)
   {
      if (pipe(p) < 0)
         return 1;

      if (fcntl(p[0], F_SETFL, O_NONBLOCK) < 0)
         return 2;

      if (strcmp(process_to_write[i + 1].nombre, "LEFT") == 0 || strcmp(process_to_write[i + 1].nombre, "RIGHT"))
      {
         giro_temp = giro2;
         sem_name = semName_grados_LF;
      }
      else
      {
         giro_temp = giro1;
         sem_name = semName_grados_FB;
      }

      if ((temporal = fork()) == 0)
      {
         char buffer[10] = {0};
         strcpy(buffer, process_to_write[i + 1].nombre);
         close(p[1]);
         auxPropulsor(giro_temp, buffer, sem_name, p[0]);
      }
      else if (temporal > 0)
      {
         close(p[0]);
         process_to_write[i + 1].pid = temporal;
         process_to_write[i + 1].process_pipe = p[1];
      }
      else
      {
         perror("Couldn't fork");
         return -1;
      }
   }

   //Inicializo Sensores Giroscopicos
   for (int i = 2; i < NUM_PROC_READ; i++)
   {

      if (pipe(p) < 0)
         return 1;

      if (fcntl(p[0], F_SETFL, O_NONBLOCK) < 0)
         return 2;

      if (strcmp(process_to_read[i].nombre, "GIRO_LF") == 0)
         giro_temp = giro2;
      else
         giro_temp = giro1;

      if ((temporal = fork()) == 0)
      {
         close(p[0]);
         char buffer[10] = {0};
         strcpy(buffer, process_to_write[i + 1].nombre);
         giroscopirSensor(giro_temp, buffer, p[1]);
      }
      else if (temporal > 0)
      {
         close(p[1]);
         process_to_read[i].pid = temporal;
         process_to_read[i].process_pipe = p[0];
      }
      else
      {
         perror("Couldn't fork");
         return -1;
      }
   }
}

void tankSystem(int p)
{

   liberar_recursos();

   semName_tanque = sem_open(semName_tanque, O_CREAT, 0600, 0);

   if (semName_tanque == SEM_FAILED)
   {
   }

   double level_tank = TANK_LEVEL;

   double controller = TANK_LEVEL * 0.1;

   while (1)
   {
      sleep(1);
      sem_wait(semName_tanque);
      level_tank = TANK_LEVEL * (double)(*nivel / 100);
      sem_post(semName_tanque);
      if (level_tank <= controller)
         printf("Envia mensaje al control");
   }
}

void mainPropulsor(int p)
{
   liberar_recursos();

   semName_tanque = sem_open(semName_tanque, O_CREAT, 0600, 0);
   sem_id_distancia = sem_open(semName_distancia, O_CREAT, 0600, 0);
   if (semName_tanque == SEM_FAILED)
   {
   }

   int distance_decrease = -1;

   while (1)
   {
      sleep(1);
      sem_wait(semName_tanque);

      sem_post(semName_tanque);
      sem_wait(sem_id_distancia);
      *distancia += distance_decrease;
      sem_wait(sem_id_distancia);
   }
}

void auxPropulsor(int *grados, char *name, char *sem_name, int p)
{
   liberar_recursos();

   semName_tanque = sem_open(semName_tanque, O_CREAT, 0600, 0);
   sem_id_grados_1 = sem_open(sem_name, O_CREAT, 0600, 0);

   if (semName_tanque == SEM_FAILED)
   {
   }

   bool encendido = false;

   while (1)
   {
      while (!encendido)
      {
      }
      correct_position(grados, &encendido);
   }
}

void correct_position(int *grados, bool *encendido)
{
   while (*grados > 0)
   {
      sem_wait(semName_tanque);

      sem_post(semName_tanque);
      sem_wait(sem_id_grados_1);
      *grados--;
      sem_post(sem_id_grados_1);

      sleep(1);
   }
   *encendido = false;
}

void giroscopirSensor(int *grados, char *name, int p)
{
   liberar_recursos();
   while (1)
   {
      sleep(1);
      if (*grados != 0)
         printf("Envia mensaje al control");
   }
}

void distanceSensor(int p)
{
   liberar_recursos();
   while (1)
   {
      sleep(1);
      if (*distancia <= 100)
         printf("Envia mensaje al control");
      if (*distancia <= 10)
         print("Envia mensaje al control");
   }
}

void *thread_controller_request(void *vargp){
   size_t readed;
   request_t temp = {0};
   request_t *request = NULL;
   while(1){
      for(int i=0;i<NUM_PROC_READ;i++){
         readed = read(process_to_read[i].process_pipe, (void *) &temp, sizeof(temp));
         if(readed > 0){
            request = (request_t *) malloc(sizeof(request_t));
            strcpy(request->process_name,temp.process_name);
            strcpy(request->message,temp.message);
            encolar(requests,(void *) request);
         }
      }
   }
   return NULL;
}

int make_request(char *process_name, char *message, int p){
   request_t request = {0};
   strcpy(request.process_name,process_name);
   strcpy(request.message,message);
   if(write(p,(void *) &request, sizeof(request))){
      perror("Couldn't write to control\n");
      return -1;
   }
   return 0;

}

void liberar_recursos(void){
   free(process_to_read);
   free(process_to_write);
}

cola *crear_cola(){
	cola *mi_cola = (cola *) malloc(sizeof(cola));
	mi_cola->first = mi_cola->last = NULL;
	mi_cola->tamano = 0;
	if(mi_cola == NULL) return NULL;
	return mi_cola;
}

int encolar(cola *mi_cola, void * elemento){
	if(elemento == NULL || mi_cola == NULL) return -1;
	nodo_cola *nodo = (nodo_cola *) malloc(sizeof(nodo_cola));
	nodo->elemento = elemento;
    if(mi_cola->tamano == 0){
        mi_cola->first = mi_cola->last = nodo;
		nodo->siguiente = NULL;
    }else {
	  	mi_cola->last->siguiente = nodo;
		nodo->siguiente = NULL;
      mi_cola->last = nodo;
    }
    mi_cola->tamano += 1;
	return 0;
}

void *decolar(cola *mi_cola){
	if(mi_cola->tamano == 0 || mi_cola == NULL) return NULL;
	nodo_cola *temp = mi_cola->first;
	if(temp == NULL) return NULL;
	void *data = temp->elemento;
	if(mi_cola->first == mi_cola->last){ 
        mi_cola->first = mi_cola->last = NULL;
    } else {
        mi_cola->first = mi_cola->first->siguiente;
        temp->siguiente = NULL;
    }
	free(temp);
   mi_cola->tamano -= 1;
	return data;
}

unsigned long tamano_cola(cola *mi_cola){
	return mi_cola->tamano;
}

int destruir_cola(cola *mi_cola){
	if(mi_cola->tamano == 0) {
		free(mi_cola);
		return 0;
	}
	return -1;

}

/*
int open_clientfd(void)
{

   struct sockaddr_in servidor;

   memset(&servidor, 0, sizeof(servidor));
   servidor.sin_family = AF_INET;
   servidor.sin_port = htons(PORT);
   servidor.sin_addr.s_addr = inet_addr(HOST);

   int clientfd;

   if ((clientfd = socket(servidor.sin_family, SOCK_STREAM, 0)) < 0)
   {
      perror("Error al crear socket\n");
      return -1;
   }

   if (connect(clientfd, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
   {
      perror("Error conexion\n");
      close(clientfd);
      return -1;
   }

   return clientfd;
}

int open_listenfd(void)
{
   struct sockaddr_in direccion_servidor;
   memset(&direccion_servidor, 0, sizeof(direccion_servidor));

   direccion_servidor.sin_family = AF_INET;
   direccion_servidor.sin_port = htons(PORT);

   direccion_servidor.sin_addr.s_addr = inet_addr(HOST);

   int listenfd;

   if ((listenfd = socket(direccion_servidor.sin_family, SOCK_STREAM, 0)) < 0)
   {
      perror("Error creando descriptor de socket\n");
      return -1;
   }

   if (bind(listenfd, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor)) < 0)
   {
      perror("Error en el bind\n");
      return -1;
   }

   if (listen(listenfd, QUEUESIZE) < 0)
   {
      perror("Error listen\n");
      return -1;
   }

   return listenfd;
}

int send_request_to_control(char *message, size_t *tam_buffer)
{
   int connfd = -1;
   if ((connfd = open_clientfd()) < 0)
      return -1;
   if (write(connfd, message, tam_buffer) < 0)
      return -1;
   close(connfd);
   return 0;
}
*/



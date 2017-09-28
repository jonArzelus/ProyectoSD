/*
Autores: Jon Arzelus y Alessandro Pomes
Sistemas Distribuidos 2017/18
*/

//inclusion de librerias estandar
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//inclusion de librerias de utilidades (socket...)
//#include <sys/types.h>          /* this header file is not required on Linux */
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

//inclusion del archivo de cabeceras
#include "servidor_adf.h"

//creacion del socket y de las conversaciones con el mismo
int main() 
{
	//definicion de valores para crear el socket, la direccion...
	int sock, conversacion;
	struct sockaddr_in dir_servidor;

	//creacion del socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) //socket IPv4 TCP
	{
		perror("Error al crear el socket");
		exit(1);
	}

	//creacion de la estructura para dar direccion al socket
	memset(&dir_servidor, 0, sizeof(dir_servidor));
	dir_servidor.sin_family = AF_INET;
	dir_servidor.sin_addr.s_addr = htonl(INADDR_ANY); //definir direccion ?
	dir_servidor.sin_port = htons(PORT);

	//asignar la direccion al socket	
	if(bind(sock, (struct sockaddr *) &dir_servidor, sizeof(dir_servidor)) < 0)
	{
		perror("Error al asignar una direccion al socket");
		exit(1);
	}

	//establecer el socket como socket de escucha
	if(listen(sock,5) < 0)
	{
		perror("Error al establecer el socket como socket de escucha");
		exit(1);
	}

	//no hacer nada cuando se reciba la señal SIG_CHLD (evita procesos zombie cuando los hijos terminan su tarea)
	signal(SIGCHLD, SIG_IGN);

	while(1)
	{
		//aceptar la peticion de conexion y crear el socket de conversacion
		if((conversacion = accept(sock, NULL, NULL)) < 0)
		{
			perror("Error al aceptar la conexion");
			exit(1);
		}

		//crear un proceso hijo para comunicarse con el cliente
		switch(fork())
		{
			case 0:
				close(sock);
				sesion(conversacion);
				close(conversacion);
				exit(0);
			default:
				close(conversacion);
		}
	}
}

/* programa principal (proceso hijo) para comunicacion con el cliente */
void sesion(int sock_conv)
{
	//creacion de buffers
	char buf[MAX_BUF], file_path[MAX_BUF], file_name[MAX_BUF];

	//poner el estado actual como el estado inicial
	estado = ST_INIT;

	//TODO
}

/* cuando sucede algo inesperado se envia el mensaje de error y se actualiza el estado */
void inesperado(int s)
{
	write(s,"ER1\r\n",5);
	if(estado == ST_AUTH) //error en el login
	{
		estado = ST_INIT;
	}
	else if(estado == ST_DOWN || estado == ST_UP) //error en descarga o subida
	{
		estado = ST_MAIN;
	}
}
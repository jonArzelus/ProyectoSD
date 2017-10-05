#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cli_fich.h"

int main(int argc, char *argv[])
{
	char buf[MAX_BUF], param[MAX_BUF];
	char servidor[MAX_BUF];
	int puerto = PORT;
	
	int sock, n, status, opcion;
	long tam_fich, leidos;
	struct stat file_info;
	FILE *fp;
	struct sockaddr_in dir_serv;
	struct hostent *hp;

	// Procesar parametros
	switch(argc)
	{
		case 1:
			strcpy(servidor, SERVER);
			break;
		case 3:
			puerto = atoi(argv[2]);
		case 2:
			strcpy(servidor, argv[1]);
			break;
		default:
			printf("Uso: %s <servidor> <puerto>\n", argv[0]);
			exit(1);
	}

	// Crear socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error al crear el socket");
		exit(1);
	}

	// Componer la direccion del servidor
	dir_serv.sin_family = AF_INET;
	dir_serv.sin_port = htons(puerto);
	if((hp = gethostbyname(servidor)) == NULL)
	{
		herror("Error al resolver el nombre del servidor");
		exit(1);
	}
	memcpy(&dir_serv.sin_addr, hp->h_addr, hp->h_length);
	
	// Enviar peticion de conexion al servidor
	if(connect(sock, (struct sockaddr *) &dir_serv, sizeof(dir_serv)) < 0)
	{
		perror("Error al intentar conectar con el servidor");
		exit(1);
	}
	
	// Enviar usuario y clave de paso (password)
	int i=0;
	do
	{
		printf("Usuario: ");
		fgets(param,MAX_BUF,stdin);
		param[strlen(param)-1] = 0;
		sprintf(buf,"%s%s\r\n",COMANDOS[COM_USER],param);
		
		write(sock, buf, strlen(buf));
		readline(sock, buf, MAX_BUF);
		status = parse(buf);
		if(status != 0)
		{
			fprintf(stderr,"Error: ");
			fprintf(stderr,"%s",MSG_ERR[status]);
			continue;
		}

		printf("Password: ");
		fgets(param,MAX_BUF,stdin);
		param[strlen(param)-1] = 0;
		sprintf(buf,"%s%s\r\n",COMANDOS[COM_PASS],param);
		
		write(sock, buf, strlen(buf));
		readline(sock, buf, MAX_BUF);
		status = parse(buf);
		if(status != 0)
		{
			fprintf(stderr,"Error: ");
			fprintf(stderr,"%s",MSG_ERR[status]);
			continue;
		}
		break;
	} while(1);

	// Mostrar menu y realizar operacion
	do
	{
		opcion = menu(); // Mostrar el menu
		switch(opcion)
		{
			case OP_LIST: // Pedir lista de ficheros y mostrar en pantalla
				sprintf(buf,"%s\r\n",COMANDOS[COM_LIST]);
				write(sock,buf,strlen(buf)); // Enviar comando
				n = readline(sock, buf, MAX_BUF); // Recibir respuesta
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Error: ");
					fprintf(stderr,"%s",MSG_ERR[status]);
				}
				else
				{
					// Recibir lista de ficheros y mostrarla linea a linea
					int kop = 0;	//  Para controlar el numero de ficheros
					printf("Lista de ficheros recibida del servidor:\n");
					printf("------------------------------------------\n");
					n = readline(sock, buf, MAX_BUF);
					while(n > 2)
					{
						buf[n-2] = 0;
						printf("%s\t\t",strtok(buf,"?"));
						tam_fich = atol(strtok(NULL,"?"));
						if(tam_fich < 0)
							printf("Tamaño desconocido\n");
						else
						{
							if(tam_fich < 1024)
								printf("% 5ld B\n", tam_fich);
							else if(tam_fich < 1024*1024)
								printf("%5.1f KB\n", tam_fich/1024.0);
							else if(tam_fich < 1024*1024*1024)
								printf("%5.1f MB\n", tam_fich/(1024.0*1024));
							else
								printf("%5.1f GB\n", tam_fich/(1024.0*1024*1024));
						}
						kop++;
						n = readline(sock, buf, MAX_BUF);
					}
					if(kop > 0)
					{
						printf("------------------------------------------\n");
						printf("Total ficheros disponibles: %d.\n",kop);
					}
					else
						printf("No hay ficheros disponibles.\n");
					printf("------------------------------------------\n");
				}	
				break;
			case OP_DOWN: // Bajar un fichero del servidor
				printf("Indica el nombre del fichero que quieres bajar: ");
				fgets(param,MAX_BUF,stdin);
				param[strlen(param)-1] = 0;
				sprintf(buf,"%s%s\r\n",COMANDOS[COM_DOWN], param);
				write(sock,buf,strlen(buf)); // Enviar comando
				n = readline(sock, buf, MAX_BUF); // Recibir respuesta
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Error: ");
					fprintf(stderr,"%s",MSG_ERR[status]);
				}
				else
				{
					buf[n-2] = 0; // Borra el final de linea (EOL)
					tam_fich = atol(buf+2);
					sprintf(buf,"%s\r\n",COMANDOS[COM_DOW2]);
					write(sock,buf,strlen(buf)); // Enviar comando
					n = readline(sock, buf, MAX_BUF); // Recibir respuesta
					status = parse(buf);
					if(status != 0)
					{
						fprintf(stderr,"Error: ");
						fprintf(stderr,"%s",MSG_ERR[status]);
					}
					else
					{
						leidos = 0;
						if((fp = fopen(param,"w")) == NULL) // Crear fichero
						{
							perror("No se puede guardar el fichero en el disco local");
							exit(1);
						}
						while(leidos < tam_fich) // Recibir fichero y guardar en disco
						{
							n = read(sock, buf, MAX_BUF);
							if(fwrite(buf, 1, n, fp) < 0)
							{
								perror("Error al guardar el fichero en el disco local");
								exit(1);
							}
							leidos += n;
						}
						fclose(fp);
						printf("El fichero %s ha sido recibido correctamente.\n",param);
					}
				}
				break;
			case OP_UP: // Subir un fichero al servidor
				printf("Indica el nombre del fichero que quieres subir: ");
				fgets(param,MAX_BUF,stdin);
				param[strlen(param)-1] = 0;
				if(stat(param, &file_info) < 0)	// Obtener el tamanyo del fichero
					fprintf(stderr,"El fichero %s no se ha encontrado.\n",param);
				else
				{
					sprintf(buf,"%s%s?%ld\r\n",COMANDOS[COM_UPLO], param, file_info.st_size);
					write(sock,buf,strlen(buf)); // Enviar comando
					n = readline(sock, buf, MAX_BUF); // Recibir respuesta
					status = parse(buf);
					if(status != 0)
					{
						fprintf(stderr,"Error: ");
						fprintf(stderr,"%s",MSG_ERR[status]);
					}
					else
					{
						if((fp = fopen(param,"r")) == NULL) // Abrir fichero
						{
							fprintf(stderr,"Error al abrir el fichero %s.\n",param);
							exit(1);
						}
						sprintf(buf,"%s\r\n",COMANDOS[COM_UPL2]);
						write(sock,buf,strlen(buf));	// Enviar comando
						while((n=fread(buf,1,MAX_BUF,fp))==MAX_BUF) // Enviar fichero a trozos
							write(sock,buf,MAX_BUF);
						if(ferror(fp)!=0)
						{
							fprintf(stderr,"Error al enviar el fichero.\n");
							exit(1);
						}
						write(sock,buf,n); // Enviar el ultimo trozo de fichero
					
						n = readline(sock, buf, MAX_BUF); // Recibir respuesta
						status = parse(buf);
						if(status != 0)
						{
							fprintf(stderr,"Error: ");
							fprintf(stderr,"%s",MSG_ERR[status]);
						}
						else
							printf("El fichero %s ha sido enviado correctamente.\n", param);
					}
				}
				break;
			case OP_DEL: // Borrar un fichero en el servidor
				printf("Indica el nombre del fichero que quieres borrar: ");
				fgets(param,MAX_BUF,stdin);
				param[strlen(param)-1] = 0;
				sprintf(buf,"%s%s\r\n",COMANDOS[COM_DELE], param);
				write(sock,buf,strlen(buf)); // Enviar comando
				n = readline(sock, buf, MAX_BUF); // Recibir respuesta
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Error: ");
					fprintf(stderr,"%s",MSG_ERR[status]);
				}
				else
					printf("El fichero %s ha sido borrado.\n", param);
				break;
			case OP_EXIT: // Terminar sesion
				sprintf(buf,"%s\r\n",COMANDOS[COM_EXIT]);
				write(sock,buf,strlen(buf)); // Enviar comando
				n = readline(sock, buf, MAX_BUF); // Recibir respuesta
				status = parse(buf);
				if(status != 0)
				{
					fprintf(stderr,"Error: ");
					fprintf(stderr,"%s",MSG_ERR[status]);
				}
				break;
		}
	} while(opcion != 5);
	
	close(sock);
}

/* Comprueba la respuesta recibida del servidor y devuelve 0 si es OK o
* el codigo de error recibido en caso que sea ER
*/
int parse(char *status)
{
	if(!strncmp(status,"OK",2))
		return 0;
	else if(!strncmp(status,"ER",2))
		return(atoi(status+2));
	else
	{
		fprintf(stderr,"Respuesta inesperada.\n");
		exit(1); 
	}
}

/*
* Lee datos desde un 'stream' hasta que encuentra un salto de línea estándar de telnet ("\r\n") y
los devuelve en 'buf'.
* Por simplicidad, sólo busca el fin de línea en los últimos bytes de cada lectura.
* Si todo va bien, devuelve el número de caracteres leídos.
* En caso de llegar al final del stream sin haber leido nada devuelve 0.
* En caso de haber leido algo y llegar al final pero sin leer "\r\n", devuelve -1. 
* En caso de haber leido 'tam' caracteres sin haber leido "\r\n", devuelve -2. 
* Si se produce cualquier otro errro, devuelve -3.
*/
int readline(int stream, char *buf, int tam)
{
	/*
		Atencion! Esta implementacion es simple, pero no es nada eficiente.
	*/
	char c;
	int total=0;
	int cr = 0;

	while(total<tam)
	{
		int n = read(stream, &c, 1);
		if(n == 0)
		{
			if(total == 0)
				return 0;
			else
				return -1;
		}
		if(n<0)
			return -3;
		buf[total++]=c;
		if(cr && c=='\n')
			return total;
		else if(c=='\r')
			cr = 1;
		else
			cr = 0;
	}
	return -2;
}

int menu()
{
	char buf[MAX_BUF];
	int opcion;
	
	printf("\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t**                                         **\n");
	printf("\t\t\t\t**       1. Mostrar Lista de ficheros      **\n");
	printf("\t\t\t\t**       2. Bajar fichero                  **\n");
	printf("\t\t\t\t**       3. Subir fichero                  **\n");
	printf("\t\t\t\t**       4. Borrar fichero                 **\n");
	printf("\t\t\t\t**       5. Terminar                       **\n");
	printf("\t\t\t\t**                                         **\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t*********************************************\n");
	
	printf("\t\t\t\tElige una opcion: ");
	while(1)
	{
		fgets(buf,MAX_BUF,stdin);
		opcion = atoi(buf);
		if(opcion > 0 && opcion < 6)
			break;
		printf("\t\t\t\tOpcion erronea. Intentalo de nuevo: ");
	}
	printf("\n");
	return opcion;
}

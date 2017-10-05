#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>

#include "serv_fich.h"

int main()
{
	int sock, dialogo;
	struct sockaddr_in dir_serv;
	socklen_t tam_dir;

	// Crear el socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error al crear el socket");
		exit(1);
	}
	
	memset(&dir_serv, 0, sizeof(dir_serv));
	dir_serv.sin_family = AF_INET;
	dir_serv.sin_addr.s_addr = htonl(INADDR_ANY);
	dir_serv.sin_port = htons(PORT);
	
	// Asignar direccion al socket
	if(bind(sock, (struct sockaddr *) &dir_serv, sizeof(dir_serv)) < 0)
	{
		perror("Error al asignar una direccion al socket");
		exit(1);
	}

	// Preparar el socket como socket de escucha
	if(listen(sock,5) < 0)
	{
		perror("Error al definir el socket como socket de escucha");
		exit(1);
	}

	// Ignorar la senyal enviada cuando un proceso hijo acaba, para que no quede como zombi
	signal(SIGCHLD, SIG_IGN);

	while(1)
	{
		// Aceptar peticion de conexion de cliente y crear socket de dialogo
		if((dialogo = accept(sock, NULL, NULL)) < 0)
		{
			perror("Error al aceptar la conexion");
			exit(1);
		}

		// Crear proceso hijo para que se encargue de la comunicacion con el cliente
		switch(fork())
		{
			case 0:
				close(sock);
				sesion(dialogo);
				close(dialogo);
				exit(0);
			default:
				close(dialogo);
		}
	}
}

/*
* Esta funcion se encarga de llevar a cabo la comunicacion con el cliente conectado
siguiendo el protocolo de la aplicacion.
* Para ello se le pasa el socket de dialogo como parametro.
* Se ejecutara concurrentemente en un proceso hijo.
*/
void sesion(int s)
{
	char buf[MAX_BUF], file_path[MAX_BUF], file_name[MAX_BUF];
	int n, usuario, comando, error;
	FILE *fp;
	struct stat file_info;
	unsigned long file_size, leidos;
	char * sep;
	
	// Establecer estado como inicial
	estado = ST_INIT;
	
	while(1)
	{
		// Leer lo enviado por el cliente
		if((n=readline(s,buf,MAX_BUF)) <= 0)
			return;

		// Comprobar si el comando es conocido
		if((comando=busca_substring(buf,COMANDOS)) < 0)
		{
			inesperado(s);
			continue;
		}

		// Realizar la operacion correspondiente segun el comando recibido
		switch(comando)
		{
			case COM_USER:
				if(estado != ST_INIT) // Comprobar si el estado es el esperado
				{
					inesperado(s);
					continue;
				}
				buf[n-2] = 0; // Borra el final de linea (EOL)
				// Comprobar si es un usuario valido.
				if((usuario = busca_string(buf+4, usuarios)) < 0)
					write(s,"ER2\r\n",5);
				else
				{
					write(s,"OK\r\n",4);
					estado = ST_AUTH;
				}
				break;
			case COM_PASS:
				if(estado != ST_AUTH) // Comprobar si el estado es el esperado
				{
					inesperado(s);
					continue;
				}
				buf[n-2] = 0;	// Borrar EOL
				// Comprobar si el password es correcto
				if(usuario == 0 || !strcmp(passwords[usuario], buf+4))
				{
					write(s,"OK\r\n",4);
					estado = ST_MAIN;
				}
				else
				{
					write(s,"ER3\r\n",5);
					estado = ST_INIT;
				}
				break;
			case COM_LIST:
				if(n>6 || estado != ST_MAIN) // Comprobar si el estado es el esperado y que no se han recibido parametros
				{
					inesperado(s);
					continue;
				}
				if(enviar_listado(s) < 0)
					write(s,"ER4\r\n",5);
				break;
			case COM_DOWN:
				if(estado != ST_MAIN) // Comprobar si el estado es el esperado
				{
					inesperado(s);
					continue;
				}
				buf[n-2] = 0; // Borrar EOL
				// Concatenar el path y el nombre del fichero
				sprintf(file_path,"%s/%s",FILES_PATH,buf+4);
				if(stat(file_path, &file_info) < 0)
					write(s,"ER5\r\n",5);
				else
				{
					sprintf(buf,"OK%ld\r\n", file_info.st_size);
					write(s, buf, strlen(buf));
					estado = ST_DOWN;
				}
				break;
			case COM_DOW2:
				if(n > 6 || estado != ST_DOWN) // Comprobar si el estado es el esperado y que no se han recibido parametros
				{
					inesperado(s);
					continue;
				}
				estado = ST_MAIN;
				// Abrir fichero
				if((fp=fopen(file_path,"r")) == NULL)
					write(s,"ER6\r\n",5);
				else
				{
					// Leer el primer trozo de fichero
					if((n=fread(buf,1,MAX_BUF,fp))<MAX_BUF && ferror(fp) != 0)
						write(s,"ER6\r\n",5);
					else
					{
						write(s,"OK\r\n",4);
						// Enviar trozo de fichero
						do
						{
							write(s,buf,n);
						} while((n=fread(buf,1,MAX_BUF,fp))==MAX_BUF);
						if(ferror(fp) != 0)
						{
							close(s);
							return;
						}
						else if(n>0)
							write(s,buf,n);	// Enviar ultimo trozo
					}
					fclose(fp);
				}
				break;
			case COM_UPLO:
				if(estado != ST_MAIN) // Comprobar si el estado es el esperado
				{
					inesperado(s);
					continue;
				}
				// Rechazar usuario anonimo
				if(usuario==0)
				{
					write(s,"ER7\r\n",5);
					continue;
				}
				
				buf[n-2] = 0; // Borrar EOL
				// Extraer del mensaje el nombre del fichero y el tamanyo
				if((sep = strchr(buf,'?')) == NULL)
				{
					inesperado(s);
					continue;
				}
				// Posicionar 'sep' como comienza el string con el tamanyo
				*sep = 0;
				sep++;
				strcpy(file_name,buf+4); // Nombre del fichero
				file_size = atoi(sep);   // Tamanyo del fichero
				// Concatenar el path y el nombre del fichero
				sprintf(file_path,"%s/%s",FILES_PATH,file_name);
				if(file_size > MAX_UPLOAD_SIZE) // Comprobar que no supera el tamanyo maximo de fichero
				{
					write(s,"ER8\r\n",5);
					continue;
				}
				if(espacio_libre() < file_size + SPACE_MARGIN)	// Mantener siempre un mínimo de espacio en el disco
				{
					write(s,"ER9\r\n",5);
					continue;
				}
				write(s,"OK\r\n",4);
				estado = ST_UP;
				break;
			case COM_UPL2:
				if(n > 6 || estado != ST_UP) // Comprobar si el estado es el esperado y que no se han recibido parametros
				{
					inesperado(s);
					continue;
				}
				estado = ST_MAIN;
				leidos = 0L;
				// Crear el fichero nuevo
				fp=fopen(file_path,"w");
				error = (fp == NULL);
				while(leidos < file_size)
				{
					// Recibir un trozo de fichero
					if((n=read(s,buf,MAX_BUF)) <= 0)
					{
						close(s);
						return;
					}
					// Guardar el trozo en disco si no ha habido error.
					// Si ha habido error, continuar recibiendo puesto que el cliente seguira enviando todo el fichero; el protocolo no da opcion de avisar al cliente para que pare.
					if(!error)
					{
						if(fwrite(buf, 1, n, fp) < n)
						{
							fclose(fp);
							unlink(file_path);
							error = 1;
						}
					}
					leidos += n;
				}
				if(!error)
				{
					fclose(fp);
					write(s,"OK\r\n",4);
				}
				else
					write(s,"ER10\r\n",6);
				break;
			case COM_DELE:
				if(estado != ST_MAIN) // Comprobar si el estado es el esperado
				{
					inesperado(s);
					continue;
				}
				// Rechazar usuario anonimo
				if(usuario==0)
				{
					write(s,"ER7\r\n",5);
					continue;
				}
				buf[n-2] = 0; // Borrar EOL
				// Concatenar el path y el nombre del fichero
				sprintf(file_path,"%s/%s",FILES_PATH,buf+4);
				// Borrar el fichero
				if(unlink(file_path) < 0)
					write(s,"ER11\r\n",6);
				else
					write(s,"OK\r\n",4);
				break;
			case COM_EXIT:
				if(n > 6) // Comprobar que no se han recibido parametros
				{
					inesperado(s);
					continue;
				}
				write(s,"OK\r\n",4);
				return;
		}
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

/*
* Busca el string 'string' en el array de strings 'strings'. El último elemento del array 'strings' ha de ser NULL.
* Devuelve el índice de la primera aparición de 'string' en 'strings'. Si 'string' no esta en el array, devuelve un valor negativo.
*/

int busca_string(char *string, char **strings)
{
	int i=0;
	while(strings[i] != NULL)
	{
		if(!strcmp(string,strings[i]))
			return i;
		i++;
	}
	return -1;
}

/*
* Busca si el string 'string' comienza con algún string contenido en el array de strings 'strings'. El último elemento del array 'strings' ha de ser NULL.
* Devuelve el índice del primer elemento de 'strings' que coincide con el comienzo de 'string'. Si no hay coincidencia, devuelve un valor negativo.
*/
int busca_substring(char *string, char **strings)
{
	int i=0;
	while(strings[i] != NULL)
	{
		if(!strncmp(string,strings[i],strlen(strings[i])))
			return i;
		i++;
	}
	return -1;
}

/*
* Tratamiento de un comportamiento inesperado: Enviar el correspondiente error al cliente y actualizar el estado.
*/
void inesperado(int s)
{
	write(s,"ER1\r\n",5);
	if(estado == ST_AUTH)
	{
		estado = ST_INIT;
	}
	else if(estado == ST_DOWN || estado == ST_UP)
	{
		estado = ST_MAIN;
	}
}

/*
* Envia el listado de ficheros disponibles a través del stream 's'. Si no es posible escanear el directorio de ficheros devuelve un valor negativo, en caso contrario devuelve el número de nombres de fichero enviados.
*/
int enviar_listado(int s)
{
	struct dirent ** nombre_ficheros;
	int i,num_ficheros;
	long tam_fichero;
	char buf[MAX_BUF], path_fichero[MAX_BUF];
	struct stat file_info;

	num_ficheros = scandir(FILES_PATH, &nombre_ficheros, no_oculto, alphasort);
	if(num_ficheros < 0)
		return -1;
	if(write(s,"OK\r\n",4) < 4)
		return -1;
		
	for(i=0;i<num_ficheros;i++)
	{
		sprintf(path_fichero,"%s/%s",FILES_PATH, nombre_ficheros[i]->d_name);
		if(stat(path_fichero,&file_info) < 0)
			tam_fichero=-1L;
		else
			tam_fichero = file_info.st_size;
		sprintf(buf,"%s?%ld\r\n",nombre_ficheros[i]->d_name, tam_fichero);
		if(write(s, buf, strlen(buf)) < strlen(buf))
			return i;
	}
	write(s,"\r\n",2);
	return num_ficheros;
}

/*
* Devuelve el espacio disponible en bytes en el disco donde se ubican los ficheros, o un valor negativo en caso de error.
*/
unsigned long espacio_libre()
{
	struct statvfs info;

	if(statvfs(FILES_PATH,&info)<0)
		return -1;
	return info.f_bsize * info.f_bavail;
}

/*
* Filtro para no tener en cuenta los ficheros ocultos de un directorio. Si el nombre del fichero comienza con '.' devuelve 0 y 1 en caso contrario. Hay que tener en cuenta que esta es la forma indicar que un fichero es oculto en UNIX y sistemas similares, con lo que no funcionara en sistemas Windows.
*/
int no_oculto(const struct dirent *entry)
{
   if (entry->d_name[0]== '.')
     return (0);
   else
     return (1);
}

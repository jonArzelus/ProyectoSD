#define MAX_BUF 1024
#define SERVER "localhost"
#define PORT 6012

#define COM_USER	0
#define COM_PASS	1
#define COM_LIST	2
#define COM_DOWN	3
#define COM_DOW2	4
#define COM_UPLO	5
#define COM_UPL2	6
#define COM_DELE	7
#define COM_EXIT	8

#define OP_LIST	1
#define OP_DOWN	2
#define OP_UP		3
#define OP_DEL	4
#define OP_EXIT	5


char * COMANDOS[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","EXIT",NULL};
char * MSG_ERR[] =
{
	"Todo correcto.\n",
	"Comando desconocido o inesperado.\n",
	"Usuario desconocido.\n",
	"Clave de paso o password incorrecto.\n",
	"Error al crear la lista de ficheros.\n",
	"El fichero no existe.\n",
	"Error al bajar el fichero.\n",
	"Un usuario anonimo no tiene permisos para esta operacion.\n",
	"El fichero es demasiado grande.\n",
	"Error al preparar el fichero para subirlo.\n",
	"Error al subir el fichero.\n",
	"Error al borrar el fichero.\n"
};

int parse(char *status);
int readline(int stream, char *buf, int tam);
int menu();

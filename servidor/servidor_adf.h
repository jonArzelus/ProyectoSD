/*
Autores: Jon Arzelus y Alessandro Pomes
Sistemas Distribuidos 2017/18
*/

//definicion de valores estandar para la ejecucion
#define MAX_BUF 1024
#define PORT 6800
#define FILES_PATH	"files"

//definicion de numeros de comando de usuario
#define COM_USER	0
#define COM_PASS	1
#define COM_LIST	2
#define COM_DOWN	3
#define COM_DOW2	4
#define COM_UPLO	5
#define COM_UPL2	6
#define COM_DELE	7
#define COM_EXIT	8

//definicion de valores de la aplicacion
#define MAX_UPLOAD_SIZE	10*1024*1024 //10MB
#define SPACE_MARGIN 50*1024*1024 //50MB

//definicion del array de comandos, usuarios...
char * COMANDOS[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","EXIT",NULL};
char * lista_usuarios[] = {"anonimo","user1","user2",NULL};
char * lista_contraseñas[] = {"","user1","user2"};
int estado;
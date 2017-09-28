/*
Autores: Jon Arzelus y Alessandro Pomes
Sistemas Distribuidos 2017/18
*/

//definicion de valores estandar para la ejecucion
#define MAX_BUF 1024
//#define SOCK_ADDR 127.0.0.1
#define PORT 50005
#define FILES_PATH	"archivos"

//definicion de estados posibles del programa
#define ST_INIT	0
#define ST_AUTH	1
#define ST_MAIN	2
#define ST_DOWN	3
#define ST_UP	4

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
#define SPACE_MARGIN 	50*1024*1024 //50MB

//definicion del array de comandos, usuarios...
char * COMANDOS[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","EXIT",NULL};
char * lista_usuarios[] = {"anonimo","user1","user2",NULL};
char * lista_contrasenas[] = {"","user1","user2"}; //falta NULL ?
int estado;

//crear las funciones del programa
void sesion(int sock_conv);
void inesperado(int s);
#define MAX_BUF 1024
#define PORT 6012
#define FILES_PATH	"files"

#define ST_INIT	0
#define ST_AUTH	1
#define ST_MAIN	2
#define ST_DOWN	3
#define ST_UP		4

#define COM_USER	0
#define COM_PASS	1
#define COM_LIST	2
#define COM_DOWN	3
#define COM_DOW2	4
#define COM_UPLO	5
#define COM_UPL2	6
#define COM_DELE	7
#define COM_EXIT	8

#define MAX_UPLOAD_SIZE	10*1024*1024	// 10 MB
#define SPACE_MARGIN		50*1024*1024	// 50 MB

char * COMANDOS[] = {"USER","PASS","LIST","DOWN","DOW2","UPLO","UPL2","DELE","EXIT",NULL};
char * usuarios[] = {"anonimous","sar","sza",NULL};
char * passwords[] = {"","sar","sza"};
int estado;

void sesion(int s);
int readline(int stream, char *buf, int tam);
int busca_string(char *string, char **strings);
int busca_substring(char *string, char **strings);
void inesperado(int s);
int enviar_listado(int s);
unsigned long espacio_libre();
int no_oculto(const struct dirent *entry);

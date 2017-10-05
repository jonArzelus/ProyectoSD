// Prueba de captura de notificaciones de Linux
// Adaptado de http://www.thegeekstuff.com/2010/04/inotify-c-program-example/
// por Alberto Lafuente, Fac. Informática UPV/EHU

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
//#define EVENT_SIZE  (3*sizeof(uint32_t)+sizeof(int))
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

int main(int argc, char *argv[])
{
  int length, i = 0;
  int fd;
  int wd;
  int wd_cd;
  char buffer[EVENT_BUF_LEN];
  char testigo[1024];

  if (argc!=2) {
      fprintf(stderr, "Uso: %s Directorio_de_notificaciones\n", argv[0]);
      exit(1);
  }

  fprintf(stderr, "---Prueba de inotify sobre %s\n", argv[1]);
  fprintf(stderr, "---Notifica crear/borrar ficheros/directorios sobre %s\n", argv[1]);
  fprintf(stderr, "---%s debe exixtir!\n", argv[1]);
  fprintf(stderr, "---Para salir, borrar %s/inotify.example.executing\n", argv[1]); 

  sprintf(testigo, "%s/inotify.example.executing", argv[1]);
  fprintf(stderr, "      Testigo: %s\n", testigo); 

  /*creating the INOTIFY instance*/
  fd = inotify_init();

  /*checking for error*/
  if ( fd < 0 ) {
    perror( "inotify_init" );
  }

  /*adding the /tmp directory into watch list. Here, the suggestion is to validate the existence of the directory before adding into monitoring list.*/
//  wd = inotify_add_watch( fd, "/tmp", IN_CREATE | IN_DELETE );

  /* Notificaremos los cambios en el directorio ./My_inotify */
//  mkdir(argv[1]);
  wd_cd = inotify_add_watch( fd, argv[1], IN_CREATE | IN_DELETE );

  /* Testigo para finalizar cuando lo borremos: */
  mkdir(testigo);



  /*read to determine the event change happens on the directory. Actually this read blocks until the change event occurs*/ 
  struct inotify_event event_st, *event;
  int k=0;
  int exiting= 0;


  while (!exiting) {
    fprintf(stderr, "---%s: waiting for event %d...\n", argv[0], ++k); 
    length = read( fd, buffer, EVENT_BUF_LEN ); 
    fprintf(stderr, "---%s: event %d read.\n", argv[0], k); 
    /*checking for error*/
    if ( length < 0 ) {
      perror( "read" );
      break;
    }
//    struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
    while ( (i < length) && !exiting ) {
//    event= &event_st;
      event = ( struct inotify_event * ) &buffer[ i ];
//    fprintf(stderr, "---example: event name length: %i\n", event->len);
//    memcpy(event, buffer, length);
      if ( event->len ) {
//      memcpy(event+EVENT_SIZE, buffer+EVENT_SIZE, length);
        if ( event->mask & IN_CREATE ) {
          if ( event->mask & IN_ISDIR ) {	// event: directory created
            printf( "---%s: New directory %s created.\n", argv[0], event->name );
          }
          else {	// event: fie created
            printf( "---%s: New file %s created.\n", argv[0], event->name );
          }
        }
        else if ( event->mask & IN_DELETE ) {
          if ( event->mask & IN_ISDIR ) {	// event: directory removed
            if (!strcmp(event->name, "inotify.example.executing")) {
              rmdir("example.inotify.executing");
              exiting= 1;
//              break;
            }
            printf( "---%s: Directory %s deleted.\n", argv[0], event->name );
          }
          else {	// event: fie removed
            printf( "---%s: File %s deleted.\n", argv[0], event->name );
          }
        }
      }
      else {	// event ignored
        fprintf(stderr, "---%s: event ignored for %s\n", argv[0], event->name); 
      }
      i += EVENT_SIZE + event->len;
//    fprintf(stderr, "---example.event count: %i\n", i); 
    }
    i= 0;
  }
  fprintf(stderr, "---Exiting %s\n", argv[0]); 
 /*removing the directory from the watch list.*/
  inotify_rm_watch( fd, wd );
  inotify_rm_watch( fd, wd_cd );
//  rmdir(argv[1]);

 /*closing the INOTIFY instance*/
  close( fd );

}

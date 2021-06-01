/*------------------------------------------------------------------------------
Proyecto Shell de UNIX. Sistemas Operativos
Grados I. Informática, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Algunas secciones están inspiradas en ejercicios publicados en el libro
"Fundamentos de Sistemas Operativos", Silberschatz et al.

Para compilar este programa: gcc MiShell.c ApoyoTareas.c -o MiShell
Para ejecutar este programa: ./MiShell
Para salir del programa en ejecución, pulsar Control+D
------------------------------------------------------------------------------*/

/*----------------------------------------
Datos del alumno:
Apellidos, Nombre: Machuca Durán, Rafael
Grado: Ing. Informática
Curso y grupo: 2º A
-----------------------------------------*/

#include "ApoyoTareas.h" 

#define MAX_LINE 256 
#include <string.h>  

job *tasks;

void manejador_signal(int signal) {
    block_SIGCHLD(); 
    job *item;
    int estado, info, pid_wait = 0;
    enum status status_res;

    for(int i = 1; i <= list_size(tasks); i++) {
        item = get_item_bypos(tasks, i);
        pid_wait =  waitpid(item -> pgid, &estado, WUNTRACED | WNOHANG); 
        if (pid_wait == item -> pgid) {
            status_res = analyze_status(estado, &info);
            if (status_res == SUSPENDIDO){
                printf("Ejecutado en segundo plano. Comando: %s. PID %d. Ha suspendido su ejecucion\n", item->command, item->pgid);
                item -> ground = DETENIDO;
            } else if(status_res == FINALIZADO){
                printf("Ejecutado en segundo plano Comando: %s. PID %d. Ha concluido su ejecucion\n", item->command, item->pgid);
                delete_job(tasks, item);
            }
        }
    }
    unblock_SIGCHLD(); 
}

int main(void) {
    char inputBuffer[MAX_LINE]; 
    int background;         
    char *args[MAX_LINE / 2]; 
    int pid_fork, pid_wait; 
    int status;             
    enum status status_res; 
    int info;        
    job *item;
    int primerplano = 0; 

    ignore_terminal_signals();
    signal(SIGCHLD, manejador_signal);
    tasks = new_list("tareas");

    while (1) {  
        printf("COMANDO-> ");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); 
        if (args[0] == NULL) continue; 
      
        if (!strcmp(args[0], "cd")) {
            chdir(args[1]); 
            continue; 
        }

        if (!strcmp(args[0], "logout")) exit(0);

        if(!strcmp(args[0], "jobs") ){
            block_SIGCHLD();
            if (list_size(tasks) == 0) printf("No hay ningún proceso en la lista.\n");
            else print_job_list(tasks);
            unblock_SIGCHLD();
            continue;
        }

        if(!strcmp(args[0], "fg")) {
            block_SIGCHLD();
            int pos = 1;
            primerplano = 1; 
            if(args[1] != NULL)pos = atoi(args[1]); 
            item = get_item_bypos(tasks, pos);
            if(item != NULL){
                set_terminal(item->pgid); 
                if(item->ground == DETENIDO)killpg(item->pgid, SIGCONT);
                pid_fork = item->pgid;
                delete_job(tasks, item);
            }
            unblock_SIGCHLD();
        }

        if(!strcmp(args[0], "bg")) {
            block_SIGCHLD();
            int pos = 1;
            if(args[1] != NULL) pos = atoi(args[1]); 
            item = get_item_bypos(tasks, pos);
            if((item != NULL) && (item->ground == DETENIDO)){ 
                item->ground = SEGUNDOPLANO;
                killpg(item->pgid, SIGCONT);
            } 
            unblock_SIGCHLD();
            continue;
        }

        if (!primerplano) pid_fork = fork(); 

        if (pid_fork > 0) {
            if(background == 0)  {
                waitpid(pid_fork, &status, WUNTRACED);
                set_terminal(getpid());
                status_res = analyze_status(status, &info); 
                if(status_res == SUSPENDIDO){
                    block_SIGCHLD();
                    item = new_job(pid_fork, args[0], DETENIDO);
                    add_job(tasks, item);
                    printf("Ejecutado en primer plano. Comando: %s. PID: %d. Estado: %s. Info: %d.\n", args[0], pid_fork, status_strings[status_res], info);
                    block_SIGCHLD();
                }
                else if(status_res == FINALIZADO)
                    if(info != 255) 
                        printf("Ejecutado en primer plano. Comando: %s. PID: %d. Estado: %s. Info: %d.\n", args[0], pid_fork, status_strings[status_res], info);
                primerplano = 0;
            } else {  
                block_SIGCHLD();
                item = new_job(pid_fork, args[0], SEGUNDOPLANO);
                add_job(tasks, item);
                printf("Ejecutado en segundo plano. Comando: %s. PID: %d.\n", args[0], pid_fork);
                unblock_SIGCHLD();
            }
        } else {
            new_process_group(getpid());
            if (background == 0) set_terminal(getpid());
            restore_terminal_signals();
            execvp(args[0], args); 
            printf("Error. Comando %s no encontrado\n", args[0]);
            exit(-1); 
        }
    } 
}



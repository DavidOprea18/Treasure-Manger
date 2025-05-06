#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/types.h> 

// ------------------------ STRUCT ------------------------ //

typedef struct {
    int treasure_id;
    char user_name[50];
    float latitude_coordinates;
    float longitude_coordinates;
    char clue_text[100];
    int value;
} TREASURE;

// ------------------------ global variables ------------------------ //

pid_t monitor_pid = -1;
volatile sig_atomic_t ok = 0;

// ------------------------ handler - ok ------------------------ //

void handle_ok()
{
    ok = 1;
}

// ------------------------ List Hunts ------------------------ //

void list_hunts()
{
    DIR *dir;
    struct dirent *dp;

    dir = opendir(".");
    if (dir == NULL) 
    {
        write(1, "Error: Could not open project directory\n", strlen("Error: Could not open project directory\n"));
        return;
    }

    while ((dp = readdir(dir)) != NULL) 
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            continue;
        }

        struct stat st;
        char path[100];
        strcpy(path, ".");
        strcat(path, "/");
        strcat(path, dp->d_name);

        if (stat(path, &st) == -1)
        {
            perror(NULL);
            exit(-1);
        }

        if (S_ISDIR(st.st_mode))
        {
            //create path for treasures
            char treasure_path[256];
            snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.bin", path);

            //number of treasures in hunt
            int fd = open(treasure_path, O_RDONLY);
            if (fd == -1)
            {
                perror("Failed to open treasures file");
                continue;
            }
            int treasure_count = 0;
            TREASURE t;
            while (read(fd, &t, sizeof(TREASURE)) == sizeof(TREASURE))
            {
                treasure_count++;
            }
            close(fd);

            //error if no treasures
            if (treasure_count == 0)
            {
                write(1, "Error: No treasures found in hunt\n", strlen("Error: No treasures found in hunt\n"));
                continue;
            }

            //print hunt name and number of treasures
            char treasure_count_str[16];
            snprintf(treasure_count_str, sizeof(treasure_count_str), "%d", treasure_count);

            write(1, "Hunt: ", strlen("Hunt: "));
            write(1, dp->d_name, strlen(dp->d_name));
            write(1, " - Tresures: ", strlen(" - Treasures: "));
            write(1, treasure_count_str, strlen(treasure_count_str));
            write(1, "\n", 1);
        }
    }
    write(1, "\n", 1);
    closedir(dir);

    kill(getppid(), SIGTERM);
}

// ------------------------ List Treasures ------------------------ //

void list_treasures()
{
    pid_t compile_pid, exec_pid;

    // Compile the treasure_manager.c file
    if ((compile_pid = fork()) < 0) {
        perror("Compilation fork failed");
        exit(-1);
    }
    else if (compile_pid == 0) {
        execlp("gcc", "gcc", "-o", "app", "treasure_manager.c", NULL);
        perror("Compilation failed");
        exit(-1);
    }

    waitpid(compile_pid, NULL, 0);
    
    // Get hunt ID
    char hunt_id[16];
    write(1, "Enter hunt ID to list treasures: ", strlen("Enter hunt ID to list treasures: "));
    read(0, hunt_id, sizeof(hunt_id));
    hunt_id[strcspn(hunt_id, "\n")] = '\0';
    
    // Execute list command from treasure_manager.c
    if ((exec_pid = fork()) < 0) {
        perror("Execution fork failed");
        exit(-1);
    }
    else if (exec_pid == 0) {
        execlp("./app", "app", "list", hunt_id, NULL);
        perror("Execution failed");
        exit(-1);
    }

    waitpid(exec_pid, NULL, 0);
    kill(getppid(), SIGTERM);
}

// ------------------------ View Treasure ------------------------ //

void view_treasure()
{
    pid_t compile_pid, exec_pid;

    // Compile the treasure_manager.c file
    if ((compile_pid = fork()) < 0) {
        perror("Compilation fork failed");
        exit(-1);
    }
    else if (compile_pid == 0) {
        execlp("gcc", "gcc", "-o", "app", "treasure_manager.c", NULL);
        perror("Compilation failed");
        exit(-1);
    }

    waitpid(compile_pid, NULL, 0);
    
    // Get hunt ID
    char hunt_id[16];
    write(1, "Enter hunt ID: ", strlen("Enter hunt ID: "));
    read(0, hunt_id, sizeof(hunt_id));
    hunt_id[strcspn(hunt_id, "\n")] = '\0';

    // Get treasure ID
    char treasure_id[16];
    write(1, "Enter treasure ID: ", strlen("Enter treasure ID: "));
    read(0, treasure_id, sizeof(treasure_id));
    treasure_id[strcspn(treasure_id, "\n")] = '\0';
    
    // Execute view command from treasure_manager.c
    if ((exec_pid = fork()) < 0) {
        perror("Execution fork failed");
        exit(-1);
    }
    else if (exec_pid == 0) {
        execlp("./app", "app", "view", hunt_id, treasure_id, NULL);
        perror("Execution failed");
        exit(-1);
    }

    waitpid(exec_pid, NULL, 0);
    kill(getppid(), SIGTERM);
}


// ------------------------ Start Monitor ------------------------ //

void start_monitor()
{
    if (monitor_pid != -1)
    {
        write(1, "Monitor is already running\n", strlen("Monitor is already running\n"));
        return;
    }
    
    monitor_pid = fork();
    
    if (monitor_pid < 0)
    {
        perror("Failed to fork monitor\n");
        exit(-1);
    }
    
    if (monitor_pid == 0)
    {
        write(1, "Monitor started\n\n", strlen("Monitor started\n\n"));
        
        struct sigaction sig;
        
        sig.sa_handler = handler_terminate;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);
        sigaction(SIGTERM, &sig, NULL);
        
        sig.sa_handler = list_hunts;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);
        sigaction(SIGUSR1, &sig, NULL);
        
        sig.sa_handler = list_treasures;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);
        sigaction(SIGUSR2, &sig, NULL);
        
        sig.sa_handler = view_treasure;
        sig.sa_flags = 0;
        sigemptyset(&sig.sa_mask);
        sigaction(SIGINT, &sig, NULL);
        
        while (1)
        {
            pause();
        }
    }
}

// ------------------------ handler - terminate ------------------------ //

void handler_terminate()
{
    write(1, "Monitor terminated\n\n", strlen("Monitor terminated\n\n"));
    exit(0);
}

// ------------------------ Stop Monitor ------------------------ //

void stop_monitor()
{
    if (monitor_pid == -1)
    {
        write(1, "Monitor is already off\n", strlen("Monitor is already off\n"));
        return;
    }

    write(1, "Monitor stops\n", strlen("Monitor stops\n"));

    if (kill(monitor_pid, SIGTERM) == -1)
    {
        write(1, "Error stopping\n", strlen("Error stopping\n"));
    }

    waitpid(monitor_pid, NULL, 0);
    monitor_pid = -1;
    usleep(1000000);
}

// ------------------------ MAIN FUNCTION ------------------------ //

int main()
{
    struct sigaction sig;
    sig.sa_handler = handle_ok;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGTERM, &sig, NULL);

    char buff[256];

    while (read(0, buff, sizeof(buff)) > 0)
    {
        buff[strcspn(buff, "\n")] = '\0';

        // start_monitor
        if (strcmp(buff, "start_monitor") == 0)
        {
            start_monitor();
        }

        // list_hunts   
        else if (strcmp(buff, "list_hunts") == 0)
        {
            ok = 0;
            if (monitor_pid != -1) {
                if (kill(monitor_pid, SIGUSR1) == -1) {
                    write(1, "Error sending signal\n", strlen("Error sending signal\n"));
                }
            } else {
                write(1, "Monitor is not running\n", strlen("Monitor is not running\n"));
            }
            while(!ok) pause();
        }

        // list_treasures
        else if (strcmp(buff, "list_treasures") == 0)
        {
            ok = 0;
            if (monitor_pid != -1) {
                if (kill(monitor_pid, SIGUSR2) == -1) {
                    write(1, "Error sending signal\n", strlen("Error sending signal\n"));
                }
            } else {
                write(1, "Monitor is not running\n", strlen("Monitor is not running\n"));
            }
            while(!ok) pause();
        }

        // view_treasure
        else if (strcmp(buff, "view_treasure") == 0)
        {
            ok = 0;
            if (monitor_pid != -1) {
                if (kill(monitor_pid, SIGINT) == -1) {
                    write(1, "Error sending signal\n", strlen("Error sending signal\n"));
                }
            } else {
                write(1, "Monitor is not running\n", strlen("Monitor is not running\n"));
            }
            while(!ok) pause();
        }

        //stop_monitor
        else if (strcmp(buff, "stop_monitor") == 0)
        {
            stop_monitor();
        }

        //exit
        else if (strcmp(buff, "exit") == 0)
        {
            write(1, "Exiting...\n", strlen("Exiting...\n"));
            if (monitor_pid == -1)
                break;
            write(1, "Monitor still running...\n", strlen("Monitor still running...\n"));
        }

        //invalid command
        else
        {
            write(1, "Invalid command\n", strlen("Invalid command\n"));
        }
    }

    return 0;
}
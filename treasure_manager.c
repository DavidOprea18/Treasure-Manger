#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>

// ------------------------ STRUCT ------------------------ //

typedef struct
{
    int treasure_id;
    char user_name[50];
    float latitude_coordinates;
    float longitude_coordinates;
    char clue_text[100];
    int value;
} TREASURE;

// ------------------------ Create Directory ------------------------ //

void create_directory(const char *hunt_id)
{
    char directory[256];
    snprintf(directory, sizeof(directory), "%s", hunt_id);

    struct stat st;
    if (stat(directory, &st) == -1)
    {
        if (mkdir(directory, 0777) == -1)
        {
            perror("Failed to create the directory :(");
            exit(-1);
        }
    }
}

// ------------------------ treasures file path ------------------------ //

void get_treasures_file_path(const char *hunt_id, char *path, size_t path_size)
{
    snprintf(path, path_size, "%s/treasures.bin", hunt_id);
}

// ------------------------ log file path ------------------------ //

void get_log_file_path(const char *hunt_id, char *path, size_t path_size)
{
    snprintf(path, path_size, "%s/logged_hunt", hunt_id);
}

// ------------------------ Legaturi Simbolice ------------------------ //

void create_log_symlink(const char *hunt_id)
{
    char log_path[256];
    char symlink_path[256];
    get_log_file_path(hunt_id, log_path, sizeof(log_path));
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);

    unlink(symlink_path);
    if (symlink(log_path, symlink_path) == -1)
    {
        perror("Failed to create symlink");
    }
}

// ------------------------ LOG ------------------------ //

void log_operation(const char *hunt_id, const char *operation)
{
    char log_path[256];
    get_log_file_path(hunt_id, log_path, sizeof(log_path));

    int fd = open(log_path, O_CREAT | O_WRONLY | O_APPEND, 0777);
    if (fd == -1)
    {
        perror("Failed to open log file");
        return;
    }

    char buf[512];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    if (t == NULL)
    {
        snprintf(buf, sizeof(buf), "Failed to get local time\n");
    }
    else
    {
        snprintf(buf, sizeof(buf), "[%02d-%02d-%04d %02d:%02d:%02d] - %s\n", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, operation);
    }
    write(fd, buf, strlen(buf));
    close(fd);

    create_log_symlink(hunt_id);
}

// ------------------------ ADD <HUNT_ID> ------------------------ //

void add_treasure(const char *hunt_id)
{
    create_directory(hunt_id);

    char file_path[256];
    get_treasures_file_path(hunt_id, file_path, sizeof(file_path));

    int fd = open(file_path, O_CREAT | O_RDWR | O_APPEND, 0777);
    if (fd == -1)
    {
        perror("Failed to open treasures file");
        exit(-1);
    }

    TREASURE t;
    printf("Enter treasure ID: ");
    scanf("%d", &t.treasure_id);
    printf("Enter user name: ");
    scanf("%s", t.user_name);
    printf("Enter latitude: ");
    scanf("%f", &t.latitude_coordinates);
    printf("Enter longitude: ");
    scanf("%f", &t.longitude_coordinates);
    printf("Enter clue text: ");
    scanf("%s", t.clue_text);
    printf("Enter value: ");
    scanf("%d", &t.value);

    if (write(fd, &t, sizeof(TREASURE)) != sizeof(TREASURE))
    {
        perror("Failed to write treasure");
        close(fd);
        exit(-1);
    }

    close(fd);
    log_operation(hunt_id, "New treasure added");
}

// ------------------------ LIST <HUNT_ID> ------------------------ //

void list_treasures(const char *hunt_id)
{
    char file_path[256];
    get_treasures_file_path(hunt_id, file_path, sizeof(file_path));

    struct stat st;
    if (stat(file_path, &st) == -1)
    {
        perror("Failed to stat treasures file");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %lld bytes\n", st.st_size);
    printf("Last modified: %s\n", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open treasures file");
        return;
    }

    TREASURE t;
    while (read(fd, &t, sizeof(TREASURE)) == sizeof(TREASURE))
    {
        printf("Treasure ID: %d\n", t.treasure_id);
        printf("User name: %s\n", t.user_name);
        printf("GPS coordinates: (%f, %f)\n", t.latitude_coordinates, t.longitude_coordinates);
        printf("Clue text: %s\n", t.clue_text);
        printf("Value: %d\n\n", t.value);
    }

    close(fd);
    log_operation(hunt_id, "Listed treasures");
}

// ------------------------ VIEW <HUNT_ID> <ID> ------------------------ //

void view_treasure(const char *hunt_id, int treasure_id)
{
    char file_path[256];
    get_treasures_file_path(hunt_id, file_path, sizeof(file_path));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open treasures file");
        return;
    }

    TREASURE t;
    int found = 0;
    while (read(fd, &t, sizeof(TREASURE)) == sizeof(TREASURE))
    {
        if (t.treasure_id == treasure_id)
        {
            printf("Treasure ID: %d\n", t.treasure_id);
            printf("User name: %s\n", t.user_name);
            printf("GPS coordinates: (%f, %f)\n", t.latitude_coordinates, t.longitude_coordinates);
            printf("Clue text: %s\n", t.clue_text);
            printf("Value: %d\n", t.value);
            found = 1;
            break;
        }
    }

    if (found == 0)
    {
        printf("Treasure with ID %d not found in hunt %s\n", treasure_id, hunt_id);
    }

    close(fd);
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Viewed treasure ID %d", treasure_id);
    log_operation(hunt_id, log_msg);
}

// ------------------------ REMOVE_TREASURE <HUNT_ID> <ID> ------------------------ //

void remove_treasure(const char *hunt_id, int treasure_id)
{
    char file_path[256];
    get_treasures_file_path(hunt_id, file_path, sizeof(file_path));

    int fd = open(file_path, O_RDWR);
    if (fd == -1)
    {
        perror("Failed to open treasures file");
        return;
    }

    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s/temp.bin", hunt_id);
    int temp_fd = open(temp_path, O_CREAT | O_WRONLY, 0777);
    if (temp_fd == -1)
    {
        perror("Failed to create temp file");
        close(fd);
        return;
    }

    TREASURE t;
    int found = 0;
    while (read(fd, &t, sizeof(TREASURE)) == sizeof(TREASURE))
    {
        if (t.treasure_id != treasure_id)
        {
            write(temp_fd, &t, sizeof(TREASURE));
        }
        else
        {
            found = 1;
        }
    }

    close(fd);
    close(temp_fd);

    unlink(file_path);
    rename(temp_path, file_path);

    if (found == 0)
    {
        printf("Treasure with ID %d not found in hunt %s\n", treasure_id, hunt_id);
    }
    else
    {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Removed treasure ID %d", treasure_id);
        log_operation(hunt_id, log_msg);
    }
}

// ------------------------ REMOVE_HUNT <HUNT_ID> ------------------------ //

void remove_hunt(const char *hunt_id)
{
    char directory[256];
    snprintf(directory, sizeof(directory), "%s", hunt_id);

    char symlink_path[256];
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);
    unlink(symlink_path);

    char file_path[256];
    get_treasures_file_path(hunt_id, file_path, sizeof(file_path));
    unlink(file_path);

    char log_path[256];
    get_log_file_path(hunt_id, log_path, sizeof(log_path));
    unlink(log_path);

    if (rmdir(directory) == -1)
    {
        perror("Failed to remove directory");
    }
    else
    {
        printf("Hunt removed\n");
    }
}

// ------------------------ MAIN FUNCTION ------------------------ //

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        perror("Not enough arguments");
        exit(-1);
    }

    char *command = argv[1];
    char *hunt_id = argv[2];

    if (strcmp(command, "add") == 0)
    {
        if (argc != 3)
        {
            perror("Not enough arguments for add");
            exit(-1);
        }
        add_treasure(hunt_id);
    }
    else if (strcmp(command, "list") == 0)
    {
        if (argc != 3)
        {
            perror("Not enough arguments for list");
            exit(-1);
        }
        list_treasures(hunt_id);
    }
    else if (strcmp(command, "view") == 0)
    {
        if (argc != 4)
        {
            perror("Not enough arguments for view");
            exit(-1);
        }
        int treasure_id = atoi(argv[3]);
        view_treasure(hunt_id, treasure_id);
    }
    else if (strcmp(command, "remove_treasure") == 0)
    {
        if (argc != 4)
        {
            perror("Not enough arguments for remove_treasure");
            exit(-1);
        }
        int treasure_id = atoi(argv[3]);
        remove_treasure(hunt_id, treasure_id);
    }
    else if (strcmp(command, "remove_hunt") == 0)
    {
        if (argc != 3)
        {
            perror("Not enough arguments for remove_hunt");
            exit(-1);
        }
        remove_hunt(hunt_id);
    }
    else
    {
        perror("Not enough arguments");
        exit(-1);
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

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

typedef struct
{
    char user_name[50];
    int score;
} UserScore;

// ------------------------ MAIN FUNCTION ------------------------ //

int main(int argc, char **argv)
{
    DIR *dir;
    struct dirent *dp;

    dir = opendir(".");
    if (dir == NULL)
    {
        perror("Could not open project directory");
        exit(-1);
    }

    int has_hunts = 0;

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
            perror("Could not get file status");
            exit(-1);
        }

        if (S_ISDIR(st.st_mode))
        {
            has_hunts = 1;

            char treasure_path[256];
            snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.bin", path);

            int fd = open(treasure_path, O_RDONLY);
            if (fd == -1)
            {
                continue;
            }

            UserScore user_scores[100];
            int user_count = 0;

            TREASURE t;
            while (read(fd, &t, sizeof(TREASURE)) == sizeof(TREASURE))
            {
                int user_index = -1;
                for (int i = 0; i < user_count; i++)
                {
                    if (strcmp(user_scores[i].user_name, t.user_name) == 0)
                    {
                        user_index = i;
                        break;
                    }
                }

                if (user_index == -1)
                {
                    user_index = user_count;
                    strcpy(user_scores[user_count].user_name, t.user_name);
                    user_scores[user_count].score = 0;
                    user_count++;
                }

                user_scores[user_index].score += t.value;
            }
            close(fd);

            char directory_name[256];
            snprintf(directory_name, sizeof(directory_name), "%s:\n", dp->d_name);
            write(1, directory_name, strlen(directory_name));

            if (user_count == 0)
            {
                printf("No treasures found\n");
            }
            else
            {
                for (int i = 0; i < user_count; i++)
                {
                    char score_str[70];
                    snprintf(score_str, sizeof(score_str), "%s - score %d\n", user_scores[i].user_name, user_scores[i].score);
                    write(1, score_str, strlen(score_str));
                }
            }
            write(1, "\n", 1);
        }
    }

    closedir(dir);

    if (!has_hunts)
    {
        printf("No hunts found\n");
    }

    return 0;
}
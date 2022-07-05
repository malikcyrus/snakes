#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define PORT                5100 
#define ARENA_HEIGHT        24
#define ARENA_WIDTH         80
#define FRUIT               -1000
#define BORDER              -99
#define WINNER              -94
#define REFRESH             0.15
#define ONGOING             -34
#define INTERRUPTED         -30
#define UP_KEY              'W'
#define DOWN_KEY            'S'
#define LEFT_KEY            'A'
#define RIGHT_KEY           'D'

WINDOW *window;
char key = UP_KEY; 
int game_output = ONGOING;  

/**
 * @brief Error output 
 * 
 * @param err error message to display 
 */
void error_output(const char* err){
    perror(err); 
    exit(0); 
}

int generate_thread(void* (*fn)(void *), void* arg){
    int err; 
    pthread_t tid; 
    pthread_attr_t attr; 

    err = pthread_attr_init(&attr);
    if(err != 0)
        return err;
    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(err == 0)
        err = pthread_create(&tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    return err;
}

void* send_to_server(void* arg){
    int sockfd = *(int *) arg; 
    struct timespec ts; 
    ts.tv_sec = REFRESH; 
    ts.tv_nsec = ((int)(REFRESH * 1000) % 1000) * 1000000; 
    while(game_output == ONGOING){
        nanosleep(&ts, NULL); 
        int n = write(sockfd, &key, 1); 
        if(n < 0)
            error_output("Error writing to socket");
    }
    return 0; 
}

void* refresh_display(void* arg){
    int sockfd = *(int*) arg; 
    int bytes_read; 
    int arena[ARENA_HEIGHT][ARENA_WIDTH];
    int arena_size = ARENA_HEIGHT * ARENA_WIDTH * sizeof(arena[0][0]);
    char arena_buf[arena_size];
    int n; 
    
    while(game_output == ONGOING){
        bytes_read = 0; 
        bzero(arena, arena_size);
        while(bytes_read < arena_size){
            n = read(sockfd, arena_buf + bytes_read, arena_size - bytes_read);
            if(n <= 0) {
                game_output = arena[0][0]; 
                return 0; 
            }
            bytes_read += n; 
        }
        memcpy(arena, arena_buf, arena_size); 

        clear(); 
        box(window, '+', '+');
        refresh(); 
        wrefresh(window); 

        for(int i = 1; i < ARENA_HEIGHT-1; i++){
            for(int j = 1; j < ARENA_WIDTH - 1; i++){
                int index = arena[i][j]; 
                int color = abs(index) % 7; 
                attron(COLOR_PAIR(color));
                if((index > 0) && (index != FRUIT)){
                    mvprintw(i, j, "  ");
                    attroff(COLOR_PAIR(color));
                }
                else if ((index < 0) && (index != FRUIT)){
                    if(arena[i-1][j] == -index)
                        mvprintw(i, j, "..");
                    else if(arena[i+1][j] == -index)
                        mvprintw(i, j, "**");
                    else if(arena[i][j-1] == -index)
                        mvprint(i, j, " :"); 
                    else if(arena[i][j+1] == -index)
                        mvprintw(i, j, ": ");
                    attroff(COLOR_PAIR(color));
                }
                else if (index == FRUIT){
                    attroff(COLOR_PAIR(color));
                    mvprintw(i, j, "o");
                }
            }
        }
        refresh();   
    }

    game_output = arena[0][0]; 
    return 0; 
}

int main(int argc, char *argv[])
{
    int sockfd; 
    struct sockaddr_in serv_addr; 
    struct hostent* server; 
    char key_buf; 

    if (argc < 2){
        fprintf(stderr, "Please type:\n\t %s [server ip]\n to launch the game.\n", argv[0]); 
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) 
        error_output("ERROR opening socket");
    
    server = gethostbyname(argv[1]);
    if (server == NULL){
        fprintf(stderr, "ERROR, no such host exits.\n");
        exit(0); 
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; 
    bcopy((char *) server->h_addr, 
        (char *) &serv_addr.sin_addr.s_addr, 
        server->h_length);
    
    serv_addr.sin_port = htons(PORT); 

    if (connect(sockfd, (struct sockaddr * ) &serv_addr, sizeof(serv_addr)) < 0)
        error_output("ERROR connecting");

    // initialising ncurses  window 
    initscr(); 
    cbreak(); 
    noecho(); 
    start_color(); 
    use_default_colors(); 
    curs_set(0); 

    window = newwin(ARENA_HEIGHT, ARENA_WIDTH, 0, 0); 

    // snake colors 
    init_pair(0, COLOR_WHITE, COLOR_BLUE);
    init_pair(1, COLOR_WHITE, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_GREEN);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);
    init_pair(4, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(5, COLOR_BLACK, COLOR_CYAN);
    init_pair(6, COLOR_BLACK, COLOR_WHITE);

    mvprintw((ARENA_HEIGHT-20)/2, (ARENA_WIDTH-58)/2,"                          _");
    mvprintw((ARENA_HEIGHT-20)/2 + 1, (ARENA_WIDTH-58)/2," __                      | |");
    mvprintw((ARENA_HEIGHT-20)/2 + 2, (ARENA_WIDTH-58)/2,"{OO}      ___ _ __   __ _| | _____  ___");
    mvprintw((ARENA_HEIGHT-20)/2 + 3, (ARENA_WIDTH-58)/2,"\\__/     / __| '_ \\ / _` | |/ / _ \\/ __|");
    mvprintw((ARENA_HEIGHT-20)/2 + 4, (ARENA_WIDTH-58)/2," |^|     \\__ \\ | | | (_| |   <  __/\\__ \\");
    mvprintw((ARENA_HEIGHT-20)/2 + 5, (ARENA_WIDTH-58)/2," | |     |___/_| |_|\\__,_|_|\\_\\___||___/   v1.0  /\\");
    mvprintw((ARENA_HEIGHT-20)/2 + 6, (ARENA_WIDTH-58)/2," | |____________________________________________/ /");
    mvprintw((ARENA_HEIGHT-20)/2 + 7, (ARENA_WIDTH-58)/2," \\_______________________________________________/");
    mvprintw((ARENA_HEIGHT-20)/2 + 10, (ARENA_WIDTH-58)/2," Instructions:"); 
    mvprintw((ARENA_HEIGHT-20)/2 + 12, (ARENA_WIDTH-58)/2," - Use the keys w, a, s, d to move your snake.");
    mvprintw((ARENA_HEIGHT-20)/2 + 13, (ARENA_WIDTH-58)/2," - Eat fruit to grow in length.");
    mvprintw((ARENA_HEIGHT-20)/2 + 14, (ARENA_WIDTH-58)/2," - Do not run in to other snakes, the game border"); 
    mvprintw((ARENA_HEIGHT-20)/2 + 15, (ARENA_WIDTH-58)/2,"   or yourself.");
    mvprintw((ARENA_HEIGHT-20)/2 + 16, (ARENA_WIDTH-58)/2," - The first snake to reach length 15 wins!");
    mvprintw((ARENA_HEIGHT-20)/2 + 17, (ARENA_WIDTH-58)/2," - Press '.' to quit at any time.");
    mvprintw((ARENA_HEIGHT-20)/2 + 19, (ARENA_WIDTH-58)/2,"Press any key to start . . ."); 
    getch();

    generate_thread(refresh_display, &sockfd);
    generate_thread(send_to_server, &sockfd);

    while(game_output == ONGOING){

        bzero(&key_buf, 1); 
        timeout(REFRESH * 1000); 
        key_buf = getch(); 
        key_buf = toupper(key_buf); 
        if (key_buf == '.'){
            game_output = INTERRUPTED; 
            break; 
        } else if((key_buf == UP_KEY)
                || (key_buf == DOWN_KEY)
                || (key_buf == LEFT_KEY)
                || (key_buf == RIGHT_KEY))
            key = key_buf; 
    }
    // Display winner
    WINDOW* announcement = newwin(7, 35, (ARENA_HEIGHT - 7)/2, (ARENA_WIDTH - 35)/2);
    box(announcement, 0, 0);
    if (game_output == WINNER){
        mvwaddstr(announcement, 2, (35-21)/2, "GAME OVER! - You WIN!");
        mvwaddstr(announcement, 4, (35-21)/2, "Press any key to quit.");
        wbkgd(announcement, COLOR_PAIR(2));
    } else {
        mvwaddstr(announcement, 2, (35-21)/2, "Game over - you lose!");
        if(game_output > 0){
            mvwprintw(announcement, 3, (35-13)/2, "Player %d won.", game_output);    
        }
        mvwaddstr(announcement, 4, (35-21)/2, "Press any key to quit.");
        wbkgd(announcement, COLOR_PAIR(1)); 
    }
    mvwin(announcement, (ARENA_HEIGHT - 7)/2, (ARENA_WIDTH - 35)/2);
    wnoutrefresh(announcement); 
    wrefresh(announcement); 
    sleep(2); 
    wgetch(announcement); 
    delwin(announcement);
    wclear(window); 

    echo();
    curs_set(1); 
    endwin(); 

    close(sockfd); 
    return 0; 
} 


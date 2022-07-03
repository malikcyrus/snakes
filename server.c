#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PORT                5100 
#define MAX_PLAYERS         1000
#define ARENA_HEIGHT        24
#define ARENA_WIDTH         80
#define MAX_SNAKE_LEN       ARENA_HEIGHT * ARENA_WIDTH
#define BEGIN_SNAKE_LEN     3
#define KING_SNAKE_LEN      15 
#define FRUIT               -1000
#define BORDER              -99
#define WINNER              -94
#define UP_KEY              'W'
#define DOWN_KEY            'S'
#define LEFT_KEY            'A'
#define RIGHT_KEY           'D'

// arena 
int arena[ARENA_HEIGHT][ARENA_WIDTH];
int arena_size = ARENA_HEIGHT * ARENA_WIDTH * sizeof(arena[0][0]);
int won = 0; 
pthread_mutex_t arena_lock = PTHREAD_MUTEX_INITIALIZER; 

// keys 
typedef enum{
    UP = UP_KEY, 
    DOWN = DOWN_KEY, 
    LEFT = LEFT_KEY, 
    RIGHT = RIGHT_KEY
} direction; 

// Point on map
typedef struct{
    int x, y; 
    direction d; 
} coordinate; 

// snake structure 
typedef struct{
    int player_id, length; 
    coordinate head; 
    coordinate body[MAX_SNAKE_LEN -2];
    coordinate tail; 
} snake;

/**
 * @brief 
 * Function to return a snake struct 
 * @param player_id ID of player 
 * @param head_y y coordinate of head of snake
 * @param head_x x coordinate of head of snake 
 * @return snake* struct snake 
 */
snake* snake_init(int player_id, int head_y, int head_x){
    
    // enter snake on arena 
    // lock arena_lock mutex 
    pthread_mutex_lock(&arena_lock);
    arena[head_y][head_x] = -player_id; 
    arena[head_y + 1][head_x] = 
    arena[head_y + 2][head_x] = player_id; 

    // unlock the mutex 
    pthread_mutex_unlock(&arena_lock);

    // allocate memory for snake 
    snake* s = malloc(sizeof(snake));

    // set properties of snake with 
    s->player_id = player_id; 
    s->length = BEGIN_SNAKE_LEN; 

    s->head.y = head_y; 
    s->head.x = head_x; 
    s->head.d = UP; // initial direction 

    s->body[0].y = head_y + 1; 
    s->body[0].x = head_x + 1; 
    s->body[0].d = UP; 

    s->tail.y = head_y + 2; 
    s->tail.x = head_x;
    s->tail.d = UP;

    return s; 
}

/**
 * @brief Function to kill a snake 
 * @param s Snake to kill 
 */
void kill_snake(snake *s){
    // Lock Mutex 
    pthread_mutex_lock(&arena_lock);

    // set arena coordinates to 0 
    arena[s->head.y][s->head.x] = 
    arena[s->tail.y][s->tail.y] = 0; 
    
    // delete body of snake 
    for(int i = 0; i < s->length - 2; i++)
        arena[s->body[i].y][s->body[i].x] = 0; 

    // unlock mutex 
    pthread_mutex_unlock(&arena_lock);

    // free snake and set pointer to null 
    free(s);
    s = NULL; 
}

/**
 * @brief Function to move a snake in a specified direction 
 * 
 * @param s Sanke to move 
 * @param d Direction to given snake in (UP, DOWN, LEFT, RIGHT)
 */
void snake_move(snake *s, direction d) { 
    // move snake 
    memmove(&(s->body[1]), &(s->body[0]), 
            (s->length-2) * sizeof(coordinate));

    // set head properties 
    s->body[0].y = s->head.y; 
    s->body[0].x = s->head.x; 
    s->body[0].d = s->head.d; 
    
    // set direction 
    switch(d){
        case UP:{
            s->head.y = s->head.y-1; 
            s->head.d = UP; 
            break;
        }
        case DOWN:{
            s->head.y = s->head.y+1; 
            s->head.d = DOWN; 
            break;
        }
        case LEFT:{
            s->head.x = s->head.x-1; 
            s->head.d = LEFT; 
            break;
        }
        case RIGHT:{
            s->head.x = s->head.x+1; 
            s->head.d = RIGHT; 
            break; 
        }
        default: break; 
    }

    // lock mutex 
    pthread_mutex_lock(&arena_lock);
    // display player id 
    arena[s->head.y][s->head.x] = -(s->player_id);
    arena[s->body[0].y][s->body[0].x] = s->player_id; 
    arena[s->tail.y][s->tail.x] = 0; 
    pthread_mutex_unlock(&arena_lock);

    s->tail.y = s->body[s->length-2].y; 
    s->tail.x = s->body[s->length-2].x;
}

/**
 * @brief Function to add fruit on the arena 
 * 
 */
void add_fruit(){
    int x, y; 

    // generate random coordinates to add fruit to 
    do{
        y = rand() % (ARENA_HEIGHT - 6) + 3; 
        x = rand() % (ARENA_WIDTH - 6) + 3; 
    } while (arena[x][y] != 0);

    // lock mutex 
    pthread_mutex_lock(&arena_lock);

    // set coordinate to show fruit at 
    arena[y][x] = FRUIT; 

    // unlock mutex 
    pthread_mutex_unlock(&arena_lock);
}

/**
 * @brief Function to consume fruit 
 * 
 * @param s snake that ate the fruit 
 * @param d Direction the snake was moving in 
 */
void consume_fruit(snake* s, direction d){
    // move snake 
    memmove(&(s->body[1]), &(s->body[0]), 
            (s->length-2) * sizeof(coordinate));

    // set properties 
    s->body[0].y = s->head.y; 
    s->body[0].x = s->head.x; 
    s->body[0].d = s->head.d; 

    // move snake in direction 
     switch(d){
        case UP:{
            s->head.y = s->head.y-1; 
            s->head.d = UP; 
            if(arena[s->head.y][s->head.x + 1] == FRUIT){
                pthread_mutex_lock(&arena_lock); // locking the thread 
                arena[s->head.y][s->head.x + 1] = 0; 
                pthread_mutex_unlock(&arena_lock);
            }
            break;
        }
        case DOWN:{
            s->head.y = s->head.y+1; 
            s->head.d = DOWN; 
            if(arena[s->head.y][s->head.x + 1] == FRUIT){
                pthread_mutex_lock(&arena_lock);
                arena[s->head.y][s->head.x + 1] = 0; 
                pthread_mutex_unlock(&arena_lock);
            }
            break;
        }
        case LEFT:{
            s->head.x = s->head.x-1; 
            s->head.d = LEFT; 
            break;
        }
        case RIGHT:{
            s->head.x = s->head.x+1; 
            s->head.d = RIGHT; 
            break; 
        }
        default: break; 
    }

    pthread_mutex_lock(&arena_lock);
    arena[s->head.y][s->head.x] = -(s->player_id);
    arena[s->body[0].y][s->body[0].x] = s->player_id; 
    pthread_mutex_lock(&arena_lock);
    s->length++;
    add_fruit(); 
}

/**
 * @brief Function to generate a thread 
 * https://docs.oracle.com/cd/E19455-01/806-5257/attrib-69011/index.html
 * @param fn 
 * @param arg 
 * @return int 
 */
int generate_thread(void* (*fn)(void *), void *arg){
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

/**
 * @brief Error output 
 * 
 * @param err error message to display 
 */
void error_output(const char* err){
    perror(err); 
    // flush the stream 
    fflush(stdout); 
    exit(1); 
}

/**
 * @brief Function to destroy the server 
 * 
 */
void exit_handler(){
    printf("Exiting Server, RIP.\n");
    exit(0);
}

/**
 * @brief a thread of gameplay
 * 
 * @param arg arguments 
 * @return void* Pointer to a gameplay 
 */
void* game(void *arg){
    int fd = *(int*) arg; 
    int player_id = fd-3; 
    printf("Entered player_id: %d\n", player_id);

    int head_y, head_x; 
    srand(time(NULL)); 
    do{
        head_y = rand() % (ARENA_HEIGHT - 6) + 3;
        head_x = rand() % (ARENA_WIDTH - 6) + 3;     
    } while(!(
        ((arena[head_y][head_x] == arena[head_y+1][head_x])
        == arena[head_y+2][head_x]) == 0));

    snake *player = snake_init(player_id, head_y, head_x);

    char key = UP; 
    char key_buff; 
    char arena_buf[arena_size];
    int bytes_sent, n; 
    int success = 1; 

    while(success){
        if(won)
            success = 0; 
        
        if(player->length >= 15){
            won = player_id; 
            pthread_mutex_lock(&arena_lock);
            arena[0][0] = WINNER; 
            pthread_mutex_unlock(&arena_lock);
        } else if(arena[0][0] != BORDER) {
            pthread_mutex_lock(&arena_lock); 
            arena[0][0] = won; 
            pthread_mutex_unlock(&arena_lock);
        }

        memcpy(arena_buf, arena, arena_size);
        bytes_sent = 0; 
        while(bytes_sent < arena_size){
            bytes_sent += write(fd, arena, arena_size);
            if(bytes_sent < 0) error_output("ERROR writing to socket");
        }

        bzero(&key_buff, 1); 
        n = read(fd, &key_buff, 1);
        if (n <= 0)
            break; 
        
        key_buff = toupper(key_buff); 
        if( ((key_buff == UP) && !(player->head.d == DOWN))
            ||((key_buff == DOWN) && !(player->head.d == UP))
            ||((key_buff == LEFT) && !(player->head.d == RIGHT))
            ||((key_buff == RIGHT) && !(player->head.d == LEFT)))
            key = key_buff; 

        switch(key){
            case UP:{
                if((arena[player->head.y-1][player->head.x] == 0) &&
                    !(arena[player->head.y-1][player->head.x+1] == FRUIT)){
                        snake_move(player, UP); 
                        printf("Player %d moved 1 up\n", player_id); 
                }
                else if((arena[player->head.y-1][player->head.x] == FRUIT) ||
                    (arena[player->head.y-1][player->head.x+1] == FRUIT)){
                        consume_fruit(player, UP); 
                        printf("%d ate fruit\n", player_id);
                }
                else{
                    snake_move(player, LEFT); 
                    success = 0; 
                }
                break; 
            }

            case DOWN:{
                if((arena[player->head.y+1][player->head.x] == 0) &&
                    !(arena[player->head.y+1][player->head.x+1] == FRUIT)){
                        snake_move(player, DOWN); 
                        printf("Player %d moved 1 down\n", player_id); 
                }
                else if((arena[player->head.y+1][player->head.x] == FRUIT) ||
                    (arena[player->head.y+1][player->head.x+1] == FRUIT)){
                        consume_fruit(player, DOWN); 
                        printf("%d ate fruit\n", player_id);
                }
                else{
                    snake_move(player, DOWN); 
                    success = 0; 
                }
                break;
            }

            case RIGHT:{
                if(arena[player->head.y][player->head.x+1] == 0){
                    snake_move(player, RIGHT); 
                    printf("Player %d moved Right\n", player_id);
                }
                else if(arena[player->head.y][player->head.x+1] == FRUIT){
                    consume_fruit(player, RIGHT); 
                    printf("Player %d ate fruit\n", player_id);
                }
                else{
                    snake_move(player, RIGHT);
                    success = 0; 
                }
                break; 
            }

            case LEFT:{
                if(arena[player->head.y][player->head.x-1] == 0) {
                    snake_move(player, LEFT); 
                    printf("Player %d moved left\n", player_id);
                }
                else if(arena[player->head.y][player->head.x-1] == FRUIT) {
                    consume_fruit(player, LEFT);
                    printf("Played %d ate fruit\n", player_id);
                }
                else {
                    snake_move(player, LEFT); 
                    success = 0; 
                }
                break;
            }

            default: break; 
        }   
    }

    if(player->length == KING_SNAKE_LEN){
        fprintf(stderr, "Player %d won\n", player_id);
        kill_snake(player); 
        close(fd); 
        return 0; 
    } else {
        fprintf(stderr, "Player %d left the game\n.", player_id);
        kill_snake(player); 
        close(fd); 
        return 0; 
    }
}

// Main function 
int main( int argc, char *argv[] )
{
    int     socket_fds[MAX_PLAYERS]; 
    struct sockaddr_in socket_addr[MAX_PLAYERS];
    int     i; 

    // Exit handler  
    signal(SIGINT, exit_handler); 

    memset(arena, 0, arena_size); 

    for(int i = 0; i < ARENA_HEIGHT; i++){
        arena[i][0] = arena[i][ARENA_WIDTH - 2] = BORDER; 
    }

    for(int i = 0; i < ARENA_WIDTH; i++) {
        arena[0][i] = arena[ARENA_HEIGHT - 1][i] = BORDER; 
    }

    // adding fruit randomly 
    for(int i = 0; i < 3; i++){
        add_fruit(); 
    }

    // Creating server socket 
    socket_fds[0] socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_fds[0] < 0) {
        error_output("ERROR Opening socket");
    }

    bzero((char *) &socket_addr[0], sizeof(socket_addr[0])); 
    socket_addr[0].sin_family = AF_INET; 
    socket_addr[0].sin_addr.s_addr = INADDR_ANY; 

    //Converting unsigned short integer from host byte order to network byte order. 
    socket_addr[0].sin_port = htons(PORT); 

    // assigning address specified by address to the socket referred by the server socket fd 
    if (bind(socket_fds[0], (struct sockaddr *) &socket_addr[0], sizeof(socket_addr[0])) < 0) {
        error_output("ERROR on binding");
    }

    // Listener for requests 
    listen(socket_fds[0], 5); 
    socklen_t clilen = sizeof(socket_addr[0]); 

    for(int i = 1; i++) { 
        // accept an incoming request 
        socket_fds[i] = accept(socket_fds[0], (struct sockaddr *) &socket_addr[i], &clilen); 
        if (socket_fds[i] < 0) {
            error_output("Error accepting request"); 
        }

        // reset 
        if(won){
            printf("Restarting game\n");
            won = 0; 
        }

        generate_thread(&game, &socket_fds[i]);
    }

    close(socket_fds[0]); 
    return 0; 
}

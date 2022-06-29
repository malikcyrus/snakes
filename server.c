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

}

int main( int argc, char *argv[] )
{
    // Declare variables
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    // Create server socket (AF_INET, SOCK_STREAM)
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    // Initialize socket structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno = 5001;  // Server port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any address
    serv_addr.sin_port = htons(portno);
 
    // Bind the host address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
		 close(sockfd);
         exit(-1);
    }

    // Start listening for the clients
    if (listen(sockfd, 5) < 0) {
        perror("ERROR on listening\n");
        close(sockfd);
        exit(-1);
    }
    clilen = sizeof(cli_addr);

    // Accept actual connection from the client
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) 
    {
        perror("ERROR on accept");
		close(sockfd);
        exit(-1);
    }

    // If connection is established then start communicating 
    memset(buffer, 0, 256);
    if ((n = read(newsockfd, buffer, 255 )) < 0)
    {
        perror("ERROR reading from socket");
		close(sockfd);
		close(newsockfd);
        exit(1);
    }
    printf("Here is the message: %s",buffer);

    // Write a response to the client
    if ((n = write(newsockfd, "I got your message", 18)) < 0)
    {
        perror("ERROR writing to socket");
		close(newsockfd);
		close(sockfd);
        exit(1);
    }

    // All done, close sockets
    close(newsockfd);
    close(sockfd);
    exit(0);
}

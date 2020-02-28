#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include<ncurses.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include<stdlib.h>
#include <sys/mman.h>
#define SEM_GET "SemGet"
#define SEM_GOT "SemGot"
#define SEM_P "SemPlayer"  // Changing player data

#define SHM_PLAYER1 "ShmPlayer1"
#define SHM_PLAYER2 "ShmPlayer2"
#define SHM_PLAYER3 "ShmPlayer3"
#define SHM_PLAYER4 "ShmPlayer4"
#define SHM_SERV_ALIVE "ShmServAlive"
#define SHM_PLAYER1_INF "ShmPlayer1Inf"
#define SHM_PLAYER2_INF "ShmPlayer2Inf"
#define SHM_PLAYER3_INF "ShmPlayer3Inf"
#define SHM_PLAYER4_INF "ShmPlayer4Inf"

#define SHM_ROOM "ShmRoom"
sem_t *sem_get;
sem_t *sem_got;
sem_t *sem_p;
pthread_t th,th2;
struct room* room;
int num;
int r;

struct player_t *player;
int p;

int p_inf;
struct player_info_t *player_inf;

struct point_t
{
    int x,y;
};
struct player_t  // Player can change
{
    int Pid; //PID id
    int Type; // 0 Human , 1 Bot
    char key; // move wasd
    int p_number;
    int asigned;
    
    
};
struct player_info_t  // Player can't change
{
    int x,y; // position in map
    int sx,sy; //starting point
    int map[5][5]; // part of the map
    int server_Pid; // Server Pid
    int p_number; // numer that player signed to
    int deaths; // number of player's deaths
    int coins_f; // coins found
    int coins_b; // coins brought to campsite
    int type;
    int camp_x;
    int camp_y;
    int round;
   
};

struct room
{
   int player[4];
   int serv_alive;
   int full;
   
};
void handle_sigint(int sig) 
{ 
    pthread_cancel(th);
    pthread_cancel(th2);
    room->player[num] = 0;
    player->Pid = 0;
    player->asigned = 0;
    room->full = 0;
    
    unlink(SEM_P);
    unlink(SEM_GET);
    unlink(SEM_GOT);
    munmap(room, sizeof(struct room));
    munmap(player,sizeof(struct player_t));
    close(r);
    close(p);
    echo();
    endwin();
    exit(0);

} 
static void err(int c,const char* msg)
{
    if(!c)
        return;
    perror(msg);
    exit(1);    
}
void printch(int x,int y,char ch,int color)
{
   attron(COLOR_PAIR(color));
   mvaddch(x,y,ch);
   attroff(COLOR_PAIR(color));
}
void init_colors()
{
   start_color();
   init_pair(1, COLOR_BLACK, COLOR_WHITE);
   init_pair(2, COLOR_CYAN, COLOR_BLUE);
   init_pair(3, COLOR_BLACK, COLOR_GREEN);
   init_pair(4, COLOR_BLACK, COLOR_YELLOW);
   init_pair(5,COLOR_YELLOW,COLOR_GREEN);
   init_pair(6,COLOR_RED,COLOR_WHITE);
}
void print_statistics(struct player_info_t * S)
{
    mvprintw(1,60,"Server's PID: %d",S->server_Pid);
    if(S->camp_x != 0 && S->camp_y !=0)
        mvprintw(2,60,"Campsote X/Y: %3d/%3d",S->camp_x,S->camp_y);
    else    
        mvprintw(2,60,"Campsote X/Y: unknown");
    mvprintw(3,60,"Round number: %8d",S->round);

    mvprintw(6,60,"Player:");

    mvprintw(7,60,"Number:");
    mvprintw(7,75,"%d",S->p_number);

    mvprintw(8,60,"Type:");
    if(S->type == 0)
        mvprintw(8,75,"HUMAN");
    else
        mvprintw(8,75,"BOT");

    mvprintw(9,60,"Curr X/Y:");
    mvprintw(9,75,"%2d/%2d",S->x,S->y);

    mvprintw(10,60,"Deaths:");
    mvprintw(10,75,"%3d",S->deaths);

    mvprintw(12,60,"Coins found:");
    mvprintw(12,75,"%5d",S->coins_f);

    mvprintw(13,60,"Coins brought:");
    mvprintw(13,75,"%5d",S->coins_b);

}
void print_legend()
{
   attron(COLOR_PAIR(3));
   mvprintw(17,58,"wsad");
   mvprintw(18,58,"1234");
   attroff(COLOR_PAIR(3));
   printch(19,58,' ',2);
   printch(20,58,'#',1);
   printch(21,58,'*',6);
   printch(22,58,'c',4);
   printch(23,58,'t',4);
   printch(24,58,'T',4);
   printch(25,58,'A',5);
  
   mvprintw(17,63," - poruszanie graczem");
   mvprintw(18,63," - players");
   mvprintw(19,63," - wall");
   mvprintw(20,63," - bushes (slow down)");
   mvprintw(21,63," - wild beast");
   mvprintw(22,63," - one coin");
   mvprintw(23,63," - treasure (10 coins)");
   mvprintw(24,63," - large treasure (50 coins)");
   mvprintw(25,63," - campsite");

}
void *printing_map(void *arg)
{
    struct player_info_t * P = (struct player_info_t*)arg;
    
    sem_wait(sem_p);
    initscr();
    init_colors();
    sem_post(sem_p);
    

    while(1)
    {   sem_wait(sem_p);
       
        for(int i=0;i<5;i++)
        {
            for(int j=0;j<5;j++)
            {
                
                if(P->map[i][j] == '0')
                {
                    printch(i+2,j+2,' ',1);
                }
                else if(P->map[i][j] == '0' + 1)
                {
                    printch(i+2,j+2,' ',2);
                } 
                else if(P->map[i][j] == '0' + 2)
                {
                    printch(i+2,j+2,'#',1);
                } 
                else if(P->map[i][j] == '0' + 3)
                    printch(i+2,j+2,'1',3);
                else if(P->map[i][j] == '0' + 4)
                    printch(i+2,j+2,'2',3);
                else if(P->map[i][j] == '0' + 5)
                    printch(i+2,j+2,'3',3);
                else if(P->map[i][j] == '0' + 6) 
                    printch(i+2,j+2,'4',3);  
                else if(P->map[i][j] == '0' + 7)    
                    printch(i+2,j+2,'D',4); 
                else if(P->map[i][j] == '0' + 8)    
                    printch(i+2,j+2,'c',4);
                else if(P->map[i][j] == '0' + 9)    
                    printch(i+2,j+2,'t',4);
                else if(P->map[i][j] == '0' + 10)    
                    printch(i+2,j+2,'T',4);  
                else if(P->map[i][j] == '0' + 11)    
                    printch(i+2,j+2,'A',5);    
                else if(P->map[i][j] == '0' + 12)    
                    printch(i+2,j+2,'*',6);            
                                  
            }
        }
        print_statistics(P);
        print_legend();
        refresh();
        sem_post(sem_p);
        
        usleep(15000);
    }
}
void *moving(void * arg)
{
    struct player_t * P = (struct player_t*)arg;

   sem_wait(sem_p); 
   WINDOW *w;
   w = initscr();
   raw();
   noecho();
   
   sem_post(sem_p);
   //raw();
   
    P->key  = 'w';
    while(1)
    {  
      
    
      fflush(stdout);
      char c = wgetch(w);
        if(c == 'q' || c=='Q')
        {

            pthread_cancel(th2);
            sem_wait(sem_p);
            room->player[num] = 0;
            player->Pid = 0;
            player->asigned = 0;
            room->full = 0;
            sem_post(sem_p);
            unlink(SEM_P);
            unlink(SEM_GET);
            unlink(SEM_GOT);
            munmap(room, sizeof(struct room));
            munmap(player,sizeof(struct player_t));
            close(r);
            close(p);
            echo();
            endwin();
            
            
            exit(0);
        }
        else
            P->key  = c;
         
        
    }
    endwin();
    
}

int main(int argc, char **argv) {

system("printf '\033[8;40;120t'");
signal(SIGINT, handle_sigint);
int a;
while(1)//Czekam na start servera
{
   a = shm_open(SHM_SERV_ALIVE,O_RDONLY,0666);
    if(a == -1)
        usleep(1000000);
    else
        break;
}
//Open room 
int r = shm_open(SHM_ROOM,O_CREAT | O_RDWR, 0666);
err(r == -1, "shm_open");
room=(struct room*)mmap(NULL, sizeof(struct room), PROT_READ | PROT_WRITE,MAP_SHARED, r, 0);
err(room == NULL , "mmap");



 
sem_get = sem_open(SEM_GET,O_CREAT);
sem_got = sem_open(SEM_GOT,O_CREAT);

pid_t pid = getppid();


num=-1;
int flag =0;

sem_wait(sem_got);//Czekam na dostęp do poczekalni  
//Sprawdzam czy jest miejsce, jeżeli nie ma,kończe proces
if(room->full == 1)
{
    
    printf("\nBRAK WOLNEGO MIEJSCA\n");
    munmap(room, sizeof(struct room));
    close(r);
    close(a);
    unlink(SHM_ROOM);
    unlink(SHM_SERV_ALIVE);
    sem_post(sem_got);//zwracam dostęp do poczekalni
    unlink(SEM_GET);
    unlink(SEM_GOT);
    exit(0);
}
//szukam wolnego miejsca 
for(int i = 0;i<4;i++)
{
    if(room->player[i] == 0)
    {
            room->player[i] = pid;
            num = i;
            flag = 1;
            break;
    }
       
}


switch(num)
{
    case 0:
        p = shm_open(SHM_PLAYER1,O_CREAT | O_RDWR, 0666);
        p_inf=  shm_open(SHM_PLAYER1_INF,O_RDONLY, 0666);
        break;
    case 1:
        p = shm_open(SHM_PLAYER2,O_CREAT | O_RDWR, 0666);
        p_inf=  shm_open(SHM_PLAYER2_INF,O_RDONLY, 0666);
        break;
    case 2:
        p = shm_open(SHM_PLAYER3,O_CREAT | O_RDWR, 0666);
        p_inf=  shm_open(SHM_PLAYER3_INF,O_RDONLY, 0666);
         break;
    case 3:
        p = shm_open(SHM_PLAYER4,O_CREAT | O_RDWR, 0666);
        p_inf=  shm_open(SHM_PLAYER4_INF,O_RDONLY, 0666);
        break;   

}
player=(struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE,MAP_SHARED, p, 0);
err(player == NULL , "mmap");

player_inf=(struct player_info_t*)mmap(NULL, sizeof(struct player_info_t), PROT_READ ,MAP_SHARED, p_inf, 0);
err(player_inf == NULL , "mmap"); 
player->Pid = pid;
player->Type = 0;
player->key = ' ';
player->p_number = num + 1;
player->asigned = 1;


sem_post(sem_got);//Zwalniam dostępn do poczekalni
sem_post(sem_get);//informuje serwer że znalazłem miejsce w poczekalni

sem_p = sem_open(SEM_P,O_CREAT);
usleep(100000);
pthread_create(&th2,NULL,printing_map,player_inf); 
usleep(100000);
pthread_create(&th,NULL,moving,player);

pthread_join(th2,NULL);
pthread_join(th,NULL);
  




}

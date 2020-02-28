#include <stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <ncurses.h>

#define SHM_ROOM "ShmRoom"
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

pthread_t th; //dispaly
pthread_t th2; //move
pthread_t th3; // keyboard
pthread_t b[10]; //beast
sem_t *sem_get,*sem_got; // entering room
sem_t *sem_p; // used in function move and display

pid_t pid;
struct room *room;
int r;

int a;
int *serv_alive;

struct player_t *player1;
struct player_t *player2;
struct player_t *player3;
struct player_t *player4;
int p1,p2,p3,p4;

struct player_info_t *player_inf1;
struct player_info_t *player_inf2;
struct player_info_t *player_inf3;
struct player_info_t *player_inf4;
int pi1,pi2,pi3,pi4;

struct room
{
   int player[4];
   int serv_alive;
   int full;
};

static void err(int c,const char* msg)
{
    if(!c)
        return;
    perror(msg);
    exit(1);    
}
struct point_t
{
    int x,y;
};
struct treasure_t
{
   int x,y;
   int type; // 0 - player (D) 1 - coin(c) 2 - small tresure(t) 3 -big treasure(T)
   int visible;
   int value;
};
struct beast_t
{
   int x;
   int y;
   int visible;
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
    int type; // 0-human 1-CPU
    int camp_x; //camp cords
    int camp_y; //camp cords
    int round; //round
   
};
struct server_t
{
    struct player_info_t *player_info[4];
    struct player_t *player[4];
    int map[27][53]; // Original map
    int server_Pid;
    int round;
    struct point_t campsite; //campsite coords
    struct treasure_t treasure[20]; // treasures are randomly drawn
    struct treasure_t treasure_add[20]; // treasures are added by button
    struct beast_t beast[10] ; // 10 beasts you can make them active by pressing b
    
};
void init_beast(struct server_t *S)
{
   for(int i=0;i<10;i++)
   {
      S->beast[i].x=0;
      S->beast[i].y=0;
      S->beast[i].visible=0;
   }
}
void *add_beast(struct server_t *S)
{
   
   int i=0;
   for(;i<10;i++)
   {
      if(S->beast[i].visible == 0)
          break;
   }
   
   while(1) 
   {
      S->beast[i].x = rand() % 20 + 2;
      S->beast[i].y = rand() % 50 + 2;
         
      if(S->map[S->beast[i].x][S->beast[i].y] == '0')
      {
         S->beast[i].visible = 1;
         break;
      }
            
   } 
    return NULL;
}
void init_tresures_add(struct server_t *S)
{
   for(int i=0;i<20;i++)
   {
      S->treasure_add[i].visible = 0;
      S->treasure_add[i].value = 0;
      S->treasure_add[i].x = 0;
      S->treasure_add[i].y = 0;
      S->treasure_add[i].type = 0;
   }
}
void init_tresures(struct server_t *S)
{
   
   for(int i=0;i<4;i++)
   {
      S->treasure[i].visible = 0;
      S->treasure[i].value = 0;
      S->treasure[i].x = 0;
      S->treasure[i].y = 0;
      S->treasure[i].type = 0;
   }
   for(int i=4;i<13;i++)
   {
      
      S->treasure[i].visible = 1;
      S->treasure[i].value = 1;
     
       while(1)
         {
            
            S->treasure[i].x = rand() % 20 + 2;
            
            S->treasure[i].y = rand() % 50 + 2;
            int fl=0;
            if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
            {
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure[i].x == S->treasure[v].x && S->treasure[i].y == S->treasure[v].y && S->treasure[v].visible ==1)
                  {
                        fl = 1;
                        break;
                  }
               }
               if(fl==0)
                  break;
            }
               
         }
     
      S->treasure[i].type = 1;
   }
   for(int i=13;i<18;i++)
   {
      S->treasure[i].visible = 1;
      S->treasure[i].value = 10;
      while(1)
         {
            
            S->treasure[i].x = rand() % 20 + 2;
            
            S->treasure[i].y = rand() % 50 + 2;
            int fl=0;
            if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
            {
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure[i].x == S->treasure[v].x && S->treasure[i].y == S->treasure[v].y && S->treasure[v].visible ==1)
                  {
                        fl = 1;
                        break;
                  }
               }
               if(fl==0)
                  break;
            }
               
         }
     
      S->treasure[i].type = 2;
   }
   for(int i=18;i<20;i++)
   {
      S->treasure[i].visible = 1;
      S->treasure[i].value = 50;
      while(1)
         {
            
            S->treasure[i].x = rand() % 20 + 2;
            
            S->treasure[i].y = rand() % 50 + 2;
            int fl=0;
            if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
            {
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure[i].x == S->treasure[v].x && S->treasure[i].y == S->treasure[v].y && S->treasure[v].visible ==1)
                  {
                        fl = 1;
                        break;
                  }
               }
               if(fl==0)
                  break;
            }
               
         }
     
      S->treasure[i].type = 3;
   }
}

int player_inf_init(struct player_info_t *P,struct player_t *P2,struct server_t *S)
{
   
   while(1)
   {
      P->x = rand() % 22 + 2;
      P->y = rand() % 49 + 2;
      if(S->map[P->x][P->y] == '0')
         break;
   }
   P->camp_x=S->campsite.x;
   P->camp_y=S->campsite.y;
   P->sx = P->x;
   P->sy = P->y;
   P->server_Pid = getppid();
   P->deaths = 0;
   P->coins_b = 0;
   P->coins_f = 0;
   P->type = P2->Type;
   P->p_number = P2->p_number;
   for(int i=0;i<5;i++)
   {
      for(int j=0;j<5;j++)
         P->map[i][j] = '1';
   }
   
   return 1;
}
void player_init(struct player_t *P)
{
   P->Pid = 0;
   P->asigned = 0;
   P->key = ' ';
   P->p_number = 0;
   P->Type = 0;
}
void clean()
{  
    sem_close(sem_get);
    sem_close(sem_got);
    sem_close(sem_p);
    sem_unlink(SEM_GET);
    sem_unlink(SEM_GOT);
    sem_unlink(SEM_P);
    
    if(player1->Pid > 0)   
      kill(player1->Pid,SIGKILL);
    if(player2->Pid > 0)   
      kill(player2->Pid,SIGKILL);
    if(player3->Pid > 0)   
      kill(player3->Pid,SIGKILL);   
    if(player4->Pid > 0)   
      kill(player4->Pid,SIGKILL);   
    
    room->player[0] = -1;
    room->player[1] = -1;
    room->player[2] = -1;
    room->player[3] = -1;
    munmap(room, sizeof(struct room));
    munmap(player1, sizeof(struct player_t));
    munmap(player2, sizeof(struct player_t));
    munmap(player3, sizeof(struct player_t));
    munmap(player4, sizeof(struct player_t));

    munmap(player_inf1, sizeof(struct player_t));
    munmap(player_inf2, sizeof(struct player_t));
    munmap(player_inf3, sizeof(struct player_t));
    munmap(player_inf4, sizeof(struct player_t));

    close(r);
    close(a);
    close(p1);
    close(p2);
    close(p3);
    close(p4);

    close(pi1);
    close(pi2);
    close(pi3);
    close(pi4);
    
    shm_unlink(SHM_ROOM);
    shm_unlink(SHM_PLAYER1);
    shm_unlink(SHM_PLAYER2);
    shm_unlink(SHM_PLAYER3);
    shm_unlink(SHM_PLAYER4);

    shm_unlink(SHM_PLAYER1_INF);
    shm_unlink(SHM_PLAYER2_INF);
    shm_unlink(SHM_PLAYER3_INF);
    shm_unlink(SHM_PLAYER4_INF);
    shm_unlink(SHM_SERV_ALIVE);
    
   
    endwin();

}
void handle_sigint(int sig) 
{ 
    pthread_cancel(th);
    pthread_cancel(th2);
    pthread_cancel(th3);
    clean();

    kill(pid,sig); 
    exit(0);
    
} 
int loadmap(int tab[27][53])
{
  FILE *fp;
  fp=fopen("map.txt","r");
  if(fp == NULL)
    return 1;
  for(int i=0;i<27;i++)
  {
      for(int j=0;j<53;j++)
      {
          tab[i][j] = fgetc(fp);
      }
      int a=fgetc(fp);
  }
  fflush(stdout);
  return 0;
}

void printch(int x,int y,char ch,int color)
{
   attron(COLOR_PAIR(color));
   mvaddch(x,y,ch);
   attroff(COLOR_PAIR(color));
}
void prints(int x,int y,char *s,int color)
{
   attron(COLOR_PAIR(color));
   mvprintw(x,y,"%s",s);
   attroff(COLOR_PAIR(color));
}
void printd(int x,int y,int d,int color)
{
  
   attron(COLOR_PAIR(color));
   mvprintw(x,y,"%d",d);
   attroff(COLOR_PAIR(color));
}
void print_statistic_serv(struct server_t *P)
{

   mvprintw(1,58,"Server's PID: %d",P->server_Pid);
   mvprintw(2,58,"Campsite X/Y: %2d/%2d",P->campsite.x,P->campsite.y);
   mvprintw(3,58,"Round number: %d",P->round);

}
void print_const_text()
{
   mvprintw(5,58,"Parameter:");
   mvprintw(6,58,"PID");
   mvprintw(7,58,"Type");
   mvprintw(8,58,"Curr X/Y");
   mvprintw(9,58,"Deaths");
   mvprintw(11,58,"Coins");
   mvprintw(12,58,"carried");
   mvprintw(13,58,"brought");
   mvprintw(16,58,"Legend:");
   
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

void move_beasts(struct server_t *S)
{
    for(int i=0;i<10;i++)
    {
        int flag = 0;
        if(S->beast[i].visible == 1)
        {
            for(int j=0;j<4;j++)
            {
                if(S->player_info[j]->p_number > 0)
                {
                    // down 2 || down 1
                    if((S->beast[i].x - 2 == S->player_info[j]->x && S->beast[i].y == S->player_info[j]->y) || (S->beast[i].x - 1 == S->player_info[j]->x && S->beast[i].y == S->player_info[j]->y))
                    {
                        if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                        else if(S->map[S->beast[i].x][S->beast[i].y - 1] != '1')
                            S->beast[i].y--;
                        else if(S->map[S->beast[i].x][S->beast[i].y + 1] != '1')
                            S->beast[i].y++;
                            
                        flag = 1;
                    }
                    
                    
                    
                    //up 2 || up 1
                    else if((S->beast[i].x + 2 == S->player_info[j]->x && S->beast[i].y == S->player_info[j]->y) || (S->beast[i].x + 1 == S->player_info[j]->x && S->beast[i].y == S->player_info[j]->y))
                    {
                        if(S->map[S->beast[i].x + 1][S->beast[i].y] != '1')
                            S->beast[i].x++;
                        else if(S->map[S->beast[i].x][S->beast[i].y - 1] != '1')
                            S->beast[i].y--;
                        else if(S->map[S->beast[i].x][S->beast[i].y + 1] != '1')
                            S->beast[i].y++;
                        
                        flag = 1;
                    }
                   
                   
                    
                    //left 2 || left 1
                    else if((S->beast[i].x  == S->player_info[j]->x && S->beast[i].y - 2 == S->player_info[j]->y) || (S->beast[i].x == S->player_info[j]->x && S->beast[i].y - 1 == S->player_info[j]->y))
                    {
                        if(S->map[S->beast[i].x][S->beast[i].y - 1] != '1')
                            S->beast[i].y--;
                        else if(S->map[S->beast[i].x + 1][S->beast[i].y] != '1')
                            S->beast[i].x++;
                        else if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                    
                        flag = 1;
                    }
                
                    
                    
                    //right 2 || right 1
                    else if((S->beast[i].x  == S->player_info[j]->x && S->beast[i].y + 2 == S->player_info[j]->y) || (S->beast[i].x == S->player_info[j]->x && S->beast[i].y + 1 == S->player_info[j]->y))
                    {
                        if(S->map[S->beast[i].x][S->beast[i].y + 1] != '1')
                            S->beast[i].y++;
                        else if(S->map[S->beast[i].x + 1][S->beast[i].y] != '1')
                            S->beast[i].x++;
                        else if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                        
                        flag = 1;
                    }
                    
                    //right 1 down 1
                    else if(S->beast[i].x + 1 == S->player_info[j]->x && S->beast[i].y + 1 == S->player_info[j]->y)
                    {
                        if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                        else if(S->map[S->beast[i].x][S->beast[i].y - 1] != '1')
                            S->beast[i].y--;
                        flag = 1;
                    }
                    //left 1 down 1
                    else if(S->beast[i].x + 1 == S->player_info[j]->x && S->beast[i].y - 1 == S->player_info[j]->y)
                    {
                        if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                        else if(S->map[S->beast[i].x][S->beast[i].y + 1] != '1')
                            S->beast[i].y++;
                        flag = 1;
                    }
                    //right1 up1
                    else if(S->beast[i].x - 1 == S->player_info[j]->x && S->beast[i].y + 1 == S->player_info[j]->y)
                    {
                        if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                        else if(S->map[S->beast[i].x][S->beast[i].y - 1] != '1')
                            S->beast[i].y--;
                        flag = 1;
                    }
                    //left 1 donw 1
                    else if(S->beast[i].x - 1 == S->player_info[j]->x && S->beast[i].y - 1 == S->player_info[j]->y)
                    {
                        if(S->map[S->beast[i].x - 1][S->beast[i].y] != '1')
                            S->beast[i].x--;
                        else if(S->map[S->beast[i].x][S->beast[i].y + 1] != '1')
                            S->beast[i].y++;
                        flag = 1;
                    }
                  
                }
                if(flag == 1)
                    break;
            }
        }
    }
}
void *display_real(void*arg)
{
   struct server_t * P = (struct server_t *)arg;
   sem_wait(sem_p);
   initscr();
   noecho();
   start_color();
   init_pair(1, COLOR_BLACK, COLOR_WHITE);
   init_pair(2, COLOR_CYAN, COLOR_BLUE);
   init_pair(3, COLOR_BLACK, COLOR_GREEN);
   init_pair(4, COLOR_BLACK, COLOR_YELLOW);
   init_pair(5,COLOR_YELLOW,COLOR_GREEN);
   init_pair(6,COLOR_RED,COLOR_WHITE);
   sem_post(sem_p);
   while(1)
   {
      sem_wait(sem_p);
      
      for(int i=0;i<27;i++)
      {
         for(int j=0;j<53;j++)
         {
           
            if(P->map[i][j] == '0')
            {
               printch(i,j,' ',1);
            }
            else if(P->map[i][j] == '1')
            {
               printch(i,j,' ',2);
            } 
            else if(P->map[i][j] == '2')
            {
               printch(i,j,'#',1);
            }
               
         }  
      }
     
      print_statistic_serv(P);

      for(int i = 0;i<4;i++)
      {
         print_const_text();
        
         mvprintw(5,69+i*9,"PLAYER%d",i+1);
         
         if(P->player[i]->Pid != 0)
         {  
            printd(P->player_info[i]->x,P->player_info[i]->y,P->player[i]->p_number,3);
            mvprintw(6,69+i*9,"%d",P->player[i]->Pid);

            move(7,69+i*9);
            if(P->player[i]->Type == 0)
               printw("%5s","HUMAN");
            else
               printw("%5s","CPU");
                  
            mvprintw(8,69+i*9,"%2d/%2d",P->player_info[i]->x,P->player_info[i]->y);
            mvprintw(9,69+i*9,"%5d",P->player_info[i]->deaths);   
            mvprintw(12,69+i*9,"%5d",P->player_info[i]->coins_f);  
            mvprintw(13,69+i*9,"%5d",P->player_info[i]->coins_b);      
         }
         else
         {
            mvprintw(6,69+i*9,"%5s","-");
            mvprintw(7,69+i*9,"%5s","-");

            mvprintw(8,69+i*9,"%5s","--/--");
            mvprintw(9,69+i*9,"%5s","-");
            mvprintw(12,69+i*9,"%5d",0);  
            mvprintw(13,69+i*9,"%5d",0);    
           
         }
         
      }
      printch(P->campsite.x,P->campsite.y,'A',5);
      print_legend();
      for(int i=0;i<20;i++)
      {
         if(P->treasure[i].visible == 1)
         {
            switch(P->treasure[i].type)
            {
               case 0:
                  printch(P->treasure[i].x,P->treasure[i].y,'D',4);
                  break;
               case 1:
                  printch(P->treasure[i].x,P->treasure[i].y,'c',4); 
                  break; 
               case 2:
                  printch(P->treasure[i].x,P->treasure[i].y,'t',4);
                  break;
               case 3:
                  printch(P->treasure[i].x,P->treasure[i].y,'T',4);
                  break;
            }
         }
         if(P->treasure_add[i].visible == 1)
         {
            switch(P->treasure_add[i].type)
            {
               case 1:
                  printch(P->treasure_add[i].x,P->treasure_add[i].y,'c',4); 
                  break; 
               case 2:
                  printch(P->treasure_add[i].x,P->treasure_add[i].y,'t',4);
                  break;
               case 3:
                  printch(P->treasure_add[i].x,P->treasure_add[i].y,'T',4);
                  break;
            }
         }
         
      }
      for(int i=0;i<10;i++)
      {
         if(P->beast[i].visible)
         {
            printch(P->beast[i].x,P->beast[i].y,'*',6);
         }
      }
    
      refresh();
      sem_post(sem_p);
      
      
      usleep(15000);
   }
    

   return NULL;
}

int player_player_colision(struct player_info_t * P1,struct player_info_t * P2,int pos) // pos 0 - up , 1 - down , 2 - left , 3 - right
{
   switch(pos)
   {
      case 0:
         if(P1->x == P2->x - 1  && P1->y == P2->y)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P2->x = P2->sx;
            P2->y = P2->sy;
            P1->deaths++;
            P2->deaths++;
            return 1;                              
         }
         else
            return 0;
      case 1:
         if(P1->x == P2->x + 1  && P1->y == P2->y)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P2->x = P2->sx;
            P2->y = P2->sy;
            P1->deaths++;
            P2->deaths++;
            return 1;                              
         }
         else
            return 0; 
      case 2:
         if(P1->x == P2->x  && P1->y == P2->y - 1)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P2->x = P2->sx;
            P2->y = P2->sy;
            P1->deaths++;
            P2->deaths++;
            return 1;                              
         }
         else
            return 0;   
      case 3:
         if(P1->x == P2->x  && P1->y == P2->y + 1)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P2->x = P2->sx;
            P2->y = P2->sy;
            P1->deaths++;
            P2->deaths++;
            return 1;                              
         }
         else
            return 0;        
   }
    return 0;
   
}
int player_beast_colision(struct player_info_t * P1,struct beast_t B[10],int pos) // pos 0 - up , 1 - down , 2 - left , 3 - right,4 - beast move to your pos
{
   for(int i=0;i<10;i++)
   {
      if(B[i].visible == 0)
         continue;
      switch(pos)
      {
      case 0:
         if(B[i].x == P1->x -1  && P1->y == B[i].y)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P1->deaths++;
            return 1;                              
         }
         break;
           
      case 1:
         if(B[i].x == P1->x+1  && P1->y == B[i].y)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy; 
            P1->deaths++;
           
            return 1;                              
         }
         break;
      case 2:
         if(P1->x == B[i].x && B[i].y == P1->y - 1)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P1->deaths++;
           
            return 1;                              
         }
         break;   
      case 3:
         if(P1->x == B[i].x  && B[i].y == P1->y + 1)
         {  
                        
            P1->x = P1->sx;
            P1->y = P1->sy;
            P1->deaths++;
            return 1;                              
         }
         break;
      case 4:
        if(P1->x == B[i].x  && B[i].y == P1->y)
        {
                  
            P1->x = P1->sx;
            P1->y = P1->sy;
            P1->deaths++;
            return 1;
        }
        break;
              
      }
   }
   
    return 0;
   
}
void *keyboard(void*arg)
{
   
   struct server_t *S = (struct server_t*)arg;
   sem_wait(sem_p);
   WINDOW *w;
   w = initscr();
   sem_post(sem_p);
   //raw();
  
   while(1)
   {
      fflush(stdout);
      
      char c = getch();
      
      if(c == 'q' || c == 'Q')
      {
          pthread_cancel(th);
          pthread_cancel(th2);
          clean();
          exit(0);
      }
      if(c == 'c')
      {
            sem_wait(sem_p);
           
            int i=0;
            for(int k=0;k<20;k++)
            {
               if(S->treasure_add[k].visible == 0)
               {
                  i=k;
                   break;
               }
            }
            if(i==19)
               i=0;
              
            S->treasure_add[i].visible = 1;
            S->treasure_add[i].value = 1;
            while(1)
           {
            
            S->treasure_add[i].x = rand() % 20 + 2;
            
            S->treasure_add[i].y = rand() % 50 + 2;
            int fl=0;
            if(S->map[S->treasure_add[i].x][S->treasure_add[i].y] == '0')
            {
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure_add[i].x == S->treasure_add[v].x && S->treasure_add[i].y == S->treasure_add[v].y && S->treasure_add[v].visible ==1)
                  {
                        fl = 1;
                        break;
                  }
               }
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure_add[i].x == S->treasure[v].x && S->treasure_add[i].y == S->treasure[v].y )
                  {
                        fl = 1;
                        break;
                  }
               }
               if(fl==0)
                  break;
            }
               
         }
            
            S->treasure_add[i].type = 1;
            sem_post(sem_p);
      }
      if(c == 't')
      {
         sem_wait(sem_p);
          
         int i=0;
         for(i=0;i<20;i++)
         {
            if(S->treasure_add[i].visible == 0)
               break;
         }
         if(i==19)
            i=0;
         S->treasure_add[i].visible = 1;
         S->treasure_add[i].value = 10;
         while(1)
         {
            
            S->treasure_add[i].x = rand() % 20 + 2;
            
            S->treasure_add[i].y = rand() % 50 + 2;
            int fl=0;
            if(S->map[S->treasure_add[i].x][S->treasure_add[i].y] == '0')
            {
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure_add[i].x == S->treasure_add[v].x && S->treasure_add[i].y == S->treasure_add[v].y && S->treasure_add[v].visible ==1)
                  {
                        fl = 1;
                        break;
                  }
               }
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure_add[i].x == S->treasure[v].x && S->treasure_add[i].y == S->treasure[v].y )
                  {
                        fl = 1;
                        break;
                  }
               }
               if(fl==0)
                  break;
            }
               
         }
     
         S->treasure_add[i].type = 2;  
         sem_post(sem_p); 

      }
      if(c == 'T')
      {
         sem_wait(sem_p);
          
         int i=0;
         for(i=0;i<20;i++)
         {
            if(S->treasure_add[i].visible == 0)
               break;
         }
         if(i==19)
            i=0;
         S->treasure_add[i].visible = 1;
         S->treasure_add[i].value = 50;
         while(1)
         {
            
            S->treasure_add[i].x = rand() % 20 + 2;
            
            S->treasure_add[i].y = rand() % 50 + 2;
            int fl=0;
            if(S->map[S->treasure_add[i].x][S->treasure_add[i].y] == '0')
            {
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure_add[i].x == S->treasure_add[v].x && S->treasure_add[i].y == S->treasure_add[v].y && S->treasure_add[v].visible ==1)
                  {
                        fl = 1;
                        break;
                  }
               }
               for(int v=0;v<20;v++)
               {
                  if(i == v)
                     continue;
                  if(S->treasure_add[i].x == S->treasure[v].x && S->treasure_add[i].y == S->treasure[v].y )
                  {
                        fl = 1;
                        break;
                  }
               }
               if(fl==0)
                  break;
            }
               
         }
         S->treasure_add[i].type = 3;  
          sem_post(sem_p);  
      }
      if(c == 'b' || c == 'B')
      {
          sem_wait(sem_p);
         add_beast(S);
          sem_post(sem_p);
      }
   }
    return NULL;
   
    
}
void treasure_colision_add(struct server_t *S,struct player_info_t *P1,int pos)
{
   
   switch(pos)
   {
      case 0:
         for(int i=0;i<20;i++)
         {
            if(P1->x - 1 == S->treasure_add[i].x  && P1->y == S->treasure_add[i].y)
            {      
               P1->coins_f += S->treasure_add[i].value;
               S->treasure_add[i].visible = 0; 
               S->treasure_add[i].value = 0;                        
            }
         }
         break;
      case 1:
        for(int i=0;i<20;i++)
         {
            if(P1->x + 1 == S->treasure_add[i].x  && P1->y == S->treasure_add[i].y)
            {  
                        
               P1->coins_f += S->treasure_add[i].value;
               S->treasure_add[i].visible = 0;  
               S->treasure_add[i].value = 0; 
               break;   
            }
         }
         break;
      case 2:
 refresh();    case 3:
         for(int i=0;i<20;i++)
         {
            if(P1->x == S->treasure_add[i].x  && P1->y + 1 == S->treasure_add[i].y)
            {  
                        
               P1->coins_f += S->treasure_add[i].value;
               S->treasure_add[i].visible = 0;  
               S->treasure_add[i].value = 0; 
               break;                             
            }
           
         }
         break;     
   }
}

void treasure_colision(struct server_t *S,struct player_info_t *P1,int pos)
{
   
   switch(pos)
   {
      case 0:
         for(int i=0;i<20;i++)
         {
            if(P1->x - 1 == S->treasure[i].x  && P1->y == S->treasure[i].y)
            {  
                        
               P1->coins_f += S->treasure[i].value;
               if(S->treasure[i].type == 0)
               {
                  S->treasure[i].visible = 0;
                  S->treasure[i].value = 0;
                  break;
               }
               else
               {
                  while(1)
                  {
                     S->treasure[i].x =rand() % 20 +2;
                     S->treasure[i].y = rand() %50 +2;   
                     if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
                        break;
                   }   
               }
               break;                         
            }
            
         }
         break;
      case 1:
        for(int i=0;i<20;i++)
         {
            if(P1->x + 1 == S->treasure[i].x  && P1->y == S->treasure[i].y)
            {  
                        
               P1->coins_f += S->treasure[i].value;
               if(S->treasure[i].type == 0)
               {
                  S->treasure[i].visible = 0;
                  S->treasure[i].value = 0;
                  break;
               }
               else
               {
                  while(1)
                  {
                     S->treasure[i].x =rand() % 20 +2;
                     S->treasure[i].y = rand() %50 +2;   
                     if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
                        break;
                   }   
               }
               break;   
            }
         }
         break;
      case 2:
        for(int i=0;i<20;i++)
         {
            if(P1->x == S->treasure[i].x  && P1->y - 1 == S->treasure[i].y)
            {  
                        
               P1->coins_f += S->treasure[i].value;
               if(S->treasure[i].type == 0)
               {
                  S->treasure[i].visible = 0;
                  S->treasure[i].value = 0;
                  break;
               }
               else
               {
                  while(1)
                  {
                     S->treasure[i].x =rand() % 20 +2;
                     S->treasure[i].y = rand() %50 +2;   
                     if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
                        break;
                   }   
               }
               break;                             
            }
            
         }
         break;   
      case 3:
         for(int i=0;i<20;i++)
         {
            if(P1->x == S->treasure[i].x  && P1->y + 1 == S->treasure[i].y)
            {  
                        
               P1->coins_f += S->treasure[i].value;
               if(S->treasure[i].type == 0)
               {
                  S->treasure[i].visible = 0;
                  S->treasure[i].value = 0;
                  break;
               }
               else
               {
                  while(1)
                  {
                     S->treasure[i].x =rand() % 20 +2;
                     S->treasure[i].y = rand() %50 +2;   
                     if(S->map[S->treasure[i].x][S->treasure[i].y] == '0')
                        break;
                 }   
               }
               break;                             
            }
           
         }
         break;     
   }
}

void *mov(void* arg)
{
  
   struct server_t * S = (struct server_t *)arg;
   int bush[4]={0};
   int flag,flag2,tempx,tempy;
   
   while(1)
   {
      sem_wait(sem_p);
      move_beasts(S);
      for(int i=0;i<4;i++)
      {
         //If player leave I clear his pos
         if(S->player[i]->Pid == 0)
         {
            S->player_info[i]->x = 0;
            S->player_info[i]->y = 0;
         }
         if(S->player[i]->key != ' ')
         {  
            flag = 0;
            flag2=0;
            switch(S->player[i]->key)
            {
               case 'w':

                  //Player colison
                  for(int j = 0;j<4;j++)
                  {
                     if(i == j)
                        continue;
                     tempx = S->player_info[i]->x;
                     tempy = S->player_info[i]->y;     
                     flag = player_player_colision(S->player_info[j],S->player_info[i],0);    
                     if(flag == 1)
                     {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f + S->player_info[j]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        S->player_info[j]->coins_f = 0;
                        break;
                     }
                  }
                  if(flag == 1)
                     break;
                  //Beast collision
                  tempx = S->player_info[i]->x;
                  tempy = S->player_info[i]->y;   
                  flag2 = player_beast_colision(S->player_info[i],S->beast,0) || player_beast_colision(S->player_info[i],S->beast,4);
                  if(flag2 == 1)
                  {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        break;
                  }
                  //Bush colision
                  if(S->map[S->player_info[i]->x - 1][S->player_info[i]->y] == '2')
                  {
                     if(bush[i] == 0)
                     {
                        bush[i]++;
                         S->player[i]->key = ' ';
                        break;
                     }
                     else
                        bush[i] = 0; 
                  }
                  else if(S->map[S->player_info[i]->x - 1][S->player_info[i]->y] != '0')
                     break;
                  //treasure colison
                  treasure_colision(S,S->player_info[i],0);
                  //treasure colision_add
                  treasure_colision_add(S,S->player_info[i],0);
                  //campsite colision
                  if(S->player_info[i]->x == S->campsite.x && S->player_info[i]->y == S->campsite.y)
                  {
                     S->player_info[i]->coins_b += S->player_info[i]->coins_f;
                     S->player_info[i]->coins_f = 0;
                  }
                  //move
                  S->player_info[i]->x--;
                  S->player[i]->key = ' ';
                  //move beasts
                
                  break;
               case 'a':

                  //player colison
                  for(int j = 0;j<4;j++)
                  {
                     if(i == j)
                        continue;
                     tempx = S->player_info[i]->x;
                     tempy = S->player_info[i]->y;  
                     flag = player_player_colision(S->player_info[j],S->player_info[i],2);

                     if(flag == 1)
                     {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f + S->player_info[j]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        S->player_info[j]->coins_f = 0;
                        break;
                     }
                        
                  }
                  if(flag == 1)
                     break;
                   //Beast collision
                  tempx = S->player_info[i]->x;
                  tempy = S->player_info[i]->y;   
                  flag2 = player_beast_colision(S->player_info[i],S->beast,2) || player_beast_colision(S->player_info[i],S->beast,4);
                  if(flag2 == 1)
                  {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        break;
                  }
                  //bush colision
                   if(S->map[S->player_info[i]->x][S->player_info[i]->y - 1] == '2')
                  {
                     if(bush[i] == 0)
                     {
                        bush[i]++;
                         S->player[i]->key = ' ';
                        break;
                     }
                     else
                        bush[i] = 0; 
                  }
                  else if(S->map[S->player_info[i]->x][S->player_info[i]->y - 1] != '0')
                     break;
                  //treasure colision
                  treasure_colision(S,S->player_info[i],2);
                  //treasure colision_add
                  treasure_colision_add(S,S->player_info[i],2);
                  //campsite colision
                  if(S->player_info[i]->x == S->campsite.x && S->player_info[i]->y == S->campsite.y)
                  {
                     S->player_info[i]->camp_x = S->campsite.x;
                     S->player_info[i]->camp_y = S->campsite.y;
                     S->player_info[i]->coins_b += S->player_info[i]->coins_f;
                     S->player_info[i]->coins_f = 0;
                  }
                  //move
                  S->player_info[i]->y--;
                  S->player[i]->key = ' ';  
                  break;
               case 's':
                   //Player colison
                  for(int j = 0;j<4;j++)
                  {
                     if(i == j)
                        continue;
                     tempx = S->player_info[i]->x;
                     tempy = S->player_info[i]->y;    
                     flag = player_player_colision(S->player_info[j],S->player_info[i],1);    
                     if(flag == 1)
                     {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f + S->player_info[j]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        S->player_info[j]->coins_f = 0;
                        break;
                     }
                  }
                  if(flag == 1)
                     break;
                      //Beast collision
                  tempx = S->player_info[i]->x;
                  tempy = S->player_info[i]->y;   
                  flag2 = player_beast_colision(S->player_info[i],S->beast,1) || player_beast_colision(S->player_info[i],S->beast,4);
                  if(flag2 == 1)
                  {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        break;
                  }
                  //bush colision
                   if(S->map[S->player_info[i]->x + 1][S->player_info[i]->y] == '2')
                  {
                     if(bush[i] == 0)
                     {
                        bush[i]++;
                         S->player[i]->key = ' ';
                        break;
                     }
                     else
                        bush[i] = 0; 
                  }
                  else if(S->map[S->player_info[i]->x + 1][S->player_info[i]->y] != '0')
                     break;
                  //treasure colision   
                  treasure_colision(S,S->player_info[i],1);
                  //treasure colision_add
                  treasure_colision_add(S,S->player_info[i],1);
                   //campsite colision
                  if(S->player_info[i]->x == S->campsite.x && S->player_info[i]->y == S->campsite.y)
                  {
                     S->player_info[i]->camp_x = S->campsite.x;
                     S->player_info[i]->camp_y = S->campsite.y;
                     S->player_info[i]->coins_b += S->player_info[i]->coins_f;
                     S->player_info[i]->coins_f = 0;
                  }
                  //move
                  S->player_info[i]->x++;
                  S->player[i]->key = ' '; 
                  break;
               case 'd':
                  //player colision
                  for(int j = 0;j<4;j++)
                  {
                     if(i == j)
                        continue;
                     tempx = S->player_info[i]->x;
                     tempy = S->player_info[i]->y;  
                     flag = player_player_colision(S->player_info[j],S->player_info[i],3);    
                     if(flag == 1)
                     {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f + S->player_info[j]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        S->player_info[j]->coins_f = 0;
                        break;
                     }
                  }
                  if(flag == 1)
                     break;
                   //Beast collision
                  tempx = S->player_info[i]->x;
                  tempy = S->player_info[i]->y;   
                  flag2 = player_beast_colision(S->player_info[i],S->beast,3) || player_beast_colision(S->player_info[i],S->beast,4);
                  if(flag2 == 1)
                  {
                        S->treasure[i].x = tempx;
                        S->treasure[i].y = tempy;
                        S->treasure[i].visible = 1;
                        S->treasure[i].value = S->player_info[i]->coins_f;
                        S->player_info[i]->coins_f = 0;
                        break;
                  }
                  //bush colision 
                   if(S->map[S->player_info[i]->x][S->player_info[i]->y + 1] == '2')
                  {
                     if(bush[i] == 0)
                     {
                        bush[i]++;
                         S->player[i]->key = ' ';
                        break;
                     }
                     else
                        bush[i] = 0; 
                  }
                  else if(S->map[S->player_info[i]->x][S->player_info[i]->y + 1] != '0')
                     break;

                  //treasure colision
                  treasure_colision(S,S->player_info[i],3);
                  //treasure colision_add
                  treasure_colision_add(S,S->player_info[i],3);
                   //campsite colision
                  if(S->player_info[i]->x == S->campsite.x && S->player_info[i]->y == S->campsite.y)
                  {
                     S->player_info[i]->camp_x = S->campsite.x;
                     S->player_info[i]->camp_y = S->campsite.y;
                     S->player_info[i]->coins_b += S->player_info[i]->coins_f;
                     S->player_info[i]->coins_f = 0;
                  }

                  //move
                  S->player_info[i]->y++;
                  S->player[i]->key = ' ';   
                  break;
            }
            
            for(int k = 0;k < 5;k++)
            {
               for(int l = 0;l < 5;l++)
               {
                  //print player 
                  if(k == 2 && l == 2)
                  {
                     S->player_info[i]->map[k][l] = S->player_info[i]->p_number + '0' + 2;
                     continue;
                  }
                  //print players
                   int  tem = 0;
                  for(int n=0;n<4;n++)
                  {
                     if(n == i)
                        continue;
                     if(S->player_info[n]->x == S->player_info[i]->x + k -2 && S->player_info[n]->y == S->player_info[i]->y -2 +l) 
                     {
                        S->player_info[i]->map[k][l] = '0' + S->player_info[n]->p_number + 2; 
                        tem = 1;
                        break; 
                     }
                         
                  }   
                  if(tem == 1)
                     continue;


                  //print tresure 
                 
                  
                  for(int n = 0;n < 20;n++)
                  {
                     if(S->treasure[n].x == S->player_info[i]->x + k -2 && S->treasure[n].y == S->player_info[i]->y -2 +l)
                     {
                        if(S->treasure[n].visible == 0)
                            continue;
                        S->player_info[i]->map[k][l] = S->treasure[n].type + '0' + 7;
                        tem = 1;
                        break;
                     }
                        
                  }   
                   if(tem == 1)
                     continue;
                  //beast   
                  for(int n = 0;n < 10;n++)
                  {
                     if(S->beast[n].x == S->player_info[i]->x + k -2 && S->beast[n].y == S->player_info[i]->y -2 +l)
                     {
                        if(S->beast[n].visible == 0)
                            continue;
                        S->player_info[i]->map[k][l] = '0' + 12;
                        tem = 1;
                        break;
                     }
                        
                  }  
                  if(tem == 1)
                     continue;
                   // print treasures add
                   for(int n = 0;n < 20;n++)
                  {
                     if(S->treasure_add[n].x == S->player_info[i]->x + k -2 && S->treasure_add[n].y == S->player_info[i]->y -2 +l)
                     {
                        if(S->treasure_add[n].visible == 0)
                            continue;
                        S->player_info[i]->map[k][l] = S->treasure_add[n].type + '0' + 7;
                        tem = 1;
                        break;
                     }
                        
                  }   
                  if(tem == 1)
                     continue;  

                  //print campsite 
                  if(S->campsite.x == S->player_info[i]->x + k - 2 && S->campsite.y == S->player_info[i]->y + l -2)
                  {  
                      S->player_info[i]->map[k][l] = '0' + 11;
                      continue;
                  }
                  //print map   
                  S->player_info[i]->map[k][l] = S->map[S->player_info[i]->x-2+k][S->player_info[i]->y-2+l];
               }
            }
         }
         S->player_info[i]->round=S->round + 1;
      }
       
      S->round++;
      sem_post(sem_p);
      
      usleep(500000);
   }
   

   return NULL;

};
 
int main(int argc, char **argv)
{
   
    pid = getppid();
    signal(SIGINT, handle_sigint);
    signal(SIGABRT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    system("printf '\033[8;40;120t'");
    srand(time(NULL));

    r = shm_open(SHM_ROOM,O_CREAT | O_RDWR, 0666);
    err(r == -1, "shm_open");
    ftruncate(r, sizeof(struct room));
    room=(struct room*)mmap(NULL, sizeof(struct room), PROT_READ | PROT_WRITE,MAP_SHARED, r, 0);
    err(room == NULL , "mmap");
    room->player[0] = 0;
    room->player[1] = 0;
    room->player[2] = 0;
    room->player[3] = 0;
    room->full = 0;

    sem_get = sem_open(SEM_GET,O_CREAT ,0600,0);
    sem_got = sem_open(SEM_GOT,O_CREAT ,0600,1);
    

    p1 = shm_open(SHM_PLAYER1,O_CREAT | O_RDWR, 0666);
    err(p1 == -1, "shm_open");
    ftruncate(p1, sizeof(struct player_t));
    player1=(struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE,MAP_SHARED, p1, 0);
    err(player1 == NULL , "mmap");


     p2 = shm_open(SHM_PLAYER2,O_CREAT | O_RDWR, 0666);
    err(p2 == -1, "shm_open");
    ftruncate(p2, sizeof(struct player_t));
    player2=(struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE,MAP_SHARED, p2, 0);
    err(player2 == NULL , "mmap");
    
    p3 = shm_open(SHM_PLAYER3,O_CREAT | O_RDWR, 0666);
    err(p3 == -1, "shm_open");
    ftruncate(p3, sizeof(struct player_t));
    player3=(struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE,MAP_SHARED, p3, 0);
    err(player3 == NULL , "mmap");
    

    p4 = shm_open(SHM_PLAYER4,O_CREAT | O_RDWR, 0666);
    err(p4 == -1, "shm_open");
    ftruncate(p4, sizeof(struct player_t));
    player4=(struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE,MAP_SHARED, p4, 0);
    err(player4 == NULL , "mmap");

    player_init(player1);
    player_init(player2);
    player_init(player3);
    player_init(player4);
    
    pi1 = shm_open(SHM_PLAYER1_INF,O_CREAT | O_RDWR, 0666);
    err(pi1 == -1, "shm_open");
    ftruncate(pi1, sizeof(struct player_info_t ));
    player_inf1=(struct player_info_t *)mmap(NULL, sizeof(struct player_info_t ), PROT_READ | PROT_WRITE,MAP_SHARED, pi1, 0);
    err(player_inf1 == NULL , "mmap");
    
     pi2 = shm_open(SHM_PLAYER2_INF,O_CREAT | O_RDWR, 0666);
    err(pi2 == -1, "shm_open");
    ftruncate(pi2, sizeof(struct player_info_t ));
    player_inf2=(struct player_info_t *)mmap(NULL, sizeof(struct player_info_t ), PROT_READ | PROT_WRITE,MAP_SHARED, pi2, 0);
    err(player_inf2 == NULL , "mmap");


     pi3 = shm_open(SHM_PLAYER3_INF,O_CREAT | O_RDWR, 0666);
    err(pi3 == -1, "shm_open");
    ftruncate(pi3, sizeof(struct player_info_t ));
    player_inf3=(struct player_info_t *)mmap(NULL, sizeof(struct player_info_t ), PROT_READ | PROT_WRITE,MAP_SHARED, pi3, 0);
    err(player_inf3 == NULL , "mmap");
    

     pi4 = shm_open(SHM_PLAYER4_INF,O_CREAT | O_RDWR, 0666);
    err(pi4 == -1, "shm_open");
    ftruncate(pi4, sizeof(struct player_info_t ));
    player_inf4=(struct player_info_t *)mmap(NULL, sizeof(struct player_info_t ), PROT_READ | PROT_WRITE,MAP_SHARED, pi4, 0);
    err(player_inf4 == NULL , "mmap");
    

    struct server_t S;
    loadmap(S.map);
    S.player[0] = player1;
    S.player[1] = player2;
    S.player[2] = player3;
    S.player[3] = player4;  

    S.player_info[0] = player_inf1;
    S.player_info[1] = player_inf2;
    S.player_info[2] = player_inf3;
    S.player_info[3] = player_inf4;

    S.server_Pid = getppid();
    S.round = 0;
    init_tresures(&S);
    init_tresures_add(&S);
    init_beast(&S);
    
    while(1)
    {
      
       S.campsite.x = rand() % 24 + 2;
       S.campsite.y = rand() % 50 + 2;
       if(S.map[S.campsite.x][S.campsite.y] == '0')
         break;
    }
    
    sem_p = sem_open(SEM_P,O_CREAT ,0600,1); 
    //Daje znak że serwer istnieje
    a = shm_open(SHM_SERV_ALIVE,O_CREAT | O_RDWR, 0666);
    err(a == -1, "shm_open");
    ftruncate(pi4, sizeof(int));
    serv_alive=(int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,MAP_SHARED,a, 0);
    err(serv_alive == NULL , "mmap");
    

     
     usleep(100000);
     pthread_create(&th,NULL,display_real,&S);
     usleep(100000);
     pthread_create(&th2,NULL,mov,&S);
     usleep(100000);
    
     pthread_create(&th3,NULL,keyboard,&S);
     while(1)
     {   
       
         //Jeśli jest 4 informuje o tym kolejnych klientow
         if(player1->Pid && player2->Pid && player3->Pid && player4->Pid)
         {
            room->full = 1;
         }
     
        
        sem_wait(sem_get);//Czekam aż kilent wybierze wolne miejsce w pokoju

        sem_wait(sem_got);//Czekam na dostęp do danych
        //Inicjalizuje nowego kilenta
        if(player1->asigned == 1)
        {
           int x = player_inf_init(player_inf1,player1,&S);
           player1->asigned = 0;
           sem_post(sem_got);
           continue;
        }
         else if(player2->asigned == 1)
        {
           player_inf_init(player_inf2,player2,&S);
           player2->asigned = 0;
           sem_post(sem_got);
           continue;
        }
         else if(player3->asigned == 1)
        {
           player_inf_init(player_inf3,player3,&S);
           player3->asigned = 0;
           sem_post(sem_got);
           continue;
        }
         else if(player4->asigned == 1)
        {
           player_inf_init(player_inf4,player4,&S);
           player4->asigned = 0;
           sem_post(sem_got);
           continue;
        }
    
     }
     pthread_join(th3,NULL);
     pthread_join(th,NULL);
     pthread_join(th2,NULL);
    
}

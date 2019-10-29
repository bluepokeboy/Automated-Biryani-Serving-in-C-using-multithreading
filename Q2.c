#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include <sys/shm.h>

int m,n,k;
int min(int num1, int num2){
    return (num1 > num2 ) ? num2 : num1;
}
int studentsremain;
struct robot_chef{
     int index;
     int preptime;
     int num_vessels;
     int capacity;
} * chefs;

struct serving_table{
     int index;
     int serving_container;
     int slots;
} * tables;

struct students{
     int index;
     int status;
     int entry_time;
} * students;

pthread_mutex_t *mutex_chefs,*mutex_tables,mutex_waiting_students;
int students_waiting=0;

void biryani_ready(int index){
     int lolflag=0;
     while(chefs[index].num_vessels){
          for(int j=0; j<n; j++){
               if(pthread_mutex_trylock(&mutex_tables[j])){
                    continue;
               }
               if(tables[j].serving_container==0){
                    printf("Robot Chef %d is refilling Serving Container of Serving Table %d\n",chefs[index].index,tables[j].index);
                    tables[j].serving_container=chefs[index].capacity;
                    chefs[index].num_vessels-=1;
                    printf("Serving Container of Table %d is refilled by Robot Chef %d; Table %d resuming serving now\n",tables[j].index,chefs[index].index,tables[j].index);
                    //containers--;
                    //printf("Chef %d served table %d with capacity %d\n",chefs[index].index,tables[j].index,chefs[index].capacity);
                    fflush(stdout);
               }
               pthread_mutex_unlock(&mutex_tables[j]);
               if(!chefs[index].num_vessels){
                    lolflag=1;
                    printf("All the vessels prepared by Robot Chef %d are emptied. Resuming cooking now.\n",chefs[index].capacity);
                    break;
               }
          }
          if(!chefs[index].num_vessels && lolflag==0){
               printf("All the vessels prepared by Robot Chef %d are emptied. Resuming cooking now.\n",chefs[index].capacity);
               break;
          }
     }
}
void student_in_slot(int index,int j){
     tables[j].slots-=1;//chefs[index].capacity;
     tables[j].serving_container-=1;
     studentsremain--;
     pthread_mutex_lock(&mutex_waiting_students);
     students_waiting--;
     pthread_mutex_unlock(&mutex_waiting_students);
     pthread_mutex_unlock(&mutex_tables[j]);
     while(tables[j].slots!=0 && students_waiting!=0){

     }
     //sleep(1);
     // printf("Students ate at table %d. Slots left is %d\n",tables[j].index,tables[j].slots);
}
void wait_for_slot(int index){
     pthread_mutex_lock(&mutex_waiting_students);
     students_waiting++;
     pthread_mutex_unlock(&mutex_waiting_students);
     printf("Student %d has arrived\n",students[index].index);
     fflush(stdout);
     int cond=1;
     printf("Student %d is waiting to be allocated a slot on the serving table\n",students[index].index);
     while(cond){
          for(int j=0; j<n; j++){
               if(pthread_mutex_trylock(&mutex_tables[j])){
                    continue;
               }
               if(tables[j].slots>0){
                    printf("Student %d assigned a slot on the serving table %d and waiting to be served\n",students[index].index,tables[j].index);
                    fflush(stdout);
                    student_in_slot(index,j);
                    cond=0;
                    printf("Student %d done eating at table %d which has %d slots left\n",students[index].index,tables[j].index,tables[j].slots);
                    fflush(stdout);
                    //printf("Students ate at table %d. Slots left is %d\n",tables[j].index,tables[j].slots);
                    //pthread_mutex_unlock(&mutex_tables[j]);
                    break;
               }
               pthread_mutex_unlock(&mutex_tables[j]);
               continue;
          }
     }
}
void ready_to_serve_table(int number_of_slots, int index){
     while(1){
          if(pthread_mutex_trylock(&mutex_tables[index])){
               continue;
          }
          if(tables[index].slots==0 || studentsremain==0){
               if(tables[index].serving_container==0){
                    printf("Serving Container of Table %d is empty, waiting for refill\n",tables[index].index);
               }
               pthread_mutex_unlock(&mutex_tables[index]);
               break;
          }
          pthread_mutex_unlock(&mutex_tables[index]);
     }
}

void * chef_init(void * arg){
     int *index_pointer = (int*) arg;
     int index = *index_pointer;
     //usleep(1000);
     while(1){
          //struct robot_chef *chef=(struct robot_chef*) arg;
          chefs[index].preptime=2+(rand()%4);
          chefs[index].num_vessels=1+(rand()%10);
          chefs[index].capacity=25+(rand()%26);
          printf("Robot Chef %d is preparing %d vessels of Biryani\n",chefs[index].index,chefs[index].num_vessels);
          sleep(chefs[index].preptime);
          printf("Chef %d has prepared %d vessels which feed %d people in %d seconds. Waiting for all the vessels to be emptied to resume cooking\n",chefs[index].index,chefs[index].num_vessels,chefs[index].capacity,chefs[index].preptime);
          fflush(stdout);
          biryani_ready(index);
          //printf("%d %d\n",chef->index,chef->preptime);
          //sleep(1);
          // printf("%d\n",chef->index);
          if(studentsremain==0){
               break;
          }
     }
}
void * table_init(void * arg){
     int *index_pointer = (int*) arg;
     int index = *index_pointer;
     //usleep(1000);
     //struct serving_table *table=(struct serving_table*) arg;
     while(1){
          if(tables[index].serving_container>0){
               tables[index].slots=min(1+(rand()%10),tables[index].serving_container);
               printf("Serving Table %d serving with %d slots and has capacity %d\n",tables[index].index,tables[index].slots,tables[index].serving_container);
               fflush(stdout);
               ready_to_serve_table(tables[index].slots,tables[index].index);
          }
          if(studentsremain==0){
               break;
          }
     }
}
void * student_init(void * arg){
     int *index_pointer = (int*) arg;
     int index = *index_pointer;
     int dec=k/2;
     students[index].entry_time=rand()%10;
     printf("Student %d is going to arrive at time %d\n",students[index].index,students[index].entry_time);
     sleep(students[index].entry_time);
     //printf("lol %d\n",index);
     wait_for_slot(index);
}

int main(void){
     srand(time(NULL));
     scanf("%d %d %d",&m,&n,&k);
     //printf("lol\n");
     studentsremain=k;
     mutex_chefs=(pthread_mutex_t *)malloc((m)*sizeof(pthread_mutex_t));
     mutex_tables=(pthread_mutex_t *)malloc((n)*sizeof(pthread_mutex_t));
     pthread_mutex_init(&mutex_waiting_students,NULL);
     chefs=(struct robot_chef*)malloc(sizeof(struct robot_chef)*(m));
     pthread_t cheftids[m];
     //printf("lol\n");
     for(int i=0; i<m; i++){
          chefs[i].index=i;
          usleep(100);
          //printf("%d\n",chefs[i].index);
          pthread_create(&cheftids[i],NULL,chef_init,(void *)&chefs[i].index);
     }
     sleep(1);
     //printf("lol\n");
     tables=(struct serving_table*)malloc(sizeof(struct serving_table)*(n));
     pthread_t tabletids[n];
     for(int i=0; i<n; i++){
          tables[i].index=i;
          usleep(100);
          pthread_create(&tabletids[i],NULL,table_init,(void *)&tables[i].index);
     }
     sleep(1);
     students=(struct students*)malloc(sizeof(struct students)*(k));
     pthread_t studenttids[k];
     for(int i=0; i<k; i++){
          students[i].index=i;
          usleep(100);
          pthread_create(&studenttids[i],NULL,student_init,(void *)&students[i].index);
     }
     for(int i=0; i<k; i++){
          pthread_join(studenttids[i],NULL);
     }
     printf("Simulation Over.\n");
     return 0;
}

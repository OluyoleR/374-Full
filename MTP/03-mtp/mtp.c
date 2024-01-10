#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

int count_buf1 = 0 ; 
int count_buf2 = 0 ; 
int count_buf3 = 0 ; 

int con_idx1 = 0;
int con_idx2 = 0 ;
int con_idx3 = 0 ; 

int prod_idx1 = 0;
int prod_idx2 = 0 ;
int prod_idx3 = 0 ; 

char buffer1[50001];
char buffer2[50001];
char buffer3[50001];

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;



struct line_data{char * text; ssize_t len; };

struct line_data get_line()
{
    char * line  = NULL ; 
    size_t n_read = 0 ; 
    ssize_t char_read =  getline(&line,&n_read,stdin);
    struct line_data input = {line,char_read};
    return input;
}


void  * fill_buff1(void *args)
{
    while(1)
    {
        struct line_data newline = get_line();
        if (feof(stdin)) return NULL ;
        if (strcmp(newline.text,"STOP\n")==0) 
        {
            pthread_mutex_lock(&mutex1);
            buffer1[prod_idx1] = -1 ; 
            count_buf1 += 1;
            prod_idx1 += 1;
            pthread_cond_signal(&cond1);
            pthread_mutex_unlock(&mutex1);
            break;
        }else{
            pthread_mutex_lock(&mutex1);
            strcat(buffer1,newline.text);
            count_buf1 += (int)newline.len; 
            prod_idx1 += (int)newline.len; 
            free(newline.text);
            pthread_cond_signal(&cond1);
            pthread_mutex_unlock(&mutex1);   
        }
    }
    return NULL;
}

char get_buff1() 
{
    pthread_mutex_lock(&mutex1);
    while(count_buf1 == 0) 
        pthread_cond_wait(&cond1,&mutex1);
    char c = buffer1[con_idx1];
    con_idx1 += 1;
    count_buf1 -= 1 ; 
    pthread_mutex_unlock(&mutex1);
    return c ;
}

void put_buff2(char c)
{
    pthread_mutex_lock(&mutex2);;
    buffer2[prod_idx2] = c;
    count_buf2 +=1 ;
    prod_idx2 += 1 ;
    pthread_cond_signal(&cond2);
    pthread_mutex_unlock(&mutex2);
}

void * fill_buff2(void * args)
{
    char c = 0 ;
    while (1)
    {
        c = get_buff1();
        if (c == -1){
            put_buff2(c);
            break;
        }
        char new_c = (c =='\n'? ' ':c);
        put_buff2(new_c);
    }
    return NULL;
}


char get_buff2()
{
    pthread_mutex_lock(&mutex2);
    while (count_buf2 == 0 )
        pthread_cond_wait(&cond2,&mutex2); 
    char c = buffer2[con_idx2];
    con_idx2 += 1;
    count_buf2 -=1 ;
    pthread_mutex_unlock(&mutex2);
    return c ;
}

void put_buff3(char c )
{
    pthread_mutex_lock(&mutex3);
    buffer3[prod_idx3] = c ;
    count_buf3 +=1 ;
    prod_idx3 += 1;
    pthread_cond_signal(&cond3);
    pthread_mutex_unlock(&mutex3);    
}



void * fill_buff3(void * args){
    char c1 = 0 ; char c2 = 0 ;
    while(1){
        c1 = get_buff2();
        if (c1 == -1 ){ 
          put_buff3(c1);
          break ;
        }
        if(c1 != '+'){
            put_buff3(c1);
        }
        else{
            c2 = get_buff2();
            if (c2 == '+'){ put_buff3('^'); continue;} 
            else {
                put_buff3(c1);
                if(c2 == -1){ put_buff3(-1); break;}
                else 
                  put_buff3(c2);
            }
        }
    }
    return NULL;
}
char get_buff3()
{
    pthread_mutex_lock(&mutex3);
    while (count_buf3 == 0 )
        pthread_cond_wait(&cond3,&mutex3); 
    char c = buffer3[con_idx3];
    con_idx3 += 1;
    count_buf3 -=1 ;
    pthread_mutex_unlock(&mutex3);
    return c ;
}

void * output(void *args)
{
  char output[81] ; 
  output[80] = '\0';
  int output_idx = 0 ;

  while(1)
  {
    char c = get_buff3();
    if (c == -1) 
      break;
    output[output_idx] = c ;
    output_idx += 1 ;
    if (output_idx == 80){
      output[80]='\0';
      printf("%s\n",output);
      fflush(stdout);
      output_idx = 0;
    }
  }
  return NULL;
}

int main(void){
    pthread_t thread1,thread2,thread3,thread4;

    pthread_create(&thread1,NULL,fill_buff1,NULL);
    pthread_create(&thread2,NULL,fill_buff2,NULL);
    pthread_create(&thread3,NULL,fill_buff3,NULL);
    pthread_create(&thread4,NULL,output,NULL);

    pthread_join(thread1,NULL); 
    pthread_join(thread2,NULL);
    pthread_join(thread3,NULL);
    pthread_join(thread4,NULL);
    
    return 0 ; 
       
}

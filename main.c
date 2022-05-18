#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/ioctl.h>

#define OUTPUT_1   "t1_out"
#define OUTPUT_2   "t2_out"

pthread_attr_t tattr;
struct sched_param param;
pthread_t tid1;                                                   //Initializing variable for threads i.e threadId 
pthread_t tid2;
pthread_t main_id;

FILE *tin[2],*tout[2];

int fd0, fd1;
typedef struct ht_object                                        //initializing the headers
{
	int key;
	char data[4] ;
} ht_object_t;

struct filep {
    FILE *input_file;
    FILE *output_file;
};

void copy_d(char *buf, char *source)                     //creating function to cpy data
{
    for(int i=0; i<strlen(source); i++) 
    {
        buf[i] = source[i];
    }
}

char arr[4]; 

char *data_p(char *x)
{
    memset(arr, 0, sizeof(data_p));

    for(int i=0; i<4; i++) {
        arr[i] = x[i];
    }

    return arr;
}


ht_object_t user;
// struct dump_arg 
// {
//     int n; // the n-th bucket (in) or n objects retrieved (out)
//     ht_object_t object_array[8] ; // to retrieve at most 8 objects from the n-th bucket
// } ;


void *p_cmd(void *args)                     //routine function
{
    int t_num = *(int *)args;                                                   //initializing the variables
    char inp[100];
    char cmd;
    int ht;
    int t_key;
    int sleep;
    char t_data[4] = {0};
    //ret;
    //int i;
    //struct ht_object_t *ht_node;

    while(fgets(inp,sizeof(inp),tin[t_num]))                             //getting the input data from file
    {
        ht_object_t u_obj = {0};

        if(inp[0]== 'w')                      //checking for the command                           
        {   
            memset(t_data, 0, sizeof(t_data));
            sscanf(inp,"%c %d %d %4s",&cmd,&ht, &t_key, t_data);              //storing the input into the variables
            u_obj.key = t_key;
            copy_d(u_obj.data, t_data);
            printf("Operation: %c, Table: %d, Key: %d, Data: %s\n",cmd, ht, u_obj.key, data_p(u_obj.data));
            if(ht == 0)
            {
                write(fd0,&u_obj,sizeof(ht_object_t));        //writing 
            }
            else if(ht == 1)
            {
                write(fd1,&u_obj,sizeof(ht_object_t));
            }
        }
        else if(inp[0]=='r')                                                //checking if the input is read command
        {
            char read_dummy[50] = {0};

            sscanf(inp,"%c %d %d",&cmd, &ht, &t_key);
            u_obj.key=t_key;
            if(ht == 0)
            {
                read(fd0,&u_obj,sizeof(ht_object_t));                      //Read
            }
            else if(ht == 1)
            {
                read(fd1,&u_obj,sizeof(ht_object_t));
            }
            sprintf(read_dummy, "read from ht438_tbl_%d\nkey=%d\tdata=%s\n", ht, u_obj.key, data_p(u_obj.data));
            fputs(read_dummy, tout[t_num]);
            printf("Operation: %c, Table: %d, Key: %d, Data: %s\n",cmd, ht, u_obj.key, data_p(u_obj.data));

            //printf("")
        }
        else if(inp[0] == 's')                                  //checking for sleep
        {
            sscanf(inp,"%c %d",&cmd,&sleep);
            printf("Operation: %c, Sleep Time: %d us\n",cmd, sleep);
            usleep(sleep);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv)                       //main function
{
    int thread1 =0 , thread2 =1;                              //initializing the variable
    int flag;
    printf("argc = %d\n",argc);
    pthread_attr_init(&tattr);
    main_id = pthread_self();                           //schecduling the main
    /*open the two character device files ht438_dev_0 and ht438_dev_1*/

    fd0 = open("/dev/ht438_dev_0", O_RDWR);
    // if(fd0 < 0) {
    //     printf("Error opening file 1\n");
    //     return -1;
    // }
    fd1 = open("/dev/ht438_dev_1", O_RDWR);
    // if(fd1 < 0) {
    //     printf("Error opening file 2\n");
    //     return -1;
    // }

    tout[0] =fopen(OUTPUT_1,"w");
    if(tout[0] == NULL) 
    {
        printf("Cannot open output file 1 \n");
    }
    tout[1] =fopen(OUTPUT_2,"w");
    if(tout[1] == NULL) 
    {
        printf("Cannot open output file 2 \n");
    }
    /***set scheduling policy to RT FIFO and pthread priority 90***/

    param.sched_priority = 90;
    pthread_setschedparam(main_id, SCHED_FIFO, &param);

    /*open the two input files*/
    printf("opening files\n");
    tin[0] = fopen(argv[1],"r");
    tin[1] = fopen(argv[2],"r");


// create two testing pthreads with RT FIFO scheduling and pthread priority 80 and 70

    param.sched_priority = 80;
    pthread_attr_setschedparam(&tattr, &param);
    if (pthread_create(&tid1, &tattr,p_cmd,&thread1)!=0) 
    {
        perror("Failed to create thread\n");
        return 1;
    }
    else
    {
        printf("thread 1 created\n");

    }
    // pthread_create(&tid1,&tattr,p_cmd,tp1);
    // printf("thread 1 created");

    param.sched_priority = 70;
    pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);
    if (pthread_create(&tid2, &tattr,p_cmd,&thread2)!=0) 
    {
        perror("Failed to create thread 2\n");
        return 1;
    }
    else
    {
        printf("thread 2 created\n");

    }
    // pthread_create(&tid2,&tattr,p_cmd,tp2);
    // printf("thread 2 created");


    pthread_join(tid1,NULL);                                              //joining the threads
    pthread_join(tid2,NULL);

    pthread_attr_destroy(&tattr);                                                               //destroying the attributes
    flag = fclose(tin[0]);                          //closing files
    if(flag) {
        printf("Cannot close file\n");
    }

    flag = fclose(tin[1]);
    if(flag) {
        printf("Cannot close file\n" );
    }

    flag = fclose(tout[0]);
    if(flag) {
        printf("Cannot close file\n");
    }

    flag = fclose(tout[1]);
    if(flag) {
        printf("Cannot close file\n");
    }

    close(fd0);                              //closing device files
    // if(flag) {
    //     printf("Error closing\n");
    // }

    close(fd1);
    // if(flag) {
    //     printf("Error closing \n");
    // }

    return 0;

}
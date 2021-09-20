#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <map>
#include <arpa/inet.h>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <boost/asio/thread_pool.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/post.hpp>
#include <queue>
#define PORT 8080
#define MUTEX_TABLE_SIZE 997
#define MAXVAL 100


int keylength;
std::mutex mutex_table[MUTEX_TABLE_SIZE];
std::map<std::string, int> ht;

std::string make_key( size_t length )
{
    auto randchar = []() -> char
    {
        const char charset[] =
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
     std::string str(length,0);
     std::generate_n( str.begin(), length,   randchar );
     return str;
}

typedef struct command_and_response_single{
    int client_thread;
    int ctr;
    //std::string client_IP;
    //int IPlength;

    int command;    // get = 0, put = 1
    int status;     // client->server initial send: -5, server->client lock success: -4, server->client lock failure: -2, client->servers proceed: -3, client->servers abort: -1: server->client proceed success: 0, server->client proceed failure: 2 server->client confirm abort: 4
    int stc_id;    // child_socket, comes in as -1, changes to what is should be
    int value;
    int keylength;
    int multiput_redundancy;
    std::string key;
} command_and_response_single;



std::string write_car_status_single(command_and_response_single car, int status_number){
    std::string returnstring;
    returnstring = "(SVR:SSS) / (CNT:CCC) / (THD:" + std::to_string(car.client_thread) +
                                        ") / (KEY:" + car.key  +
                                        ") / (VAL:" + std::to_string(car.value) +
                                        ") / (CMD:" + std::to_string(car.command) +
                                        ") / (STS:" + std::to_string(car.status)  +
                                        ") / (ORDR:" + std::to_string(status_number)  +
                                        ")";
    return returnstring;
}

struct command_and_response_single make_rand_car_single(int cthread, int counter, int keylength){
    command_and_response_single c;
    c.client_thread = cthread;
    c.ctr = counter;

    c.key = make_key(keylength);
    c.keylength = keylength;
    c.value = rand()%MAXVAL;
    c.multiput_redundancy = 0;

    int putOrGet = 1+rand() % 100;
    if (putOrGet <= 60 ){c.command = 0;} else {c.command = 1;}
    c.status = 2;
    c.stc_id = -1;
    return c;
}

void serialize_single(char* outbuffer, command_and_response_single* outcar, int keylength){      // will eventually need to serialize for multiple messagers
    bzero(outbuffer, 512);

    int* ip = (int*)outbuffer;
    *ip = outcar->client_thread; ip++;
    *ip = outcar->ctr; ip++;

    *ip = outcar->command; ip++;
    *ip = outcar->status; ip++;
    *ip = outcar->stc_id; ip++;
    *ip = outcar->value; ip++;
    *ip = outcar->keylength; ip++;
    *ip = outcar->multiput_redundancy; ip++;

    char* cp = (char*)ip;
    int i = 0;
    while (i < keylength){
        *cp = outcar->key[i];
        cp++; i++;
    }

}

void deserialize_single(char* inbuffer, command_and_response_single* incar, int keylength){
    int* ip = (int*)inbuffer;
    incar->client_thread = *ip; ip++;
    incar->ctr = *ip; ip++;

    incar->command = *ip; ip++;
    incar->status = *ip; ip++;
    incar->stc_id = *ip; ip++;
    incar->value = *ip; ip++;
    incar->keylength = *ip; ip++;
    incar->multiput_redundancy = *ip; ip++;

    char* cp = (char*)ip;
    int i = 0;
    std::string temp = "";
    while (i<keylength){
        temp += *cp;
        cp++;
        i++;
    }
    incar->key = temp;
}

typedef struct command_and_response_multi{
    int client_thread;
    int ctr;
    int keylength;
    int stc_id;    // child_socket, comes in as -1, changes to what is should be
    
    //std::string client_IP;
    //int IPlength;

    int num_commands;
    int lock_sf_arr[3];
    int command_arr[3];    // get = 0, put = 1
    int status_arr[3];     // client->server initial send: -5, server->client lock success: -4, server->client lock failure: -2, client->servers proceed: -3, client->servers abort: -1: server->client proceed success: 0, server->client proceed failure: 2 server->client confirm abort: 4
    std::string key_arr[3];
    int value_arr[3];
//    int multiput_redundancy;
    
} command_and_response_multi;


struct command_and_response_multi make_rand_car_multi(int cthread, int counter, int keylength){
    command_and_response_multi c;
    c.client_thread = cthread;
    c.ctr = counter;
    c.keylength = keylength;
    c.stc_id = -1;
    c.num_commands = 0;
    for (int i=0; i<3; i++){
        c.lock_sf_arr[i] = 0;
        c.command_arr[i] = 1; // if we're doing multi, the command is put. that's it.
        c.status_arr[i] = 2;
        c.key_arr[i] = make_key(c.keylength);
        c.value_arr[i] = rand()%MAXVAL;
    }

    // int putOrGet = 1+rand() % 100;
    // if (putOrGet <= 60 ){c.command = 0;} else {c.command = 1;}
    
    return c;
}

void serialize_multi(char* outbuffer, command_and_response_multi* outcar, int keylength){      // will eventually need to serialize for multiple messagers
    bzero(outbuffer, 512);
    char* cp;
    int* ip = (int*)outbuffer;
    *ip = outcar->client_thread; ip++;
    *ip = outcar->ctr; ip++;
    *ip = outcar->keylength; ip++;
    *ip = outcar->stc_id; ip++;
    *ip = outcar->num_commands; ip++;

    for (int i = 0; i < outcar->num_commands; i++){
        *ip = outcar->lock_sf_arr[i]; 
        ip++;
        *ip = outcar->command_arr[i]; 
        ip++;
        *ip = outcar->status_arr[i]; 
        ip++;
        *ip = outcar->value_arr[i]; 
        ip++;

        cp = (char*)ip; 
        int l = 0; 
        while (l < keylength){
            *cp = outcar->key_arr[i][l]; 
            cp++; 

            // std::cout<<"current letter dropping in: "<<*cp<<std::endl;
            l++; 
        }
        ip = (int*)cp;


    }
}

void deserialize_multi(char* inbuffer, command_and_response_multi* incar, int keylength){
    int* ip = (int*)inbuffer;
    char *cp;
    incar->client_thread = *ip; ip++;
    incar->ctr = *ip; ip++;
    incar->keylength = *ip; ip++;
    incar->stc_id = *ip; ip++;
    incar->num_commands = *ip; ip++;
    for (int i = 0; i<3; i++){
        incar->lock_sf_arr[i] = *ip; ip++;
        incar->command_arr[i] = *ip; ip++;
        incar->status_arr[i] = *ip; ip++;
        incar->value_arr[i] = *ip; ip++;

        cp = (char*)ip;        
        int l = 0;
        std::string temp;
        while (l<keylength){
            temp += *cp;
//            std::cout<<"adding "<<*cp<<std::endl;
            cp++;
            l++;
        }
        incar->key_arr[i] = temp;
        ip = (int*)cp;
    }
}

std::string write_car_status_multi(command_and_response_multi car, int status_number){
    std::string returnstring ="";
    for (int i =0; i<3; i++){
        returnstring = returnstring + "(SVR:SSS) / (CNT:CCC) / (THD:" + std::to_string(car.client_thread) +
                                            ") / (MP#:" + std::to_string(car.num_commands) +
                                            ") / (KEY:" + car.key_arr[i]  +
                                            ") / (VAL:" + std::to_string(car.value_arr[i]) +
                                            ") / (CMD:" + std::to_string(car.command_arr[i]) +
                                            ") / (STS:" + std::to_string(car.status_arr[i])  +
                                            ") / (ORDR:" + std::to_string(status_number)  +
                                            ")\n";
    }
    return returnstring;
}



//////


int get(std::string key, std::map<std::string, int>& ht){
    auto it = ht.find(key);
    if (it!=ht.end()){
        return 1;
    }
    return 0;
}

int put(std::string key, int val, std::map<std::string, int>& ht){
    if (get(key, ht) == 0){
        ht.insert({key,val});
        return 1;
    }
    return 0;
};

int multithreaded_hashtable_listener(int handle, int keylength){

    std::cout<<"THREAD START"<<std::endl;

    std::string status1;
    std::string status2;
    std::string status3;
    std::string status4;
    std::string finalstatus;
    std::hash<std::string> str_hash;



while (true){

    char receive_buffer[512];
    bzero(receive_buffer,512);

    int sf;
    int valread;
    
    // receive car 1
    sf = recv( handle , receive_buffer, sizeof(receive_buffer), 0);
    if (sf <= 0){
        std::cout<<"Valread is: "<<valread<<std::endl;
        close( handle);
        break;
    }
    else{   // valread was successful! you have work to do
      //  std::cout<<"Valread is: "<<valread<<std::endl;
        command_and_response_multi car_1 = make_rand_car_multi(-100,-100, keylength);
        std::cout<<"deserializing: "<<std::endl;
        deserialize_multi(receive_buffer, &car_1, keylength);
        for (int k = 0; k< car_1.num_commands; k++){
            car_1.stc_id = handle;
            
        }
        std::cout<<"VVV Client thread "<<car_1.client_thread<<" car received s/f: "<<sf<<std::endl;
        status1 = write_car_status_multi(car_1,1);
        std::cout<<status1<<std::endl;

        for (int k = 0; k<car_1.num_commands; k++){
            if (car_1.status_arr[k] != 2){
                std::cout<<"problem! first wave! "<<std::endl;
                std::string status = write_car_status_multi(car_1,1);
                std::cout<<status<<"\n";
                std::cout<<"k here is"<<k<<"\n";
                return 0;
            }
            else{
                
                int command_result_n101;
                int client_thread_id = car_1.client_thread;
                
                if ((mutex_table[int(str_hash(car_1.key_arr[k])%MUTEX_TABLE_SIZE)]).try_lock()){

                    car_1.status_arr[k] = 3;
                    car_1.lock_sf_arr[k] = 1;

                }
                else{   // LOCK FAILURE

                    car_1.status_arr[k] = 1;    // lock failure
                    car_1.lock_sf_arr[k] = 0;

                }
            }
        }
        char sender_buffer[512];
        std::cout<<"Serializing"<<std::endl;
        serialize_multi(sender_buffer, &car_1, keylength);
        sf = send(handle,sender_buffer, sizeof(sender_buffer),0);
        status2 = write_car_status_multi(car_1,2);
        std::cout<<"VVV Client thread "<<car_1.client_thread<<" car sent s/f: "<<sf<<std::endl;
        std::cout<<status2<<std::endl;



        sf = recv( handle , receive_buffer, sizeof(receive_buffer), 0);
        if (sf <= 0){
            std::cout<<"Valread is: "<<valread<<std::endl;
            close( handle);
            break;
        }
        else{   // valread was successful! you have work to do
        //  std::cout<<"Valread is: "<<valread<<std::endl;
            command_and_response_multi car_2 = make_rand_car_multi(-100,-100, keylength);
            std::cout<<"deserializing: "<<std::endl;
            deserialize_multi(receive_buffer, &car_2, keylength);
            for (int k = 0; k< car_2.num_commands; k++){
                car_2.stc_id = handle;
            }
            std::cout<<"VVV Client thread "<<car_2.client_thread<<" car received s/f: "<<sf<<std::endl;
            status3 = write_car_status_multi(car_2,1);
            std::cout<<status3<<std::endl;



            for (int k = 0; k<car_2.num_commands; k++){
                if (car_2.status_arr[k] != 2 && car_2.status_arr[k]){
                    std::cout<<"problem! second wave wave! "<<std::endl;
                    std::string status = write_car_status_multi(car_2,2);
                    std::cout<<status<<"\n";
                    std::cout<<"k here is"<<k<<"\n";
                    return 0;
                }
                else{
                    
                    int command_result_n101;
                    int client_thread_id = car_1.client_thread;
                    // command_and_response_single car_2 = car_1;
                    
                    if (car_2.status_arr[k] == 6){ // don't execute this command

                        switch (car_2.command_arr[k]){
                            case 0:{command_result_n101 = get(car_2.key_arr[k], ht);} break;// GET
                            case 1:{command_result_n101 = put(car_2.key_arr[k], car_2.value_arr[k], ht);} break;// PUT
                            default:{
                                std::cout<<"JKL:LKJ:LPOIUYPOIUHP"<<std::endl;
                            } break;
                        }

                        switch (command_result_n101){
                            case 0:{
                                car_2.status_arr[k] = 9;
                            } break;
                            case 1:{
                                //7
                                car_2.status_arr[k] = 7;
                            } break;
                            default:{
                                std::cout<<";&*()&*)((*Ysakj"<<std::endl;
                            } break;
                        }
                    }
                    else{   // should have received status 4

                        car_2.status_arr[k] = 5;    // lock failure
                        car_1.lock_sf_arr[k] = 0;
                    }
                }
            }

            // send
            char sender_buffer[512];
            std::cout<<"Serializing"<<std::endl;
            serialize_multi(sender_buffer, &car_2, keylength);
            sf = send(handle,sender_buffer, sizeof(sender_buffer),0);
            status2 = write_car_status_multi(car_2,2);
            std::cout<<"VVV Client thread "<<car_2.client_thread<<" car sent s/f: "<<sf<<std::endl;
            std::cout<<status2<<std::endl;
        
    }


    }
}
return 0;
}




int main(int argc, char *argv[])
{

    srand(time(NULL));
    int initialize_amount = 0;
    int keylength=1;
    int option;

    while((option = getopt(argc, argv,"k:s:")) != -1){
        switch(option){
            case 'k':
                keylength = atof(optarg);
                std::cout<<"Key size is: "<<keylength<<std::endl;
                break;
        }
    }


    std::cout<< "Program start. "<<std::endl;
    int master_socket_fd;

    if ((master_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ perror("socket failed"); exit(EXIT_FAILURE); } // ask palmieri about master_socket_fd vs child_socket

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    if (bind(master_socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0){ perror("bind failed"); exit(EXIT_FAILURE); }
    printf("Socket bound to port.\n");

    char receive_buffer[512];

    if (listen(master_socket_fd, 3) < 0){perror("listen"); exit(EXIT_FAILURE);}
    printf("Listening on port %d...\n", PORT);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(master_socket_fd, &readfds);
    int max_sd = master_socket_fd;  // not sure why, but this is needed for select


    boost::asio::thread_pool tp(40);
    int client_socket;


    std::cout<<"Begin the while loop!"<<std::endl;

    auto fstart = std::chrono::steady_clock::now();
    auto fend = std::chrono::steady_clock::now();
    int op_duration_s = std::chrono::duration_cast<std::chrono::seconds>(fend - fstart).count();

    while (true){ // op_duration_s < 10){
        std::cout<<op_duration_s<<std::endl;
        fend = std::chrono::steady_clock::now();
        op_duration_s = std::chrono::duration_cast<std::chrono::seconds>(fend - fstart).count();

        int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if ((activity < 0) && (errno!=EINTR)){ printf("select error"); }

        if (FD_ISSET(master_socket_fd, &readfds)){ //FD_ISSET checks on a socket_id in the socket set, assess if there is activity there or not - this line checks on the mastersocket
            if ((client_socket = accept(master_socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) { perror("accept");exit(EXIT_FAILURE); }
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , client_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            boost::asio::post(tp,boost::bind(multithreaded_hashtable_listener,client_socket, keylength));
        }
        std::cout<<"fhkfdjaklf;da"<<std::endl;
    }


    std::cout<<"fhkfdjaklf;da"<<std::endl;
    tp.join();
    return 0;
}

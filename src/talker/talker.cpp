#include <fstream>
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <map>
#include <list>
#include <pthread.h>
#include <thread>
#include <algorithm>
#include <ratio>
#include <chrono>

#define PORT 8080
#define MAXVAL 100

int keylength;

std::string make_key(size_t length)
{
    auto randchar = []() -> char {
        const char charset[] =
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

typedef struct command_and_response_single
{
    int client_thread;
    int ctr;
    //std::string client_IP;
    //int IPlength;

    int command; // get = 0, put = 1
    int status;  // client->server initial send: -5, server->client lock success: -4, server->client lock failure: -2, client->servers proceed: -3, client->servers abort: -1: server->client proceed success: 0, server->client proceed failure: 2 server->client confirm abort: 4
    int stc_id;  // child_socket, comes in as -1, changes to what is should be
    int value;
    int keylength;
    int multiput_redundancy;
    std::string key;
} command_and_response_single;

std::string write_car_status_single(command_and_response_single car, int status_number, int svr_idx)
{
    std::string returnstring;

    returnstring = "(SVR:" + std::to_string(svr_idx) + ") / (CNT:CCC) / (THD:" + std::to_string(car.client_thread) +
                   ") / (KEY:" + car.key +
                   ") / (VAL:" + std::to_string(car.value) +
                   ") / (CMD:" + std::to_string(car.command) +
                   ") / (STS:" + std::to_string(car.status) +
                   ") / (ORDR:" + std::to_string(status_number) +
                   ")";
    return returnstring;
}

struct command_and_response_single make_rand_car_single(int cthread, int counter, int keylength)
{
    command_and_response_single c;
    c.client_thread = cthread;
    c.ctr = counter;

    c.key = make_key(keylength);
    c.keylength = keylength;
    c.value = rand() % MAXVAL;
    c.multiput_redundancy = 0;

    int putOrGet = 1 + rand() % 100;
    if (putOrGet <= 60)
    {
        c.command = 0;
    }
    else
    {
        c.command = 1;
    }
    c.status = 2;
    c.stc_id = -1;
    return c;
}

void serialize_single(char *outbuffer, command_and_response_single *outcar, int keylength)
{ // will eventually need to serialize for multiple messagers
    bzero(outbuffer, 512);

    int *ip = (int *)outbuffer;
    *ip = outcar->client_thread;
    ip++;
    *ip = outcar->ctr;
    ip++;

    *ip = outcar->command;
    ip++;
    *ip = outcar->status;
    ip++;
    *ip = outcar->stc_id;
    ip++;
    *ip = outcar->value;
    ip++;
    *ip = outcar->keylength;
    ip++;
    *ip = outcar->multiput_redundancy;
    ip++;

    char *cp = (char *)ip;
    int i = 0;
    while (i < keylength)
    {
        *cp = outcar->key[i];
        cp++;
        i++;
    }
}

void deserialize_single(char *inbuffer, command_and_response_single *incar, int keylength)
{
    int *ip = (int *)inbuffer;
    incar->client_thread = *ip;
    ip++;
    incar->ctr = *ip;
    ip++;

    incar->command = *ip;
    ip++;
    incar->status = *ip;
    ip++;
    incar->stc_id = *ip;
    ip++;
    incar->value = *ip;
    ip++;
    incar->keylength = *ip;
    ip++;
    incar->multiput_redundancy = *ip;
    ip++;

    char *cp = (char *)ip;
    int i = 0;
    std::string temp = "";
    while (i < keylength)
    {
        temp += *cp;
        cp++;
        i++;
    }
    incar->key = temp;
}

typedef struct command_and_response_multi
{
    int client_thread;
    int ctr;
    int keylength;
    int stc_id; // child_socket, comes in as -1, changes to what is should be

    //std::string client_IP;
    //int IPlength;

    int num_commands;
    int lock_sf_arr[3];
    int command_arr[3]; // get = 0, put = 1
    int status_arr[3];  // client->server initial send: -5, server->client lock success: -4, server->client lock failure: -2, client->servers proceed: -3, client->servers abort: -1: server->client proceed success: 0, server->client proceed failure: 2 server->client confirm abort: 4
    std::string key_arr[3];
    int value_arr[3];
    //    int multiput_redundancy;

} command_and_response_multi;

struct command_and_response_multi make_rand_car_multi(int cthread, int counter, int keylength)
{
    command_and_response_multi c;
    c.client_thread = cthread;
    c.ctr = counter;
    c.keylength = keylength;
    c.stc_id = -1;
    c.num_commands = 0;
    for (int i = 0; i < 3; i++)
    {
        std::cout << i << std::endl;
        c.lock_sf_arr[i] = 0;
        c.command_arr[i] = 1; // if we're doing multi, the command is put. that's it.
        c.status_arr[i] = 2;
        c.key_arr[i] = make_key(c.keylength);
        c.value_arr[i] = rand() % MAXVAL;
    }

    // int putOrGet = 1+rand() % 100;
    // if (putOrGet <= 60 ){c.command = 0;} else {c.command = 1;}

    return c;
}
void serialize_multi(char *outbuffer, command_and_response_multi *outcar, int keylength)
{ // will eventually need to serialize for multiple messagers
    bzero(outbuffer, 512);
    char *cp;
    int *ip = (int *)outbuffer;
    *ip = outcar->client_thread;
    ip++;
    *ip = outcar->ctr;
    ip++;
    *ip = outcar->keylength;
    ip++;
    *ip = outcar->stc_id;
    ip++;
    *ip = outcar->num_commands;
    ip++;

    for (int i = 0; i < outcar->num_commands; i++)
    {
        *ip = outcar->lock_sf_arr[i];
        ip++;
        *ip = outcar->command_arr[i];
        ip++;
        *ip = outcar->status_arr[i];
        ip++;
        *ip = outcar->value_arr[i];
        ip++;

        cp = (char *)ip;
        int l = 0;
        while (l < keylength)
        {
            *cp = outcar->key_arr[i][l];
            cp++;

            std::cout << "curr letter: " << *cp << std::endl;
            l++;
        }
        ip = (int *)cp;
    }
}

void deserialize_multi(char *inbuffer, command_and_response_multi *incar, int keylength)
{
    int *ip = (int *)inbuffer;
    char *cp;
    incar->client_thread = *ip;
    ip++;
    incar->ctr = *ip;
    ip++;
    incar->keylength = *ip;
    ip++;
    incar->stc_id = *ip;
    ip++;
    incar->num_commands = *ip;
    ip++;
    for (int i = 0; i < 3; i++)
    {
        incar->lock_sf_arr[i] = *ip;
        ip++;
        incar->command_arr[i] = *ip;
        ip++;
        incar->status_arr[i] = *ip;
        ip++;
        incar->value_arr[i] = *ip;
        ip++;

        cp = (char *)ip;
        int l = 0;
        std::string temp;
        while (l < keylength)
        {
            temp += *cp;
            // std::cout<<"temp = "<<temp<<std::endl;
            cp++;
            l++;
        }
        incar->key_arr[i] = temp;
        ip = (int *)cp;
    }
}

int num_servers;
int total_keyrange;

int get_server_index(std::string key)
{
    std::hash<std::string> str_hash;
    int temp = (str_hash(key) % (total_keyrange)); // creates a hash from the string which is the key
    return temp / (total_keyrange / num_servers);
}

std::string write_car_status_multi(command_and_response_multi car, int status_number)
{
    std::string returnstring = "";
    for (int i = 0; i < 3; i++)
    {
        returnstring = returnstring + "(SVR:" + std::to_string(get_server_index(car.key_arr[i])) +
                       ") / (CNT:CCC) / (THD:" + std::to_string(car.client_thread) +
                       ") / (MP#:" + std::to_string(car.num_commands) +
                       ") / (KEY:" + car.key_arr[i] +
                       ") / (VAL:" + std::to_string(car.value_arr[i]) +
                       ") / (CMD:" + std::to_string(car.command_arr[i]) +
                       ") / (STS:" + std::to_string(car.status_arr[i]) +
                       ") / (ORDR:" + std::to_string(status_number) +
                       ")\n";
    }
    return returnstring;
}

void convert_sc_to_mc(command_and_response_single *sc_array, int sc_array_length, command_and_response_multi *mc_array, int mc_array_length)
{
    for (int i = 0; i < sc_array_length; i++)
    {
        //    std::cout<<" key, value = "<< sc_array[i].key << ", "<< sc_array[i].value <<std::endl;
        int si = get_server_index(sc_array[i].key);
        int mpn = mc_array[si].num_commands;
        //     std::cout<<"server index = "<<si<<std::endl;
        std::cout << "mpn is... " << mpn << std::endl;
        mc_array[si].lock_sf_arr[mpn] = 1;

        mc_array[si].command_arr[mpn] = sc_array[i].command;
        mc_array[si].command_arr[mpn] = 1;

        mc_array[si].status_arr[mpn] = sc_array[i].status;

        //        std::cout<<"mp old key = "<<mc_array[si].key_arr[mpn]<<std::endl;
        mc_array[si].key_arr[mpn] = sc_array[i].key;
        //        std::cout<<"updated key = "<<mc_array[si].key_arr[mpn]<<std::endl;
        //        std::cout<<"mp old value = "<<mc_array[si].value_arr[mpn]<<std::endl;
        mc_array[si].value_arr[mpn] = sc_array[i].value;
        //        std::cout<<"updated value = "<<mc_array[si].value_arr[mpn]<<std::endl;
        //        std::cout<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxx"<<std::endl;
        mc_array[si].num_commands++;
    }
}

int send_and_receive_messages(std::vector<std::string> IP_addresses, int keylength, int thread_id)
{ // gonna use this with 8 threads

    std::string status1;
    std::string status2;
    std::string status3;
    std::string status4;
    std::string finalstatus;

    int redundancy = 2;

    std::ofstream thread_file;
    std::string output_name = "test_results/" + std::to_string(thread_id) + ".txt";
    thread_file.open(output_name);
    // thread_file<<"Top of file, hello:\n";

    std::vector<sockaddr_in> server_vec;
    std::vector<int> client_sock_vec;
    int client_sock;
    for (int i = 0; i < num_servers; i++)
    {

        client_sock = 0; // client sock should be an int, this step clears client_sock
        if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        } // create new socket which is associated with the specific thread

        std::cout << "Thread " << thread_id << " accesses socket number: " << client_sock << std::endl;
        thread_file << "Thread id: " << thread_id << " accesses socket number: " << client_sock << "\n";

        struct sockaddr_in curr_serv_addr;
        curr_serv_addr.sin_family = AF_INET;
        curr_serv_addr.sin_port = htons(PORT);
        char IP_charray[IP_addresses[i].length() + 1];
        strcpy(IP_charray, IP_addresses[i].c_str());
        if (inet_pton(AF_INET, IP_charray, &curr_serv_addr.sin_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        } // pass in the address of serv_addr.sin_addr , gonna modify it

        // these are associated by index
        server_vec.push_back(curr_serv_addr);
        client_sock_vec.push_back(client_sock);
    }

    int flag;

    for (int i = 0; i < num_servers; i++)
    {
        flag = 1;
        while (flag)
        {
            if (connect(client_sock_vec[i], (struct sockaddr *)&server_vec[i], sizeof(server_vec[i])) < 0)
            {
                printf("\nConnection Failed, retrying in 5 seconds... \n");
                sleep(5);
            }
            else
            {
                flag = 0;
            }
        }
    }
    //usleep(1000000);
    std::cout << "Thread " << thread_id << " connected." << std::endl;
    // thread_file<<"connected\n";
    int get_success = 0;
    int get_failure = 0;
    int put_success = 0;
    int put_failure = 0;
    int error_count = 0;
    srand(time(NULL));

    auto opstart = std::chrono::steady_clock::now();

    int cum_optime_us = 0;
    int ctr = 0;
    while (ctr < 50)
    { // usually this is 10000

        thread_file << "-----------------------------------------------------------\n";

        

        int sf = 0;

        command_and_response_single car_1 = make_rand_car_single(thread_id, ctr, keylength);

        // int server_index = get_server_index(car_1.key, num_servers, total_keyrange);        /// old
        command_and_response_single car_s_arr[3];
        for (int i = 0; i < 3; i++)
        {
            car_s_arr[i] = make_rand_car_single(thread_id, ctr, keylength);
            std::cout << write_car_status_single(car_s_arr[i], -777, get_server_index(car_s_arr[i].key)) << std::endl;
        }

        command_and_response_multi car_m_arr[num_servers];
        command_and_response_multi car_m_arr2[num_servers];
        for (int i = 0; i < num_servers; i++)
        {
            car_m_arr[i] = make_rand_car_multi(thread_id, ctr, keylength);
            for (int k = 0; k<3;k++){
                car_m_arr[i].status_arr[k] = 2;
                
            }
            
            car_m_arr2[i] = car_m_arr[i];
            std::cout << write_car_status_multi(car_m_arr[i], -777) << std::endl;
        }
        // car_m_arr2 = car_m_arr;
        for (int i = 0; i < num_servers; i++)
        {
            for (int k = 0; k < 3; k++)
            {
                car_m_arr2[i].status_arr[k] = 3;
                car_m_arr2[i].lock_sf_arr[k] = 1;
            }
        }

        convert_sc_to_mc(car_s_arr, 3, car_m_arr, num_servers);
        std::cout << "conversion successful hopefully" << std::endl;

        // phase 1 of 4
        char outgoing[512];
        char response_buffer[512];
        int lock_check_array[3];
        for (int i = 0; i < 3; i++)
        {
            lock_check_array[i] = 1;
        }
        //int all_locks_available = 1;
        //command_and_response_single car_2 = make_rand_car_single(thread_id,ctr,keylength);

        // first send and receive cycle
        for (int k = 0; k < num_servers; k++)
        {
            if (car_m_arr[k].num_commands > 0)
            {
                std::cout << write_car_status_multi(car_m_arr[k], -777) << "\n";
                int server_index = get_server_index(car_m_arr[k].key_arr[0]);
                std::cout << "now doing send and receive for this object!@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
                for (int idx = server_index; idx < server_index + redundancy; idx++)
                {
                    // get the relevant communication socket
                    int curr_soc = client_sock_vec[idx % num_servers];

                    command_and_response_multi car_1 = car_m_arr[k];
                    // serialize from car_1 and send
                    bzero(outgoing, 512);
                    std::cout << "Serializing" << std::endl;
                    serialize_multi(outgoing, &car_1, keylength);
                    sf = send(curr_soc, outgoing, sizeof(outgoing), 0); // send based on client_sock[relevant_index] - i
                    std::cout << "VVV Thread " << thread_id << " send s/f: " << sf << std::endl;
                    status1 = write_car_status_multi(car_1, 1); //, idx%num_servers);
                    std::cout << status1 << std::endl;
                    thread_file << status1 + "\n";

                    // receive and deserialize into car_2
                    bzero(response_buffer, 512);
                    sf = recv(curr_soc, response_buffer, sizeof(response_buffer), 0);
                    std::cout << "deserializing: " << std::endl;
                    deserialize_multi(response_buffer, &car_1, keylength);

                    std::cout << "VVV Thread " << thread_id << " receive s/f: " << sf << std::endl;
                    status2 = write_car_status_multi(car_1, 2); // , idx%num_servers);
                    std::cout << status2 << std::endl;
                    thread_file << status2 + "\n";

                    for (int i = 0; i < car_m_arr[k].num_commands; i++)
                    {
                        if (car_1.status_arr[i] == 1)
                        {
                            car_m_arr2[k].lock_sf_arr[i] = 0;
                            car_m_arr2[k].status_arr[i] = 1;
                        }
                        if (car_1.status_arr[i] == 3)
                        {
                        } //   this isn't necessary with multi because that info is stored
                    }
                }
            }
        }

        // // confirm or abort based on locking information phase
        // // second send and receive cycle

        for (int k = 0; k < num_servers; k++)
        {
            int x = 0;
            if (car_m_arr2[k].num_commands > 0)
            {
                std::cout << write_car_status_multi(car_m_arr[k], -777) << "\n";
                int server_index = get_server_index(car_m_arr[k].key_arr[0]);
                std::cout << "now doing send and receive for this object!@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
                for (int idx = server_index; idx < server_index + redundancy; idx++)
                {

                    int curr_soc = client_sock_vec[idx % num_servers];

                    command_and_response_multi car_2 = car_m_arr2[k];

                    // serialize from car_1 and send
                    bzero(outgoing, 512);
                    std::cout << "Serializing" << std::endl;
                    serialize_multi(outgoing, &car_2, keylength);
                    sf = send(curr_soc, outgoing, sizeof(outgoing), 0); // send based on client_sock[relevant_index] - i
                    std::cout << "VVV Thread " << thread_id << " send s/f: " << sf << std::endl;
                    status1 = write_car_status_multi(car_2, 1); //, idx%num_servers);
                    std::cout << status1 << std::endl;
                    thread_file << status1 + "\n";

                    // receive and deserialize into car_2
                    bzero(response_buffer, 512);
                    sf = recv(curr_soc, response_buffer, sizeof(response_buffer), 0);
                    std::cout << "deserializing: " << std::endl;
                    deserialize_multi(response_buffer, &car_m_arr2[k], keylength);

                    std::cout << "VVV Thread " << thread_id << " receive s/f: " << sf << std::endl;
                    status2 = write_car_status_multi(car_m_arr2[k], 2); // , idx%num_servers);
                    std::cout << status2 << std::endl;
                    thread_file << status2 + "\n";

                    // int counter_unavail = 0;
                    // int counter_avail_success =0;
                    // int counter_avail_fail =0;
                    // command_and_response_single car_3 = car_2;
                    // command_and_response_single car_4 = make_rand_car_single(thread_id,ctr,keylength);
                    // if (car_m_arr2[k].lock_sf_arr[b]==1){
                    //     car_m_arr2[k].status = 6;
                    //     //     serialize_single(outgoing, &car_3, keylength);

                    //     for (int idx = server_index ; idx < server_index+redundancy; idx++){
                    //         int curr_soc = client_sock_vec[idx%num_servers];

                    //         sf = send(curr_soc, outgoing, sizeof(outgoing), 0);
                    //         std::cout<<"VVV Thread "<<thread_id<<" send s/f: "<<sf<<std::endl;
                    //         status3 = write_car_status_single(car_3, 3, idx%num_servers);
                    //         std::cout<<status3<<std::endl;
                    //         thread_file<<status3+"\n";

                    //         sf = recv( curr_soc , response_buffer, sizeof(response_buffer), 0);
                    //         deserialize_single(response_buffer, &car_4, keylength);
                    //         std::cout<<"VVV Thread "<<thread_id<<" receive s/f: "<<sf<<std::endl;
                    //         status4 = write_car_status_single(car_4, 4, idx%num_servers);
                    //         std::cout<<status4<<std::endl;
                    //         thread_file<<status4+"\n";

                    //         switch (car_4.status){
                    //             case 7: {counter_avail_success++;} break;
                    //             case 9: {counter_avail_fail++;} break;
                    //         }
                    //     }
                    //     if (!(counter_avail_fail == redundancy || counter_avail_success == redundancy)){
                    //         // must write this to file
                    //         std::string errorstring = "Server inconsistency! Error with servers.\nFail counter says: "+ std::to_string(counter_avail_fail)+"\nSuccess counter says: "+std::to_string(counter_avail_success)+"\nNumber servers: "+std::to_string(num_servers)+"\n";
                    //         std::string errorstring2 = "Status (should be 0 or 2): "+std::to_string(car_4.status)+"\n";
                    //         std::cout<<errorstring;
                    //         std::cout<<errorstring2;
                    //         thread_file<<errorstring;
                    //         thread_file<<errorstring2;
                    //         error_count++;
                    //         return 0;
                    //         // this operation technically failed
                    //     }
                    //     else{
                    //         // nothing
                    //     }

                    // }
                    // else{       // locks not available                // update the latest thing with the correct status id DENY/abort
                    //     car_3.status = 4;

                    //     serialize_single(outgoing, &car_3, keylength);

                    //     for (int idx = server_index ; idx < server_index+redundancy; idx++){
                    //         int curr_soc = client_sock_vec[idx%num_servers];

                    //         sf = send(curr_soc, outgoing, sizeof(outgoing), 0);
                    //         std::cout<<"VVV Thread "<<thread_id<<" send s/f: "<<sf<<std::endl;
                    //         status3 = write_car_status_single(car_3,3, idx%num_servers);
                    //         std::cout<<status3<<std::endl;
                    //         thread_file<<status3+"\n";

                    //         sf = recv( curr_soc , response_buffer, sizeof(response_buffer), 0);
                    //         deserialize_single(response_buffer, &car_4, keylength);
                    //         std::cout<<"VVV Thread "<<thread_id<<" receive s/f: "<<sf<<std::endl;
                    //         status4 = write_car_status_single(car_4, 4, idx%num_servers);
                    //         std::cout<<status4<<std::endl;
                    //         thread_file<<status4+"\n";

                    //         if (car_4.status == 5){
                    //             counter_unavail++;
                    //         }
                    //     }
                    //     if (counter_unavail != redundancy){
                    //         // need to write this to the file as well
                    //         std::string errorstring = "TTTTTTTT: Server inconsistency!\nThey haven't all acknowledged the abortion: "+ std::to_string(counter_unavail)+"\n Acknowledge the abortion but there are "+std::to_string(num_servers)+" total servers.\n";
                    //         std::cout<<errorstring;
                    //         thread_file<<errorstring;
                    //         error_count++;
                    //         return 0;
                    //     }
                    //     else{
                    //         // this operation technically succeeded, although you could not get locks on the whole set of servers
                    //     }
                }
            }
        }
        std::cout << "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\n";
        ctr++;
    }

    auto opend = std::chrono::steady_clock::now();
    int op_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(opend - opstart).count();
    cum_optime_us = cum_optime_us + op_duration_us;

    for (int i = 0; i < num_servers; i++)
    {
        std::cout << "Thread " << thread_id << " closing socket with ID " << client_sock_vec[i] << std::endl;
        close(client_sock_vec[i]);
    }

    thread_file << "Key_size: " << keylength << "\n";                           //delta (sec)
    thread_file << "Cumulative_operation_time_(us): " << cum_optime_us << "\n"; //delta (sec)
    float cum_optime_s = float(float(cum_optime_us) / 1000000.0);

    thread_file << "Average_latency_(s): " << cum_optime_s / float(ctr) << "\n";
    thread_file << "Cumulative_operation_time_(s): " << cum_optime_s << "\n"; //delta (sec)
    thread_file << "Average_throughput_(ops/sec): " << float(ctr) / cum_optime_s << "\n";
    thread_file << "Total_ops: " << ctr << "\n";

    //    thread_file << ;
    //    thread_file << // total ops
    //    thread_file << // get lock failure (ops)
    //    thread_file << // get op success
    //    thread_file << // get op failure
    //    thread_file << // put lock failure (ops)
    //    thread_file << // put op success
    //    thread_file << // put  op failure
    //    thread_file << // get lock failure (latency - sec)
    //    thread_file << // get op success latency
    //    thread_file << // get op failure latency
    //    thread_file << // put lock failure latency
    //    thread_file << // put op success latency
    //    thread_file << // put op failure latency
    //    thread_file << // error total (ops)
    //    thread_file << // error latency

    thread_file.close();

    return 0;
}


int main(int argc, char *argv[])
{
    // clock_t begin = clock();
    auto binstart = std::chrono::steady_clock::now();
    int initialize_amount = 0;
    total_keyrange = 10000;
    int option;
    std::string filename;
    int keylength = 1;
    while ((option = getopt(argc, argv, "k:s:f:r:")) != -1)
    {
        switch (option)
        {
        case 'k':
            keylength = atof(optarg);
            std::cout << "Key size is: " << keylength << std::endl;
            break;
        case 'f':
            filename = optarg;
            std::cout << "IP file name is: " << filename << std::endl;
            break;
        case 'r':
            total_keyrange = atof(optarg);
            std::cout << "Key range is: " << total_keyrange << std::endl;
        }
    }

    std::ifstream file_(filename);
    std::string line_;
    std::vector<std::string> IP_addresses;
    num_servers = 0;
    int counter = 0;
    if (file_.is_open())
    {
        while (getline(file_, line_))
        {
            num_servers++;
            IP_addresses.push_back(line_);
            std::cout << "IP address: " << IP_addresses[counter] << std::endl;
            counter++;
        }
    }
    else
    {
        std::cout << "need filename" << std::endl;
        return -1;
    }
    std::cout << "Begin with the multithreading" << std::endl;

    std::vector<std::thread> threads;
    for (int i = 1; i <= 8; i++)
    {
        threads.emplace_back(std::thread(send_and_receive_messages, IP_addresses, keylength, i));
    }
    for (auto &th : threads)
    {
        th.join();
    }

    auto binend = std::chrono::steady_clock::now();

    // std::ofstream thread_file;
    // std::string output_name = "test_results/mainthread.txt";
    // thread_file.open(output_name);
    // thread_file << "Elapsed time in milliseconds : " << std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(binend - binstart).count()) <<"\n";
    // int op_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(opend - opstart).count();

    return 0;
}

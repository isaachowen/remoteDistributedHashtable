// talker aka client

#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define PORT 8080

using namespace std;

int main(int argc, char const *argv[])
{


//    cout << "Hello world!" << endl;
//    return 0;


// Client side C/C++ program to demonstrate Socket programming

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *first = "first message";
    char buffer[1024] = {0};
    //if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    // preparing information about the server we're going to reach out to
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
// what about serv_addr.sin_addr


    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) // pass in the address of serv_addr.sin_addr , gonna modify it
    {   // pton converts an ip address to a string
        // pton stores that ip address into serv_addr
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)    // client uses connect
    {
            printf("\nConnection Failed \n");
            return -1;
    }

    int i = 0;
    while (i<100){
        //sleep(1);
        cout<<"bla"<<endl;
        i++;
        cout<<i<<endl;

        send(sock , first , strlen(first) , 0 );
        printf(first);
        printf("\n");
        send(sock,"second message", sizeof("second message"),0);
        printf("I've sent the second message \n");
        valread = read( sock , buffer, 1024);
        printf("response: ");
        printf("%s\n",buffer );

    }
    return 0;
}

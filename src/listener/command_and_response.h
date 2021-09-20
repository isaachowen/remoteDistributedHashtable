#ifndef COMMAND_AND_RESPONSE_H
#define COMMAND_AND_RESPONSE_H

template <typename T>
struct command_and_response{
    int command;
    int key;
    T value;
    int result;
};

/*
template <typename T>
struct response{
    int command;
    int key;
    T value;
    int result;
}
*/
#endif // COMMAND_AND_RESPONSE_H

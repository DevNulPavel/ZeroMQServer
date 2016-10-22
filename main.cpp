#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <functional>
#include <zmq.h>



//  Receive ZeroMQ string from socket and convert into C string
//  Chops string at 255 chars, if it's longer
char* s_recv (void *socket) {
    char buffer [256];
    int size = zmq_recv (socket, buffer, 255, 0);
    if (size == -1)
        return NULL;
    if (size > 255)
        size = 255;
    buffer [size] = 0;
    return strdup(buffer);
}

void s_send (void* socket, const char* message) {
    size_t length = strlen(message);    // TODO: нужно ли вычислять размер?
    zmq_send(socket, message, length, 0);
}

int simpleServer(){
    //  Socket to talk to clients
    void* context = zmq_ctx_new ();
    zmq_ctx_set (context, ZMQ_IO_THREADS, 4);   // 4ре потока для обработки

    void* responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, "tcp://*:5555");
    assert (rc == 0);
    
    while (1) {
        char buffer [10];
        zmq_recv (responder, buffer, 10, 0);
        printf ("Received Hello\n");
        //sleep (1);          //  Do some 'work'
        zmq_send (responder, "World", 5, 0);
    }
}

// отправляет всем клиентам данные
int pushServer(){
    //  Prepare our context and publisher
    void* context = zmq_ctx_new();
    void* publisher = zmq_socket(context, ZMQ_PUB);
    int rc = zmq_bind (publisher, "tcp://*:5555");
    assert(rc == 0);
    
    //  Initialize random number generator
    srand((unsigned)time(NULL));
    while (1) {
        //  Get values that will fool the boss
        int zipcode, temperature, relhumidity;
        zipcode     = rand() % 100000;
        temperature = rand() % 215 - 80;
        relhumidity = rand() % 50 + 10;
        
        //  Send message to all subscribers
        char update [20];
        sprintf (update, "%05d %d %d", zipcode, temperature, relhumidity);
        s_send(publisher, update);
    }
    zmq_close (publisher);
    zmq_ctx_destroy (context);
    return 0;
}


void* workerTask(void* context){
    //  Сокет для работы с диспетчером
    void* receiver = zmq_socket(context, ZMQ_REP);
    zmq_connect (receiver, "inproc://workers");

    while (1) {
        char* string = s_recv(receiver);
        printf ("Received request: [%s]\n", string);
        free (string);
        //  Выполняем задачу
        sleep (1);
        // отвечаем клиенту
        s_send (receiver, "World");
    }
    zmq_close (receiver);
    return nullptr;
}

int simpleMultThreaded(void) {

    void* context = zmq_ctx_new();

    //  Сокет для связи с клиентами
    void* clients = zmq_socket (context, ZMQ_ROUTER);
    zmq_bind (clients, "tcp://*:5555");

    //  Сокет для связи с обработчиками
    void* workers = zmq_socket (context, ZMQ_DEALER);
    zmq_bind (workers, "inproc://workers");

    //  Запускаем пул воркеров
    for (int thread_nbr = 0; thread_nbr < 5; thread_nbr++) {
        pthread_t worker;
        pthread_create(&worker, NULL, workerTask, context);
    }

    //  Connect work threads to client threads via a queue proxy
    zmq_proxy (clients, workers, NULL);

    //  We never get here, but clean up anyhow
    zmq_close (clients);
    zmq_close (workers);
    zmq_ctx_destroy (context);
    return 0;
}

int main (void)
{
    return simpleMultThreaded();
}

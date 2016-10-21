//  Hello World server

#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>


//  Receive ZeroMQ string from socket and convert into C string
//  Chops string at 255 chars, if it's longer
static char* s_recv (void *socket) {
    char buffer [256];
    int size = zmq_recv (socket, buffer, 255, 0);
    if (size == -1)
        return NULL;
    if (size > 255)
        size = 255;
    buffer [size] = 0;
    return strdup (buffer);
}

static void s_send (void* socket, const char* message) {
    size_t length = strlen(message);    // TODO: нужно ли вычислять размер?
    zmq_send(socket, message, length, 0);
}

int simpleServer(){
    //  Socket to talk to clients
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    int rc = zmq_bind (responder, "tcp://*:5555");
    assert (rc == 0);
    
    while (1) {
        char buffer [10];
        zmq_recv (responder, buffer, 10, 0);
        printf ("Received Hello\n");
        sleep (1);          //  Do some 'work'
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
    srandom((unsigned)time(NULL));
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

int main (void)
{
    return pushServer();
}

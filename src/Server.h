#ifndef SERVER_H
#define SERVER_H

class Server {
public:
    void send_SERVER_OBSERVER_FLY(int x, int y, int speed) {}
    void send_SERVER_OBJECT_FLY(int x, int y, int speed, bool walls, int obj_id) {}
    void send_SERVER_OBJECT_FALL(int obj_id) {}
};

#endif

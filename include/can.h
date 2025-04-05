#include <FlexCAN_T4.h>

#ifndef CAN_H
#define CAN_H

class NextionInterface;

class CanInterface{
public:
    CanInterface();

    static bool canActive;

    static FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can0;

    static bool init();

    static void print_can_sniff(const CAN_message_t &msg);

    static void receive_can_updates(const CAN_message_t &msg);

    static void send_shift(const bool up, const bool down);

    static void task();
};

#endif //CAN_
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

//#include "cs104_slave.h"
#include "cs104_connection.h"  //as server and master station

#include "hal_thread.h"
#include "hal_time.h"

static bool running = true;

void
sigint_handler(int signalId)
{
    running = false;
}

void
printCP56Time2a(CP56Time2a time)
{
    printf("%02i:%02i:%02i %02i/%02i/%04i", CP56Time2a_getHour(time),
                             CP56Time2a_getMinute(time),
                             CP56Time2a_getSecond(time),
                             CP56Time2a_getDayOfMonth(time),
                             CP56Time2a_getMonth(time),
                             CP56Time2a_getYear(time) + 2000);
}

static bool
connectionRequestHandler_mStation(void* parameter, const char* ipAddress, int port)
{
    printf("New connection from %s:%d\n", ipAddress, port);

#if 0
    if (strcmp(ipAddress, "127.0.0.1") == 0) {
        printf("Accept connection\n");
        return true;
    }
    else {
        printf("Deny connection\n");
        return false;
    }
#else
    return true;
#endif
}

int
main(int argc, char** argv)
{
    /* Add Ctrl-C handler */
    signal(SIGINT, sigint_handler);


    CS104_Connection_mStation con_mStation = sCS104_Connection_mStation_create(100, 100);
    CS104_Connection_setLocalAddress(con_mStation,"127.0.0.1");
    CS104_Connection_setLocalPort(con_mStation,4001);//2404
    CS104_Connection_setConnectionRequestHandler(con_mStation, connectionRequestHandler_mStation, NULL);
    CS104_Connection_start_mStation(con_mStation);

    if(CS104_Connection_isRunning_mStation(con_mStation) == false) {
        printf("Starting server failed!\n");
        goto exit_program;
    }

    int16_t scaledValue = 0;

    while (running) {

        Thread_sleep(1000);

        if(scaledValue >= 60*60)
        {
            scaledValue = 1;
        }
        else
            scaledValue++;
        if( scaledValue%60 == 0 )
            printf("time count: %d\n",scaledValue);
//        //归一化值上送
//        CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_PERIODIC, 0, 1, false, false);

//        InformationObject io = (InformationObject) MeasuredValueScaled_create(NULL, 110, scaledValue, IEC60870_QUALITY_GOOD);

//        scaledValue++;

//        CS101_ASDU_addInformationObject(newAsdu, io);

//        InformationObject_destroy(io);

//        /* Add ASDU to slave event queue - don't release the ASDU afterwards!
//         * The ASDU will be released by the Slave instance when the ASDU
//         * has been sent.
//         */
//        CS104_Slave_enqueueASDU(slave, newAsdu);

//        CS101_ASDU_destroy(newAsdu);
    }

    CS104_Connection_stop_mStation(con_mStation);

exit_program:
    CS104_Connection_destroy_mStation(con_mStation);

    Thread_sleep(500);
}

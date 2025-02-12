#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "cs104_slave.h"

#include "hal_thread.h"
#include "hal_time.h"

//для рандома
#include <time.h>
#include <stdlib.h>

static bool running = true;

void sigint_handler(int signalId)
{
    running = false;
}

static bool
connectionRequestHandler(void *parameter, const char *ipAddress)
{
    printf("New connection request from %s\n", ipAddress);
    return true;
}

static void
connectionEventHandler(void *parameter, IMasterConnection con, CS104_PeerConnectionEvent event)
{
    if (event == CS104_CON_EVENT_CONNECTION_OPENED)
    {
        printf("Connection opened (%p)\n", con);
    }
    else if (event == CS104_CON_EVENT_CONNECTION_CLOSED)
    {
        printf("Connection closed (%p)\n", con);
    }
    else if (event == CS104_CON_EVENT_ACTIVATED)
    {
        printf("Connection activated (%p)\n", con);
    }
    else if (event == CS104_CON_EVENT_DEACTIVATED)
    {
        printf("Connection deactivated (%p)\n", con);
    }
}

//функция генерации данных для перменных
float greenhouse_variables(float value, float top, float down, float step)
{
    srand(time(NULL)); // Initialization, should only be called once.
    if (rand() % 2)
    {
        value += step;
    }
    else
    {
        value -= step;
    }

   if(value >= top)
   {
       value -= step;
   }

   if(value <= down)
   {
       value += step;
   }

    return value;

    
}

int main(int argc, char **argv)
{
    /* Add Ctrl-C handler */
    signal(SIGINT, sigint_handler);

    /* create a new slave/server instance with default connection parameters and
     * default message queue size */
    CS104_Slave slave = CS104_Slave_create(10, 10);

    CS104_Slave_setLocalAddress(slave, "0.0.0.0");

    /* Set mode to a single redundancy group
     * NOTE: library has to be compiled with CONFIG_CS104_SUPPORT_SERVER_MODE_SINGLE_REDUNDANCY_GROUP enabled (=1)
     */
    CS104_Slave_setServerMode(slave, CS104_MODE_SINGLE_REDUNDANCY_GROUP);

    /* /* получить параметры подключения - они нужны нам для создания правильных ASDUS -
     * вы также можете изменить параметры здесь, когда параметры по умолчанию не должны использоваться */
    CS101_AppLayerParameters alParams = CS104_Slave_getAppLayerParameters(slave);

    /* установить обработчик для обработки запросов на подключение. New connection request from [ip]*/
    CS104_Slave_setConnectionRequestHandler(slave, connectionRequestHandler, NULL);

    /* установить обработчик для отслеживания событий подключения. activated/closed */
    CS104_Slave_setConnectionEventHandler(slave, connectionEventHandler, NULL);

    CS104_Slave_start(slave);

    if (CS104_Slave_isRunning(slave) == false)
    {
        printf("Starting server failed!\n");
        goto exit_program;
    }

    int16_t scaledValue = 0;
    float scaledValue1 = 1000.1;

    float var[3][4] = {{30, 25, 35, 0.01}, {60, 30, 90, 0.05}, {760, 650, 1030, 0.5}}; //{{переменная, нижний_край, верхний_край, шаг_изменения}}

    while (running)
    {

        Thread_sleep(5000); // время задержки отправки данных

        for(int i = 0; i < 3; i++)
        {
            CS101_ASDU newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_PERIODIC, 0, 1, false, false);
            var[i][0] = greenhouse_variables(var[i][0], var[i][1], var[i][2], var[i][3]);

            InformationObject io = (InformationObject)ParameterFloatValue_create(NULL, 110 + i, var[i][0], IEC60870_QUALITY_GOOD); // тип данных float

            CS101_ASDU_addInformationObject(newAsdu, io);

            InformationObject_destroy(io);

            /* Add ASDU to slave event queue */
            CS104_Slave_enqueueASDU(slave, newAsdu);

            CS101_ASDU_destroy(newAsdu);
        }

        
    }

    CS104_Slave_stop(slave);

exit_program:
    CS104_Slave_destroy(slave);

    Thread_sleep(10);
}

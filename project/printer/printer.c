#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "printer.h"


WorkInfo produce_task(const sheet_t color)
{
    WorkInfo task;
    task.id = rand() % (ID_MAX - ID_MIN + 1) + ID_MIN;
    task.pages = rand() % (PG_MAX - PG_MIN + 1) + PG_MIN;
    task.color = color;
    
    return task;
}

void print_task(const sheet_t color, const int n_pages, int *pages_available)
{
    int time;
    char *print_type;

    if (color == BN)
    {
        time = 1;  // 1 pg/sec
        print_type = "BN";
    }
    else
    {
        time = 4;  // 1/4 pg/sec
        print_type = "RGB";
    }


    for(int i=0; i<n_pages; i++)
    {
        printf("[%s] Printing %d / %d pages and %d pages available\n", print_type, i, n_pages, *pages_available);
        if(*pages_available > 0)
        {   
            usleep(time*100000);
            *pages_available = *pages_available - 1;
        }
        else
        {
            printf("[%s] Need to fill paper box, 1 min waiting\n", print_type);
            usleep(10000000);
            *pages_available = PAGES_PRINTER;
            printf("[%s] Filled box\n", print_type);
        }
        
    }
}

void show_task(const WorkInfo *task)
{
    printf("--- TASK --\n");
    printf("ID number: %03d\n", task->id);
    switch (task->color)
    {
        case BN:
            printf("Color type: BN\n");
            break;

        case RGB:
            printf("Color type: RGB\n");
            break;
        
        default:
            printf("Color type: IND\n");
            break;
    }
    printf("Number of pages: %d\n\n", task->pages);
}


void *bn_tasks(void *ptr)
{
    //Casting
    Printer *bn_printer_machine = (Printer *)ptr;

    for(int i=0; i<N1; i++)
    {
        //Produce task
        WorkInfo print_sheets = produce_task(BN);

        //Put the task in the buffer
        put_item(print_sheets, &bn_printer_machine->queue);

        //Introduce task in the buffer
        show_task(&print_sheets);

        sleep(5);
    }
    
    pthread_exit(NULL);
}

void *rgb_tasks(void *ptr)
{
    //Casting
    Printer *rgb_printer_machine = (Printer *)ptr;

    for(int i=0; i<N2; i++)
    {
        //Produce task
        WorkInfo print_sheets = produce_task(RGB);

        //Put the task in the buffer
        put_item(print_sheets, &rgb_printer_machine->queue);

        //Introduce task in the buffer
        show_task(&print_sheets);

        sleep(5);
    }
    
    pthread_exit(NULL);
}


//TODO
void *ind_tasks(void *ptr)
{
    //Casting
    PrinterSystem *printer_machines = (PrinterSystem *)ptr;

    for(int i=0; i<N3; i++)
    {
        //Produce task
        WorkInfo print_sheets = produce_task(IND);

        //Get queue time of both
        //put_item_ind(print_sheets, &printer_machines->bn_printer_machine.queue, &printer_machines->rgb_printer_machine.queue);
        //Introduce task in the buffer
        show_task(&print_sheets);
        sleep(5);
    }

    pthread_exit(NULL);
}


//Consumers (PRINTERS)
void *bn_printer(void *ptr)
{
    //Casting
    Printer *bn_printer_machine = (Printer *)ptr;

    //Cambiar while(true)!!!
    while(true)
    {
        WorkInfo data;
        get_item(&data, &bn_printer_machine->queue);

        printf("\n[BN] Printing task with ID [%03d] collected\n Number waiting: %d\n\n", data.id, get_counter(&bn_printer_machine->queue));

        long t0 = getCurrentMicroseconds();
        print_task(BN, data.pages, &bn_printer_machine->pages_available);
        long printing_time = getCurrentMicroseconds() - t0;
        printf("\n[BN] Finished printing task with ID [%03d], time taken: %ld us, pages available: %d", data.id, printing_time, bn_printer_machine->pages_available);
    }

    pthread_exit(NULL);
}

void *rgb_printer(void *ptr)
{
    //Casting
    Printer *rgb_printer_machine = (Printer *)ptr;

    //Cambiar while(true)!!!
    while(true)
    {
        WorkInfo data;
        get_item(&data, &rgb_printer_machine->queue);

        printf("\n[RGB] Printing task with ID [%03d] collected\n Number waiting: %d\n\n", data.id, get_counter(&rgb_printer_machine->queue));

        long t0 = getCurrentMicroseconds();
        print_task(RGB, data.pages, &rgb_printer_machine->pages_available);
        long printing_time = getCurrentMicroseconds() - t0;

        printf("\n[RGB] Finished printing task with ID [%03d], time taken: %ld us, pages available: %d", data.id, printing_time, rgb_printer_machine->pages_available);
    }

    pthread_exit(NULL);
}


void init_printer_machine(Printer *printer_machine, sheet_t color_type)
{
    printer_machine->type = color_type;
    printer_machine->pages_available = PAGES_PRINTER;
    inicializar_buffer(&printer_machine->queue);
}

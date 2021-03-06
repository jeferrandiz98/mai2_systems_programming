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

long print_task(const sheet_t color, const int n_pages, Printer *printer, const int speed)
{
    long t0 = getCurrentMicroseconds();
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

    //Print all pages
    for(int i=0; i<n_pages; i++)
    {
        printf("[%s] Printing %d / %d pages and %d pages available\n", print_type, i, n_pages, printer->pages_available);
        //Check if there are pages available
        if(printer->pages_available > 0)
        {   
            usleep(time*1000000/speed);
            printer->pages_available--;
        }
        else
        {
            printf("[%s] Need to fill paper box, 1 min waiting\n", print_type);
            //Wait one minut
            usleep(60000000/speed);
            printer->pages_available = PAGES_PRINTER;
            printer->n_paper_boxes++;
            printf("[%s] Filled box\n", print_type);
        }
        
    }
    return getCurrentMicroseconds() - t0;
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
    PrinterSystem *printer_machines = (PrinterSystem *)ptr;

    for(int i=0; i<N_PRINTS; i++)
    {
        //Produce task
        WorkInfo print_sheets = produce_task(BN);

        //Put the task in the buffer BN
        put_item(print_sheets, &printer_machines->bn_printer_machine.queue);

        //Introduce task in the buffer
        show_task(&print_sheets);

        //usleep(10000000/SPEED);
    }
    
    pthread_exit(NULL);
}

void *rgb_tasks(void *ptr)
{
    //Casting
    Printer *rgb_printer_machine = (Printer *)ptr;

    for(int i=0; i<N_PRINTS; i++)
    {
        //Produce task
        WorkInfo print_sheets = produce_task(RGB);

        //Put the task in the buffer RGB
        put_item(print_sheets, &rgb_printer_machine->queue);

        //Introduce task in the buffer
        show_task(&print_sheets);

        //usleep(10000000/SPEED);
    }

    pthread_exit(NULL);
}


void *ind_tasks(void *ptr)
{
    //Casting
    PrinterSystem *printer_machines = (PrinterSystem *)ptr;

    for(int i=0; i<N_PRINTS; i++)
    {
        //Produce task
        WorkInfo print_sheets = produce_task(IND);

        //Introduce task in the buffer (RGB or BN)
        put_item_ind(print_sheets, &printer_machines->bn_printer_machine.queue, &printer_machines->rgb_printer_machine.queue);
        
        show_task(&print_sheets);
        //usleep(10000000/SPEED);
    }

    pthread_exit(NULL);
}


//Consumers (PRINTERS)
void *bn_printer(void *ptr)
{
    //Casting
    PrinterSystem *printer_machines = (PrinterSystem *)ptr;
    Printer *bn_printer_machine = &printer_machines->bn_printer_machine;
    Printer *rgb_printer_machine = &printer_machines->rgb_printer_machine;
    

    while(true)
    {
        WorkInfo data;
        get_item(&data, &bn_printer_machine->queue);
        
        printf("\n[BN] Printing task with ID [%03d] collected\n Number waiting: %d\n\n", data.id, get_counter(&bn_printer_machine->queue));

        long time_taken = print_task(BN, data.pages, bn_printer_machine, printer_machines->speed);

        printf("\n[BN] Finished printing task with ID [%03d], time taken: %ld us, pages available: %d\n", data.id, time_taken, bn_printer_machine->pages_available);
        //Save into history
        bn_printer_machine->history[bn_printer_machine->n_history_saved] = data;
        bn_printer_machine->n_history_saved++;
    }
}

void *rgb_printer(void *ptr)
{
    PrinterSystem *printer_machines = (PrinterSystem *)ptr;
    Printer *bn_printer_machine = &printer_machines->bn_printer_machine;
    Printer *rgb_printer_machine = &printer_machines->rgb_printer_machine;

    while(true)
    {
        WorkInfo data;
        
        //Si el buffer de la impresora RGB está vacío
        if(is_empty(&rgb_printer_machine->queue)) {
            //Procesa datos de la cola blanco y negro
            printf("[RGB] Buffer vacío, procesando dato de la cola BN");
            get_item(&data, &bn_printer_machine->queue);
        }
        else {
            //Si no, procesa datos de su propia cola, RGB
            get_item(&data, &rgb_printer_machine->queue);
        }

        printf("\n[RGB] Printing task with ID [%03d] collected\n Number waiting: %d\n\n", data.id, get_counter(&rgb_printer_machine->queue));

        long time_taken = print_task(RGB, data.pages, rgb_printer_machine, printer_machines->speed);

        printf("\n[RGB] Finished printing task with ID [%03d], time taken: %ld us, pages available: %d\n", data.id, time_taken, rgb_printer_machine->pages_available);
        
        //Save into history
        rgb_printer_machine->history[rgb_printer_machine->n_history_saved] = data;
        rgb_printer_machine->n_history_saved++;

    }
}


void init_printer_machine(Printer *printer_machine, sheet_t color_type)
{
    printer_machine->type = color_type;
    printer_machine->pages_available = PAGES_PRINTER;
    printer_machine->n_history_saved = 0;
    printer_machine->n_paper_boxes = 0;
    inicializar_buffer(&printer_machine->queue);
    srand(time(NULL));
}

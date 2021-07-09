#include <stdio.h>

#include "fifo.h"
#include "my_list.h"

#define SHOW_MONITOR

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

#ifdef SHOW_MONITOR
#define MONITOR_PRINT(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( false )
#else
#define MONITOR_PRINT(...) do{ } while ( false )
#endif

typedef unsigned long uint32_t;
typedef unsigned int uint16_t;


#define   DEFAULT_SHOPPING_CHECKED_TIME       (50) // 5 second

#define  DEFAULT_CUSTOMER_ENTER_PERIOD         (2) //second
#define  NUMBER_OF_CART              20
#define  NUMBER_OF_SCANNER           10

#define  MAX_CASHIER_QUEUE          (3)
#define  MAX_SCANNER_QUEUE          (2)
#define  MAX_TOTAL_QUEUE            (MAX_CASHIER_QUEUE + MAX_SCANNER_QUEUE)

#define  DEFAULT_CASHIER_CHECKOUT_PERIOD       (50)
#define  DEFAULT_SCANNER_CHECKOUT_PERIOD       (20)
#define  DEFAULT_SHOPPING_PERIOD               (50)
#define  DEFAULT_RETURN_CART_PERIOD            (20)


#define MAX_ENTRY_COUNTER      20  // 20 * 100ms = 2000ms = 2s

static sem_t cart_sem;
static sem_t scanner_sem;

static uint32_t  customer_enter_time = DEFAULT_CUSTOMER_ENTER_PERIOD;

static uint32_t  id_cnt = 0;
static uint32_t reject_cnt = 0;
// Declare list object
LIST_HEAD(return_cart_list);
LIST_HEAD(shopping_list);

list_object_struct_t *return_list_head = NULL;
list_object_struct_t *shopping_list_head = NULL;

// Declare FIFO object
char manual_fifo_buffer[MAX_CASHIER_QUEUE][FIFO_MAX]; 
char scanner_fifo_buffer[MAX_SCANNER_QUEUE][FIFO_MAX]; 
struct fifo_obj manual_checkout_fifo[MAX_CASHIER_QUEUE];
struct fifo_obj scanner_checkout_fifo[MAX_SCANNER_QUEUE];


void scanner_checkout_add_to_queue(char id);
void manual_checkout_add_to_queue(char id);

void *thread_shopping_tracking(void *arg);
void *scanner_checkout_thread(void *arg);
void *manual_checkout_thread(void *arg);
void *thread_return_cart(void *arg);

void *thread_monitor_run(void *arg);


static int get_random_check_during_shopping(int max_num);
static  uint32_t get_random_enter_time();
static  uint32_t get_random_shopping_time();
static  uint32_t get_random_cashier_checkout_time();
static  uint32_t get_random_scanner_checkout_time();

int main()
{
    int res;
    int i = 0;

    pthread_t shopping_thread;
    pthread_t cashier_thread[3];
    pthread_t scanner_thread[2];
    pthread_t return_thread;
    pthread_t monitor_thread;

    void *thread_result;

    // ================= INIT SEMAPHORE ==============
    res = sem_init(&cart_sem, 0, NUMBER_OF_CART);
    if (res != 0) {
        DEBUG_PRINT("cart_sem initialization failed");
        exit(EXIT_FAILURE);
    }

    res = sem_init(&scanner_sem, 0, NUMBER_OF_SCANNER);
    if (res != 0) {
        DEBUG_PRINT("scanner_sem initialization failed");
        exit(EXIT_FAILURE);
    }

    //=================Init FIFO =======================

    for (i = 0; i < MAX_CASHIER_QUEUE; i++)
    {
        fifo_init(&manual_checkout_fifo[i]);
        manual_checkout_fifo[i].mem_pool = (char*)(&manual_fifo_buffer[i]);
    }

    for (i = 0; i < MAX_SCANNER_QUEUE; i++)
    {
        fifo_init(&scanner_checkout_fifo[i]);
        scanner_checkout_fifo[i].mem_pool = (char*)(&scanner_fifo_buffer[i]);
    }
    //=================Init link list===================
    // Init return cart list
    return_list_head = create_list(&return_cart_list, 1, DEFAULT_RETURN_CART_PERIOD);
    if (return_list_head == NULL)
    {
        DEBUG_PRINT("Init list failed\n");
        exit(EXIT_FAILURE);
    }
    shopping_list_head = create_list(&shopping_list, 2, DEFAULT_SHOPPING_PERIOD);
    if (shopping_list_head == NULL)
    {
        DEBUG_PRINT("Init list failed\n");
        exit(EXIT_FAILURE);
    }
    //=================Init THREAD=======================
     
    res = pthread_create(&monitor_thread, NULL, thread_monitor_run, (void *)NULL);
    if (res != 0)
    {
        DEBUG_PRINT("Thread creation failed");
        exit(EXIT_FAILURE);
    }
   

    res = pthread_create(&shopping_thread, NULL,  thread_shopping_tracking, (void *)NULL);
    if (res != 0)
    {
        DEBUG_PRINT("Thread creation failed");
        exit(EXIT_FAILURE);
    }


    res = pthread_create(&return_thread, NULL, thread_return_cart, (void *)NULL);
    if (res != 0)
    {
        DEBUG_PRINT("Thread creation failed");
        exit(EXIT_FAILURE);
    }

 #if 1
    for (i = 0; i < 3; i++)
    {
        res = pthread_create(&cashier_thread[i], NULL, manual_checkout_thread, (void *)&manual_checkout_fifo[i]);
        if (res != 0)
        {
            DEBUG_PRINT("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

  
    for (i = 0; i < 2; i++)
    {
        res = pthread_create(&scanner_thread[i], NULL, scanner_checkout_thread, (void *)&scanner_checkout_fifo[i]);
        if (res != 0)
        {
            DEBUG_PRINT("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }
    #endif

    // Waiting all thread finish
    DEBUG_PRINT("Waiting for thread to finish...\n");
    pthread_join(shopping_thread, &thread_result);
    pthread_join(return_thread, &thread_result);
    pthread_join(monitor_thread, &thread_result);
    for (i = 0; i < 3; i++)
    {
        pthread_join(cashier_thread[i], &thread_result);
    }

    for (i = 0; i < 2; i++)
    {
        pthread_join(scanner_thread[i], &thread_result);
    }

    // Destroy resources
    sem_destroy(&cart_sem);
    sem_destroy(&scanner_sem);

    DEBUG_PRINT("Thread joined, it returned %s\n", (char *)thread_result);

    exit(EXIT_SUCCESS);
}

/*================================PRODUCER======================================*/
/**

1. Every 2 seconds a customer arrives and takes a cart to go shopping, there are 20
carts, if no cart is available, the customer goes back home empty handed.

2. About half the customers take a handheld scanner with them, there are 10 handheld
scanners, if none are available, the customer does the shopping the normal way.

3. Shopping (and if applicable scanning with the handheld scanner) takes around 5
seconds.

4. Customers without a handheld scanner go to one of three cashiers
*/

void sig_customer_enter_timer(int signum) 
{
    bool has_canner = false;

//  DEBUG_PRINT("Customer enters\n");

    if (sem_trywait(&cart_sem) < 0)
    {
        DEBUG_PRINT("Empty cart, go backhome!!\r\n");
        reject_cnt++;
    }
    
    // Add to shopping queue
    /* 
    About half the customers take a handheld scanner with them, there are 10 handheld
    scanners
    */

    has_canner = (sem_trywait(&scanner_sem) >= 0)?1:0; 

    // DEBUG_PRINT("Customer %d - scanner %d\n", id_cnt, has_canner);
    add_to_list_shopping(shopping_list_head, id_cnt, has_canner, get_random_shopping_time());
    id_cnt++;
  
    alarm(get_random_enter_time());
}

#define   CASHIER_QUEUE_LEN_THRESHOLD       5

void  customer_prepare_checkout(customer_info_obj_t *obj)
{
    int i = 0;
    bool found = false;

    DEBUG_PRINT("Customer %d need checkout, with scanner = %d\r\n", obj->id, obj->has_scanner);

    //TODO: choose cashier or scanner queue
    if (obj->has_scanner)
    {
        scanner_checkout_add_to_queue(obj->id);
    }else 
    {
        manual_checkout_add_to_queue(obj->id);
    }
}

void *thread_shopping_tracking(void *arg)
{
    bool run = true;
    int  index = 0;
    customer_info_obj_t *curr_cust_obj = NULL;

    DEBUG_PRINT("THREAD shopping is running. Argument was %s\n", (char *)arg);
    signal(SIGALRM,sig_customer_enter_timer);

    alarm(get_random_enter_time());
    while(run)
    {
        curr_cust_obj = list_count_down(&shopping_list, shopping_list_head);
        if ( curr_cust_obj != NULL)
        {   
            // DEBUG_PRINT("==>customer need checkout\n");
            customer_prepare_checkout(curr_cust_obj);
            delete_from_list(&shopping_list, shopping_list_head, curr_cust_obj);        
        }

        usleep(100*1000); // 100ms
    }

    pthread_exit("THREAD shopping is exited");
}

/*================================RETURN CART======================================*/
/* 
After paying, every customer takes 2 seconds to bring the groceries to their bicycle or
car and return the cart, which then becomes available for new customers, and then
they go back home.
*/

void *thread_return_cart(void *arg)
{
    bool run = true;
    int  finish_id = 0;
    customer_info_obj_t *obj = NULL;

    DEBUG_PRINT("THREAD return cart is running. Argument was %s\n", (char *)arg);

    while(run)
    {
        obj = list_count_down(&return_cart_list, return_list_head);
        if ( obj != NULL)
        {
            DEBUG_PRINT("==>ID %d return cart\n", obj->id);
            // Remove finish id from list
            delete_from_list(&return_cart_list, return_list_head, obj);
            sem_post(&cart_sem);
        }
        usleep(100*1000); // 100ms
    }

    pthread_exit("THREAD return cart is exited");
}


//===========================CASHIER CHECKOUT==========================================
/*
- if there are already customers waiting in line for a cashier, 
  they choose the cashier with the least (or none) customers in line and join the end of that queue.
*/

 int findMinOfArray(int a[],int n)
 {
 	int min,i;
    int idx = 0;

 	min = a[0];
     
    for(i = 1; i < n; i++)
    {
         if(min > a[i])
         {
		  min = a[i];  
          idx = i; 
         }
    }

    return idx;    
 }

void manual_checkout_add_to_queue(char id)
{
    int  queue_items[3] = {0};
    int i = 0;

    for (i = 0; i < 3; i++)
    {
        queue_items[i] = manual_checkout_fifo[i].fifo_head;
    }

    // Find cashier with the least or none customer
    i = findMinOfArray(queue_items, 3);

    fifo_push(&manual_checkout_fifo[i], id); 
}


/* 
- When it is their turn, it takes 5 seconds to register all the items and pay.
*/
void *manual_checkout_thread(void *arg)
{
    struct fifo_obj* obj = (struct fifo_obj*)arg;
    bool checkout_turn = true;
    uint32_t counter = get_random_cashier_checkout_time();
    int id = 0;

    while(1)
    {
        // Check & pull customer from queue 
        if (checkout_turn)
        {
            if ( fifo_data_isavailable(obj) )
            {
                id = fifo_pull(obj);
                DEBUG_PRINT("==> Cashier - Customer id %d checking out\n", id);
                checkout_turn = false;
            }
        }else 
        {
            if (counter == 0)
            {
                checkout_turn = true;
                // Add to return cart list
                add_to_list(return_list_head, id, 0);
                counter = get_random_cashier_checkout_time();
            }
            else{
                 counter -= 1;
            }
        }
        usleep(100*1000); // 100ms
    }
}


//==============================================================================================
/* 
- Customers with a handheld scanner go to one of two checkout terminals, 
- if none are available, they wait in one queue, first customer in the queue takes the first terminal which becomes available. 
*/
void scanner_checkout_add_to_queue(char id)
{
    if ( scanner_checkout_fifo[0].fifo_head < scanner_checkout_fifo[1].fifo_head)
    {
        fifo_push(&scanner_checkout_fifo[0], id);
    }else 
    {
        fifo_push(&scanner_checkout_fifo[1], id);
    }
}

/*
- Registering the handheld scanner and paying only takes 2 seconds, 
- after that the handheld scanner becomes immediately available for new
customers.
*/
void *scanner_checkout_thread(void *arg)
{
    struct fifo_obj* obj = (struct fifo_obj*)arg;
    bool checkout_turn = true;
    uint32_t counter = get_random_scanner_checkout_time();
    int id = 0;

    while(1)
    {
        // Check & pull customer from queue 
        if (checkout_turn)
        {
            if ( fifo_data_isavailable(obj) )
            {
                id = fifo_pull(obj);
                DEBUG_PRINT("==>Scanner- Customer id %d checking out\n", id);
                checkout_turn = false;
            }
        }else 
        {
            if (counter == 0)
            {
                checkout_turn = true;
                DEBUG_PRINT("==>ID = %d return scanner\n", id);
                // Release scanner for next person
                sem_post(&scanner_sem);
                // Add to return cart list
                add_to_list(return_list_head, id, 1);
                counter = get_random_scanner_checkout_time();
            }else
            {
                counter -= 1;
            }
        }
        usleep(100*1000); // 100ms
    }
}

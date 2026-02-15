//slko - simple logging kernel module
//written without any headers
//tg:@s0nny13

int init_module(void);
void cleanup_module(void);
int slko_logging_thread(void *);

static unsigned int module_timer = 0;
static const char name_timer[] = "set_timer";
static char *file_path = 0;
static const char name_file_path[] = "file_path";

static volatile int flag_allow_thread_work = 1;//when cleanup_module called this must be set to 0
static volatile int flag_thread_work_finished = 0;//if previous flag eq 0 then finish thread cycle and set this flag to 1 and stop thread
static unsigned int timeout = 0;


struct kernel_module_parameter {
    const char *name;
    struct module *mod;
    void *ops;
    unsigned short perm;
    signed char level;
    unsigned char flags;
    void *arg;
};

__attribute__((section("__param"), aligned(8), used))
static const struct kernel_module_parameter __par_timer = {
    name_timer, 
    0, 
    (void *)(unsigned long)PARAM_OPS_INT_ADDR, 
    0444, 
    -1,
    0,
    &module_timer
}; 

__attribute__((section("__param"), aligned(8), used))
static const struct kernel_module_parameter __par_file_path = {
    name_file_path, 
    0, 
    (void *)(unsigned long)PARAM_OPS_CHARP_ADDR, 
    0444, 
    -1, 
    0,
    &file_path
}; 

char __attribute__((section(".modinfo"))) mod_license[] = "license=GPL";
char __attribute__((section(".modinfo"))) mod_author[] = "author=s0nny13";
char __attribute__((section(".modinfo"))) mod_vermagic[] = VERMAGIC_REVERSED;
char __attribute__((section(".modinfo"))) mod_param_timer[] = "parm=set_timer:uint";
char __attribute__((section(".modinfo"))) mod_param_path[] = "parm=file_path:charp";


typedef int (*printk_t)(const char *fmt, ...);
typedef void (*kfree_t)(const void *);
typedef void (*msleep_t)(unsigned int);

struct file;

typedef struct file *(*filp_open_t)(const char *filename, int flags, int mode);
typedef int (*filp_close_t)(struct file *filp, void *id);
typedef long (*kernel_write_t)(struct file *filp, const void *buf, unsigned long count, long long *pos);
typedef int (*snprintf_t)(char *buf, unsigned long size, const char *fmt, ...);
typedef void *(*kthread_create_t)(int (*fn)(void *), void *arg, int node, const char namefmt[], ...);
typedef int (*wake_up_process_t)(void *addr);


static printk_t _printk;
static kfree_t _kfree;
static msleep_t _msleep;
static filp_open_t _filp_open;
static filp_close_t _filp_close;
static kernel_write_t _kernel_write;
static snprintf_t _snprintf;
static kthread_create_t _kthread_create;
static wake_up_process_t _wake_up_process;

__attribute__((section(".init.text")))
int init_module(void){
    asm volatile ("endbr64");
    _printk = (printk_t)PRINTK_ADDR;
    _kfree = (kfree_t)KFREE_ADDR;
    _msleep = (msleep_t)MSLEEP_ADDR;
    _filp_open = (filp_open_t)FILP_OPEN_ADDR;
    _filp_close = (filp_close_t)FILP_CLOSE_ADDR;
    _kernel_write = (kernel_write_t)KERNEL_WRITE_ADDR;
    _snprintf = (snprintf_t)SNPRINTF_ADDR;
    _kthread_create = (kthread_create_t)KTHREAD_CREATE_ADDR;
    _wake_up_process = (wake_up_process_t)WAKE_UP_PROCESS_ADDR;
    if(!file_path){
        flag_thread_work_finished = 1;
        return -2;  
    }
    timeout = (module_timer < 100)?100:module_timer;
    if(_kthread_create && _wake_up_process){
        void *worker = _kthread_create(slko_logging_thread, (void *)0, -1, "slko_worker");
        if(!worker || (unsigned long)worker >= (unsigned long)-4095){
            _printk("[SLKO_ERR]: failed to create kthread.\n");
            flag_thread_work_finished = 1;
            return -1;
        }
        _wake_up_process(worker);
    } else {
        _printk("[LINK_ERROR]: no addresses for used functions.\n");
        return -3;
    }
    return 0;
}

int slko_logging_thread(void *addr){
    struct file *f = 0x00;
    long long pos = 0;
    char log_msg[128];
    int len;
    int counter = 0;
    //_printk("ork\n");
    if(file_path && _filp_open){
        f = _filp_open(file_path, 0101, 0644);
    }
    
    if(!f || (unsigned long)f >= (unsigned long)-4095){
        flag_thread_work_finished = 1;
        return -1;
    }
    
    while(flag_allow_thread_work == 1){
        if(_snprintf){
            len = _snprintf(log_msg, sizeof(log_msg), "Hello from kernel module (%d)\n", counter);
            if(_kernel_write && len > 0){
                _kernel_write(f, log_msg, len, &pos);
            }
        }
        counter++;
        _msleep(timeout);
    }
    if(_filp_close) _filp_close(f, 0);
    flag_thread_work_finished = 1;
    return 0;
}

__attribute__((section(".exit.text")))
void cleanup_module(void){
    asm volatile ("endbr64");
    flag_allow_thread_work = 0;
    int timeout_kill = 100;
    while(flag_thread_work_finished == 0 && timeout_kill-- > 0){
        _msleep(10);
    }
    //if(file_path)
    //if(_kfree) _kfree(file_path);
}

static void *__ptr_init __attribute__((section(".init.data"), used)) = init_module;
static void *__ptr_exit __attribute__((section(".exit.data"), used)) = cleanup_module;
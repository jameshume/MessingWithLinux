#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define DEFAULT_MS_BETWEEN_EVENTS 1000
#define MAX_LOOPS 3


struct jehtech_data {
    atomic_t should_run;
    atomic_t jiffies_between_events;
    struct timer_list timer;
    wait_queue_head_t waitq_head;
    wait_queue_t      waitq_entry;
    volatile bool completed;
    size_t counter;
};

static struct jehtech_data *gbl_data;

static void key_press_generator(unsigned long tmrdata)
{
    struct jehtech_data *data = gbl_data; //(struct jehtech_data *)tmrdata;

    printk("TIMER: %p, %p", (void *)tmrdata, (void*)gbl_data);
    if (atomic_read(&data->should_run))
    {
        if (gbl_data->counter > MAX_LOOPS) {
            gbl_data->completed = true;
            wake_up_interruptible(&data->waitq_head);
        }
        else if (gbl_data->counter >= 0) {
            gbl_data->counter+= 1;
            data->timer.expires += atomic_read(&data->jiffies_between_events);
            printk(KERN_INFO "JEH-TECH timer event %lu", gbl_data->counter);
            add_timer(&data->timer);
        }

    }
}


static int __init jehtech_kernel_timers_init(void)
{
    gbl_data = kzalloc(sizeof(*gbl_data), GFP_KERNEL);
    if (!gbl_data) {
        printk(KERN_ALERT "OUT OF MEMORY!");
        return -ENOMEM;
    }

    init_waitqueue_head(&gbl_data->waitq_head);
    init_waitqueue_entry(&gbl_data->waitq_entry, current);

    atomic_set(&gbl_data->should_run, 1);
    atomic_set(&gbl_data->jiffies_between_events, (int)msecs_to_jiffies(DEFAULT_MS_BETWEEN_EVENTS));
    init_timer(&gbl_data->timer);
    gbl_data->timer.data = (unsigned long)&gbl_data; // Hmm this doesn't appear to work!
    gbl_data->timer.function = key_press_generator;
    gbl_data->timer.expires =  jiffies + atomic_read(&gbl_data->jiffies_between_events);

    printk(KERN_INFO "Starting timer and waiting on completion! Now = %lu, exp = %lu", jiffies, gbl_data->timer.expires);
    gbl_data->completed = false;
    gbl_data->counter = 0;
    add_timer(&gbl_data->timer);

    while(true) {
       printk(KERN_INFO "Adding myself to wait Q to wait for completion...");
       add_wait_queue(&gbl_data->waitq_head, &gbl_data->waitq_entry);
       set_current_state(TASK_INTERRUPTIBLE);
       schedule();
       printk("Completed = %u", (unsigned int)gbl_data->completed);
       if (gbl_data->completed)
          break;
       remove_wait_queue(&gbl_data->waitq_head, &gbl_data->waitq_entry);
       if (signal_pending(current)) {
           printk("Signal received, aborting...");
          return -ERESTARTSYS;
       }
    }

    return 0;
}
module_init(jehtech_kernel_timers_init);

static void __exit jehtech_kernel_timers_exit(void)
{
    // atomic_set(&gbl_data->should_run, 0);
    // del_timer_sync(&gbl_data->timer);
    kfree(gbl_data);
}
module_exit(jehtech_kernel_timers_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JehTech");
MODULE_DESCRIPTION("An example");
#include <crescent/common.h>
#include <crescent/core/cmdline.h>
#include <crescent/core/limine.h>
#include <crescent/core/printk.h>
#include <crescent/mm/mmap.h>
#include <crescent/lib/hashtable.h>
#include <crescent/lib/string.h>

static struct hashtable* cmdline_hashtable = NULL;

const char* cmdline_get(const char* arg) {
    if (!cmdline_hashtable)
        return NULL;

    const char* ret;
    int err = hashtable_search(cmdline_hashtable, arg, strlen(arg), &ret);
    if (err)
        return NULL;
    return ret;
}

int cmdline_parse(void) {
    if (!g_limine_kernel_file_request.response)
        return -ENOPROTOOPT;
    const char* cmdline = g_limine_kernel_file_request.response->kernel_file->cmdline;
    if (!cmdline)
        return -ENOPROTOOPT;

    size_t cmdline_size = strlen(cmdline);
    if (cmdline_size == 0)
        return 0;
    cmdline_size++;

    cmdline_hashtable = hashtable_create(5, sizeof(char*));
    if (!cmdline_hashtable)
        return -ENOMEM;

    /* 
     * Don't want to modify the cmdline directly, so make a copy of it. Use kmmap over kmalloc
     * since after we're done parsing, the memory becomes read only.
     */
    int errno;
    char* cmdline_copy = kmmap(NULL, cmdline_size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, NULL, &errno);
    if (!cmdline_copy)
        return errno;
    strcpy(cmdline_copy, cmdline);

    const char* key;
    const char* value;

    while (*cmdline_copy) {
        while (*cmdline_copy == ' ')
            cmdline_copy++;
        if (*cmdline_copy == '\0')
            return 0;

        /* Parse the option part of the argument */
        key = cmdline_copy;
        while (*cmdline_copy != '=') {
            if (*cmdline_copy == '\0')
                return 0;
            cmdline_copy++;
        }

        /* Replace the equals sign with a null terminator */
        *cmdline_copy = '\0';
        value = ++cmdline_copy;

        /* Now parse the value of the option we want */
        while (*cmdline_copy != ' ') {
            if (*cmdline_copy == '\0') {
                /* This is it for the command line arguments, so add the final value into the hashtable and exit */
                int err = hashtable_insert(cmdline_hashtable, key, strlen(key), &value);
                if (err) {
                    printk(PL_ERR "core: Error inserting cmdline argument into hashtable (errno %i)\n", err);
                    return err;
                }
                return 0;
            }
            cmdline_copy++;
        }

        /* Replace the space with a null character, and then insert the value into the hashtable */
        *cmdline_copy = '\0';
        int err = hashtable_insert(cmdline_hashtable, key, strlen(key), &value);
        if (err) {
            printk(PL_ERR "core: Error inserting cmdline argument into hashtable (errno %i)\n", err);
            return err;
        }
        cmdline_copy++;
    }

    /* Now just remap as read only */
    errno = kmprotect(cmdline_copy, cmdline_size, MMU_READ);
    if (errno)
        printk(PL_ERR "core: Failed to remap command line arguments as read only!\n");
    return 0;
}

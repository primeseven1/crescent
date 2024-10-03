#include <crescent/common.h>
#include <crescent/core/cmdline.h>
#include <crescent/core/limine.h>
#include <crescent/core/printk.h>
#include <crescent/mm/mmap.h>
#include <crescent/lib/hashtable.h>
#include <crescent/lib/string.h>

static struct hashtable* cmdline_hashtable = NULL;

const char* cmdline_get(const char* arg) {
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

    cmdline_hashtable = hashtable_create(5, sizeof(char*));
    if (!cmdline_hashtable)
        return -ENOMEM;

    /* 
     * Since we don't want to modify the command line string the loader gives us, 
     * copy it over to another area of memory. This memory will never be freed.
     * Use kmmap so the memory can be safely marked as read only after we're done
     */
    int errno;
    size_t cmdline_buf_size = strlen(cmdline) + 1;
    char* cmdline_copy = kmmap(NULL, cmdline_buf_size, KMMAP_ALLOC, 
            MMU_READ | MMU_WRITE, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL, NULL, &errno);
    if (!cmdline_copy)
        return errno;
    strlcpy(cmdline_copy, cmdline, cmdline_buf_size);

    const char* key;
    const char* value;

    while (*cmdline_copy) {
        while (*cmdline_copy == ' ')
            cmdline++;
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
                    printk(PL_ERR "Error inserting cmdline argument into hashtable (errno %i)\n", err);
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
            printk(PL_ERR "Error inserting cmdline argument into hashtable (errno %i)\n", err);
            return err;
        }
        cmdline_copy++;
    }

    /* Now just remap as read only */
    errno = kmprotect(cmdline_copy, cmdline_buf_size, MMU_READ);
    if (errno)
        printk(PL_ERR "Failed to remap command line arguments as read only!\n");
    return 0;
}

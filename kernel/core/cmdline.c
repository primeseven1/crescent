#include <crescent/common.h>
#include <crescent/core/cmdline.h>
#include <crescent/core/limine.h>
#include <crescent/core/printk.h>
#include <crescent/mm/heap.h>
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

    size_t cmdline_buf_size = strlen(cmdline) + 1;
    char* cmdline_copy = kmalloc(cmdline_buf_size, GFP_VM_KERNEL | GFP_PM_ZONE_NORMAL);
    if (!cmdline_copy)
        return -ENOMEM;
    strlcpy(cmdline_copy, cmdline, cmdline_buf_size);

    const char* key;
    const char* value;

    while (*cmdline_copy) {
        while (*cmdline_copy == ' ')
            cmdline++;
        if (*cmdline_copy == '\0')
            return 0;

        key = cmdline_copy;
        while (*cmdline_copy != '=') {
            if (*cmdline_copy == '\0')
                return 0;
            cmdline_copy++;
        }

        *cmdline_copy = '\0';
        value = ++cmdline_copy;

        while (*cmdline_copy != ' ') {
            if (*cmdline_copy == '\0') {
                int err = hashtable_insert(cmdline_hashtable, key, strlen(key), &value);
                if (err) {
                    printk(PL_ERR "Error inserting cmdline argument into hashtable (errno %i)\n", err);
                    return err;
                }
                return 0;
            }
            cmdline_copy++;
        }

        *cmdline_copy = '\0';
        int err = hashtable_insert(cmdline_hashtable, key, strlen(key), &value);
        if (err) {
            printk(PL_ERR "Error inserting cmdline argument into hashtable (errno %i)\n", err);
            return err;
        }
        cmdline_copy++;
    }

    return 0;
}

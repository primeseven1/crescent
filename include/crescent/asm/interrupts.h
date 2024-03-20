#pragma once

#ifndef __ASSEMBLY_FILE__

/* 
 * Initialize the interrupt descriptor table 
 *
 * --- Notes ---
 * - Should only be called once
 */
void do_idt_init(void);
/* 
 * Load the interrupt descriptor table 
 *
 * --- Notes ---
 * - Should only be called after do_idt_init is called 
 */
void do_idt_load(void);

#endif /* __ASSEMBLY_FILE__ */

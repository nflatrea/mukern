/** 
file: main.c portable kernel entry.
**/

#include "console.h"
#include "arch.h"
#include "klib.h"

#define MUKERN_VERSION "v.0.1.062026"

void motd(void)
{
    kprintf ("\n    ___       ___       ___       ___       ___       ___      \n"
             "   /\\__\\     /\\__\\     /\\__\\     /\\  \\     /\\  \\     /\\__\\     \n"    
             "  /::L_L_   /:/ _/_   /:/ _/_   /::\\  \\   /::\\  \\   /:| _|_    \n"    
             " /:/L:\\__\\ /:/_/\\__\\ /::-\"\\__\\ /::\\:\\__\\ /::\\:\\__\\ /::|/\\__\\  \n"   
             " \\/_/:/  / \\:\\/:/  / \\;:;-\",-\" \\:\\:\\/  / \\;:::/  / \\/|::/  / \n"  
             "   /:/  /   \\::/  /   |:|  |    \\:\\/  /   |:\\/__/    |:/  /    \n"    
             "   \\/__/     \\/__/     \\|__|     \\/__/     \\|__|     \\/__/     \n"    
             "                                      muKern " MUKERN_VERSION "\n");

    kprintf("arch: %s\n", arch_name());
}

void repl(void); /* kernel/core/repl.c */

void kmain(void)
{
    console_init();
    arch_traps_init();   /* M1: catch CPU exceptions instead of triple-faulting */

    
    motd();
    repl();
    kprintf("Goodbye.");
    arch_halt();
}

/*
file: banner.h
desc: The boot banner, shared so the early UART path (main.c) and the init
      task (init.c, which re-prints it over IPC so it also reaches the VGA
      screen) agree on one string. Pure data, no logic.
*/
#ifndef MUKERN_BANNER_H
#define MUKERN_BANNER_H

#define MUKERN_VERSION "v.0.1.062026"

#define MUKERN_MOTD \
"\n    ___       ___       ___       ___       ___       ___      \n"   \
"   /\\__\\     /\\__\\     /\\__\\     /\\  \\     /\\  \\     /\\__\\     \n"   \
"  /::L_L_   /:/ _/_   /:/ _/_   /::\\  \\   /::\\  \\   /:| _|_    \n"     \
" /:/L:\\__\\ /:/_/\\__\\ /::-\"\\__\\ /::\\:\\__\\ /::\\:\\__\\ /::|/\\__\\  \n" \
" \\/_/:/  / \\:\\/:/  / \\;:;-\",-\" \\:\\:\\/  / \\;:::/  / \\/|::/  / \n"   \
"   /:/  /   \\::/  /   |:|  |    \\:\\/  /   |:\\/__/    |:/  /    \n"     \
"   \\/__/     \\/__/     \\|__|     \\/__/     \\|__|     \\/__/     \n"   \
"                                      muKern " MUKERN_VERSION "\n"

#endif /* MUKERN_BANNER_H */

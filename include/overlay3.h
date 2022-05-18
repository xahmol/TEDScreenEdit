#ifndef __OVERLAY3_H_
#define __OVERLAY3_H_

int chooseidandfilename(char* headertext, unsigned char maxlen);
unsigned char checkiffileexists(char* filetocheck, unsigned char id);
void loadscreenmap();
void savescreenmap();
void saveproject();
void loadproject();
void loadcharset();
void savecharset();
void changebackgroundcolor();
void changebordercolor();
void versioninfo();
void plot_try();

#endif // __OVERLAY3_H_
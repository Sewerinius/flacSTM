#ifndef FLACSTM_PLAYER_H
#define FLACSTM_PLAYER_H

void playerInit();
void playerInitFile(char* fileName);
void playerStop();
void playerProcess();
void playerDraw();
void playerHandleClick(int x, int y);

#endif //FLACSTM_PLAYER_H

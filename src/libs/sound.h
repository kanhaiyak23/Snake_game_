#ifndef SOUND_H
#define SOUND_H

void sound_eat(int combo);       /* blip, pitch rises with combo streak  */
void sound_die(void);            /* descending buzz on life lost         */
void sound_levelup(void);        /* 4-note ascending fanfare             */
void sound_game_start(void);     /* rising sweep at game start           */
void sound_wrap(void);           /* brief swoosh when snake wraps a wall */

#endif /* SOUND_H */

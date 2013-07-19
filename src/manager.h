#ifndef MANAGER_H
#define MANAGER_H

#include <stdbool.h>

void manager_update();
void manager_move(int index, bool relative);
void manager_activate(int index, bool relative);

#endif /* MANAGER_H */

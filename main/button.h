#ifndef BUTTON_H
#define BUTTON_H

#define BUTTON_GPIO GPIO_NUM_16

void button_init();
void button_task(void *args);

#endif
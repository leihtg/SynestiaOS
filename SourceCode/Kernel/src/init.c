#include <font8bits.h>
#include <gfx2d.h>
#include <gpu.h>
#include <gui_button.h>
#include <gui_canvas.h>
#include <gui_container.h>
#include <gui_label.h>
#include <gui_panel.h>
#include <gui_window.h>
#include <interrupt.h>
#include <kheap.h>
#include <sched.h>
#include <stdlib.h>
#include <synestia_os_hal.h>
#include <vmm.h>

extern uint32_t *gpu_flush(int args);

void print_splash() {
  const char *str = "   _____                       _   _       \n"
                    "  / ____|                     | | (_)      \n"
                    " | (___  _   _ _ __   ___  ___| |_ _  __ _ \n"
                    "  \\___ \\| | | | '_ \\ / _ \\/ __| __| |/ _` |\n"
                    "  ____) | |_| | | | |  __/\\__ \\ |_| | (_| |\n"
                    " |_____/ \\__, |_| |_|\\___||___/\\__|_|\\__,_|\n"
                    "          __/ |                            \n"
                    "         |___/                          \n";
  print(str);
}

uint32_t hour, minutes, second = 0;

void draw_task_bar() {
  uint32_t *barBuffer = (uint32_t *)kheap_alloc(1024 * 32 * 4);

  for (int i = 0; i < 1024; i++) {
    for (int j = 0; j < 31; j++) {
      barBuffer[j * 1024 + i] = ((FLUENT_PRIMARY_COLOR >> 16) & 0xFF - j) << 16 |
                                ((FLUENT_PRIMARY_COLOR >> 8) & 0xFF - j) << 8 | (FLUENT_PRIMARY_COLOR & 0xFF - j);
    }
  }
  for (int i = 0; i < 1024; i++) {
    barBuffer[31 * 1024 + i] = 0x999999;
  }
  gfx2d_draw_bitmap(0, 0, 1024, 32, barBuffer);
  gfx2d_draw_logo(0, 0, 0xFFFFFF);
}

uint32_t idle_0_count = 0;
uint32_t idle_1_count = 0;

uint32_t *window_thread1(int args) {
  GUIWindow window;
  gui_window_create(&window);
  window.component.size.width = 510;
  window.component.size.height = 500;
  gui_window_init(&window, 0, 32, "window1");
  GUILabel labelIdle1;
  gui_label_create(&labelIdle1);
  labelIdle1.component.foreground.r = 0x00;
  labelIdle1.component.foreground.g = 0x00;
  labelIdle1.component.foreground.b = 0x00;
  labelIdle1.component.colorMode = TRANSPARENT;
  labelIdle1.component.size.width = 100;
  gui_window_add_children(&window, &(labelIdle1.component));
  while (1) {
    char idle_1_str[10] = {'\0'};
    gui_label_init(&labelIdle1, 0, 0, itoa(idle_0_count, &idle_1_str, 10));
    disable_interrupt();
    gui_window_draw(&window);
    enable_interrupt();
    idle_0_count += 2;
  }
}

uint32_t *window_thread2(int args) {
  GUIWindow window1;
  gui_window_create(&window1);
  window1.component.size.width = 510;
  window1.component.size.height = 500;
  gui_window_init(&window1, 512, 32, "window2");
  GUILabel labelIdle2;
  gui_label_create(&labelIdle2);
  labelIdle2.component.foreground.r = 0x00;
  labelIdle2.component.foreground.g = 0x00;
  labelIdle2.component.foreground.b = 0x00;
  labelIdle2.component.colorMode = TRANSPARENT;
  labelIdle2.component.size.width = 100;
  gui_window_add_children(&window1, &(labelIdle2.component));
  while (1) {
    char idle_2_str[10] = {'\0'};
    gui_label_init(&labelIdle2, 0, 0, itoa(idle_1_count, &idle_2_str, 10));
    disable_interrupt();
    gui_window_draw(&window1);
    enable_interrupt();
    idle_1_count++;
  }
}

TimerHandler gpuHandler;
void kernel_main(void) {

  print_splash();

  vmm_init();

  kheap_init();

  init_bsp();

  init_interrupt();

  gpu_init();
  gfx2d_draw_bitmap(0, 0, 1024, 768, desktop());
  draw_task_bar();

  gpuHandler.node.next = nullptr;
  gpuHandler.node.prev = nullptr;
  gpuHandler.timer_interrupt_handler = &gpu_flush;
  register_time_interrupt(&gpuHandler);

  schd_init();

  Thread *window1Thread = thread_create("window1", &window_thread1, 1, 1);
  schd_init_thread(window1Thread, 0);

  Thread *window2Thread = thread_create("window2", &window_thread2, 1, 1);
  schd_init_thread(window2Thread, 1);

  schd_schedule();
}
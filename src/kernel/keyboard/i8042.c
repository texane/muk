/*
** Made by texane <texane@gmail.com>
** 
** Started on  Tue Dec 11 02:36:22 2007 texane
** Last update Wed Dec 12 03:23:22 2007 texane
*/




#include "../sys/types.h"
#include "../arch/cpu.h"
#include "../arch/pic.h"
#include "../debug/debug.h"
#include "../keyboard/keyboard.h"



/* controller ports
 */
#define I8042_PORT_STATUS 0x64
#define I8042_PORT_OUT 0x60
#define I8042_PORT_IN 0x64
#define I8042_PORT_DATA 0x60
#define I8042_PORT_CMD 0x64


/* controller bits
 */
#define BIT(n) (1 << (n))
#define I8042_BIT_OUT_FULL BIT(0)
#define I8042_BIT_IN_FULL BIT(1)
#define I8042_BIT_SYS_FLAG BIT(2)
#define I8042_BIT_CMD_DATA BIT(3)
#define I8042_BIT_KBD_INH BIT(4)
#define I8042_BIT_TX_TIMEOUT BIT(5)
#define I8042_BIT_RX_TIMEOUT BIT(6)
#define I8042_BIT_PARITY_EVEN BIT(7)


/* controller commands
 */
#define I8042_CMD_READ_CMD 0x20
#define I8042_CMD_WRITE_CMD 0x60
#define I8042_CMD_SELF_TEST 0xaa
#define I8042_CMD_IF_TEST 0xab
#define I8042_CMD_DUMP 0xac

#define I8042_CMD_KBD_DISABLE 0xad
#define I8042_CMD_KBD_ENABLE 0xae
#define I8042_CMD_KBD_TEST 0x01ab
#define I8042_CMD_KBD_LOOP 0x11d2

#define I8042_CMD_READ_INPUT 0xc0
#define I8042_CMD_READ_OUTPUT 0xd0
#define I8042_CMD_WRITE_OUTPUT 0xd1
#define I8042_CMD_READ_TEST_PINS 0xe0
#define I8042_CMD_MENU 0xf1



/* interrupt handler
 */

static void on_interrupt(void)
{
  uint8_t status;
  uint8_t data;

  /* interrupt gate, can safely
   */

  status = cpu_inb(I8042_PORT_STATUS);
  if (status & 1)
    {
      data = cpu_inb(I8042_PORT_OUT);
      keyboard_putc(data);
    }
}



/* initialize the controller
 */

error_t i8042_initialize(void)
{
  /* input buffer not empty
   */
  while (cpu_inb(I8042_PORT_STATUS) & I8042_BIT_IN_FULL)
    ;

  /* self test command
   */
  cpu_outb(I8042_PORT_CMD, I8042_CMD_SELF_TEST);
  while (!(cpu_inb(I8042_PORT_STATUS) & I8042_BIT_OUT_FULL))
    ;
  if (cpu_inb(I8042_PORT_OUT) != 0x55)
    {
      BUG();
      return ERROR_FAILURE;
    }

  /* test interface command
   */
  cpu_outb(I8042_PORT_CMD, I8042_CMD_IF_TEST);
  while (!(cpu_inb(I8042_PORT_STATUS) & I8042_BIT_OUT_FULL))
    ;
  if (cpu_inb(I8042_PORT_OUT) != 0x00)
    {
      BUG();
      return ERROR_FAILURE;
    }

  /* enable i8042 controller
   */
  cpu_outb(I8042_PORT_CMD, I8042_CMD_KBD_ENABLE);
  while (!(cpu_inb(I8042_PORT_STATUS) & I8042_BIT_OUT_FULL))
    ;

  /* clear output buffer
   */
  cpu_inb(I8042_PORT_OUT);

  /* enable interrupt
   */
  serial_printl("enabling interrupt\n");
  pic_enable_irq(1, on_interrupt);

  return ERROR_SUCCESS;
}

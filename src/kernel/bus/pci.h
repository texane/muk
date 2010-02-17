/*
** Made by texane <texane@gmail.com>
** 
** Started on  Wed Aug 22 23:29:19 2007 texane
** Last update Sun Aug 26 04:02:43 2007 texane
*/


#ifndef PCI_H_INCLUDED
# define PCI_H_INCLUDED


#include "../sys/types.h"


struct pci_bus;

typedef struct pci_dev
{
  struct pci_dev* next;
  struct pci_dev* prev;
  struct pci_bus* bus;
  uchar_t idev;

  /* config header */
  ushort_t vendor_id;
  ushort_t device_id;
  uint_t class_code;
  uchar_t int_ln;

  /* addressing */
  bool_t res_ismem;
  uint_t res_base;
  uint_t res_size;
} pci_dev_t;


int pci_initialize(void);
int pci_list(void);
pci_dev_t* pci_dev_find_rtl8139(void);



#endif /* ! PCI_H_INCLUDED */

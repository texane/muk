/*
** Made by texane <texane@gmail.com>
** 
** Started on  Sat Nov 17 21:20:38 2007 texane
** Last update Mon Nov 19 04:00:20 2007 texane
*/



#include "../net/net.h"
#include "../mm/mm.h"
#include "../debug/debug.h"
#include "../sys/types.h"
#include "../libc/libc.h"



/* alloc
 */

error_t net_buf_alloc(net_buf_t** nb)
{
  *nb = mm_alloc(sizeof(net_buf_t));
  if (*nb == NULL)
    return ERROR_NO_MEMORY;

  (*nb)->next = NULL;
  (*nb)->size = 0;
  (*nb)->off = 0;
  (*nb)->buf = NULL;  

  return ERROR_SUCCESS;
}


error_t net_buf_alloc_with_size(net_buf_t** res, size_t size)
{
  error_t error;
  net_buf_t* nb;

  *res = NULL;

  error = net_buf_alloc(&nb);
  if (error_is_failure(error))
    return error;

  nb->buf = mm_alloc(size);
  if (nb->buf == NULL)
    {
      net_buf_free(nb);
      return ERROR_NO_MEMORY;
    }

  nb->size = size;

  *res = nb;

  return ERROR_SUCCESS;
}



/* free
 */

error_t net_buf_free(net_buf_t* nb)
{
  if (nb->buf != NULL)
    mm_free(nb->buf);
  mm_free(nb);

  return ERROR_SUCCESS;
}



/* append
 */

error_t net_buf_append(net_buf_t* nb,
		       uchar_t* buf,
		       size_t size)
{
  if (nb->buf != NULL)
    {
      NOT_IMPLEMENTED();
      return ERROR_NOT_IMPLEMENTED;
    }

  nb->buf = mm_alloc(size);
  if (nb->buf == NULL)
    return ERROR_NO_MEMORY;

  nb->size = size;
  memcpy(nb->buf, buf, nb->size);

  return ERROR_SUCCESS;
}



/* advance
 */

error_t net_buf_advance(net_buf_t* nb, off_t off)
{
  if (nb->buf == NULL)
    return ERROR_FAILURE;

  if ((nb->off + off) > nb->size)
    return ERROR_FAILURE;

  nb->off += off;
  
  return ERROR_SUCCESS;
}



/* rewind
 */

error_t net_buf_rewind(net_buf_t* nb, off_t off)
{
  if (nb->buf == NULL)
    return ERROR_FAILURE;

  if (off > nb->off)
    return ERROR_INVALID_SIZE;

  nb->off -= off;

  return ERROR_SUCCESS;
}



/* read at current pos
 */

error_t net_buf_read(net_buf_t* nb, uchar_t** buf, size_t* size)
{
  if (nb->buf == NULL)
    return ERROR_FAILURE;

  *buf = nb->buf + nb->off;
  *size = nb->size - nb->off;

  return ERROR_SUCCESS;
}



/* write at current pos (not updated)
 */

error_t net_buf_write(net_buf_t* nb, const uint8_t* buf, size_t size)
{
  if (nb->buf == NULL)
    return ERROR_FAILURE;

  if ((nb->size - nb->off) < size)
    return ERROR_INVALID_SIZE;

  memcpy(nb->buf + nb->off, buf, size);

  return ERROR_SUCCESS;
}



/* get net buf size
 */

error_t net_buf_get_size(const struct net_buf* nb, size_t* size)
{
  *size = nb->size - nb->off;

  return ERROR_SUCCESS;
}



/* clone buffer
 */

error_t net_buf_clone(struct net_buf* nb_from,
		      struct net_buf** nb_to,
		      size_t size)
{
  struct net_buf* tmp;
  error_t error;

  *nb_to = NULL;

  /* allocate buffer
   */
  error = net_buf_alloc_with_size(&tmp, size);
  if (error_is_failure(error))
    return error;
  net_buf_write(tmp, nb_from->buf + nb_from->off, size);

  /* copy
   */
  tmp->dport = nb_from->dport;
  tmp->sport = nb_from->sport;
  tmp->proto_src = nb_from->proto_src;
  tmp->proto_dst = nb_from->proto_dst;
  memcpy(tmp->hw_src, nb_from->hw_src, ETH_ADDR_LEN);
  memcpy(tmp->hw_dst, nb_from->hw_dst, ETH_ADDR_LEN);

  *nb_to = tmp;
  
  return ERROR_SUCCESS;
}

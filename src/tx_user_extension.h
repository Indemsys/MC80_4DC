#ifndef TX_USER_EXTENSION_H_
#define TX_USER_EXTENSION_H_

#ifdef __cplusplus
extern "C"
{
#endif

// Extend TX_THREAD structure to store driver pointer
#define TX_THREAD_USER_EXTENSION \
  ULONG environment;             \
  ULONG driver;

#ifdef __cplusplus
}
#endif
#endif

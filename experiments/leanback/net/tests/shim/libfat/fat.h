#ifndef LB_FAKE_FAT_H
#define LB_FAKE_FAT_H
typedef struct {int placeholder;} DISC_INTERFACE;
int fatMountSimple(const char *,DISC_INTERFACE *);
void fatUnmount(const char *);
#endif

#ifndef MIST_COMPAT_H
#define MIST_COMPAT_H

#define LEGACY_MODE     2
#define MIST_MODE       1
#define DEFAULT_MODE    0

#ifdef __cplusplus
extern "C" {
#endif

int mist_init();
int mist_loop();

#ifdef __cplusplus
}
#endif


#endif //MIST_COMPAT_H

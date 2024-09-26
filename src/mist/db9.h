#ifndef MIST_DB9_H
#define MIST_DB9_H

#ifdef __cplusplus
extern "C" {
#endif
    void InitDB9();
    char GetDB9(char index, unsigned char *joy_map);
    void DB9SetLegacy(unsigned char on);  
    void DB9Update(unsigned char joy_num, unsigned char usbjoy);
#ifdef __cplusplus
}
#endif
#endif //MIST_DB9_H
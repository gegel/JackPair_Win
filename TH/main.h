#ifdef __cplusplus
extern "C" {
#endif

 void start_th(char* com, char* sec, char* voice_in, char* voice_out, char* line_in, char* line_out);
 void stop_th(void);
 unsigned int get_status(void);
 void set_ptt(unsigned char val);
 void set_duplex(unsigned char val);
 void set_direct(unsigned char val);
 void set_modem(unsigned char val);
 void set_avad(unsigned char val);
 
 short th(void);
 #ifdef __cplusplus
}
#endif

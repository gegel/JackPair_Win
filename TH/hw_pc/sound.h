//interface

void init_Snd(char* voice_in, char* voice_out, char* line_in, char* line_out);

int start_Voice(void);
void stop_Voice(void);
int start_Line(void);
void stop_Line(void);

short setLineRate(short value);

int rec_Mike(short* pcm);
int rec_Line(short* pcm);
void play_Spc(short* pcm);
void play_Line(short* pcm);

unsigned char get_level(short* m);

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc,char **argv)
{
  FILE *f=fopen("tmds_decoder.vhdl","r");
  char in[1024],out[1024],terc[1024];
  char line[1024];
  line[0]=0; fgets(line,1024,f);
  while(line[0]) {
    int ofs=0;
    while(line[ofs]&&line[ofs]<=' ') {
      ofs++;
    }
    if (sscanf(&line[ofs],"when \"%[01]\" => lookup <= \"%[01]\";  -- TERC4 %[01]",in,out,terc)==3) {
      printf("/* %s -> %s TERC %s */\n",in,out,terc);
      int c=strtol(in, NULL, 2);
      int v=strtol(out, NULL, 2);
      int t=strtol(terc, NULL, 2);
      printf("{0x%03x,1,0x%02x,1,0x%x},\n",
	     c,v&0xff,t);
    }
    else if (sscanf(&line[ofs],"when \"%[01]\" => lookup <= \"%[01]\"",in,out)==2) {
      printf("/* %s -> %s */\n",in,out);
      int c=strtol(in, NULL, 2);
      int v=strtol(out, NULL, 2);
      if (strlen(out)>2) {
	printf("{0x%03x,1,0x%02x,0,0},\n",
	       c,v&0xff);
      }
    }
    
    line[0]=0; fgets(line,1024,f);
  }
}

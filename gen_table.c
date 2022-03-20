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
      printf("%s -> %s TERC %s\n",in,out,terc);
    }
    else if (sscanf(&line[ofs],"when \"%[01]\" => lookup <= \"%[01]\"",in,out)==2) {
      printf("%s -> %s\n",in,out);
    }
    
    line[0]=0; fgets(line,1024,f);
  }
}

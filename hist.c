#include <stdio.h>
#include <stdlib.h>

int bins[65536];

char bs_out[17];
char *binstr(unsigned int v)
{
  for(int i=0;i<16;i++) {
    if (v&(1<<i)) bs_out[i]='1'; else bs_out[i]='0';
  }
  bs_out[16]=0;
  return bs_out;
}

int main(int argc,char **argv)
{
  if (argc!=2) {
    fprintf(stderr,"usage: hist <file.cap>\n");
    exit(-1);
  }

  for(int o=0;o<4;o++) {
    fprintf(stderr,"CHANNEL %d:\n",o);
    FILE *f=fopen(argv[1],"rb");
    if (!f) {
      perror("fopen");
      exit(-1);
    }
    
    unsigned char buff[8];
    
    for(int i=0;i<65536;i++) bins[i]=0;
    
    int count=0;
    int n=fread(buff,1,8,f);
    while(n==8) {
      count++;
      n=fread(buff,1,8,f);
      int v=buff[o*2+1]+(buff[o*2+0]<<8);
      bins[v]++;

      int sum=buff[0]+buff[1]+buff[2]+(buff[3]&0xc0);
      sum-=0x100;
      if (buff[3]&0x80) sum++;
      sum&=0xff;
      //      while(sum>0xff) sum-=0xff;
      if (sum!=buff[5]) {
	fprintf(stderr,"Checksum error at row $%x\n",count);
	fprintf(stderr,"   %02x+%02x+%02x+%02x = %03x (checksum is %02x, but calculated %02x)\n",
		buff[0],buff[1],buff[2],(buff[3]&0xc0),
		buff[0]+buff[1]+buff[2]+(buff[3]&0xc0),
		buff[5],sum);
      }
      
    }
  
    fclose(f);
    fprintf(stderr,"Read %d 8-byte rows.\n",count);
    int unique=0;
    for(int i=0;i<65536;i++) {
      if (bins[i]) {
	unique++;
	fprintf(stderr,"  $%04X : %s : %d\n",i,binstr(i),bins[i]);
      }
    }
    fprintf(stderr,"Saw %d unique 2-byte tokens.\n",unique);
  }
}

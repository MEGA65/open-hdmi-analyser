#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct tmds_ref {
  int code;
  int is_pixel;
  int value;
  int is_terc;
  int terc;
};

struct tmds_ref tmds_refs[]=
  {
#include "tmds_table.h"
   {-1,-1,-1,-1,-1}
  };

struct mode_line {
  int x_res,y_res,frame_rate;
  float pixel_clock_mhz;
  int active_x,hsync_start,hsync_end,h_total;
  int active_y,vsync_start,vsync_end,v_total;
  int hsync_pol,vsync_pol,interlaced;
};

struct mode_line modelines[]
={
  {640,480,60,25.17,640,656,752,800,480,490,492,525,-1,-1,0},
  {720,480,60,13.50,720,739,801,858,480,488,494,524,-1,-1,1},
  {720,480,60,27.00,720,736,798,858,480,489,495,525,-1,-1,0},
  {720,576,50,13.50,720,732,795,864,576,580,586,624,-1,-1,1},
  {720,576,50,27.00,720,732,796,864,576,581,586,625,-1,-1,0},
  {800,600,60,40.00,800,840,968,1056,600,601,605,628,+1,+1,0},
  {1024,768,60,65.00,1024,1048,1184,1344,768,771,777,806,-1,-1,0},
  {1280,720,50,74.25,1280,1720,1760,1980,720,725,730,750,+1,+1,0},
  {1280,720,60,74.18,1280,1390,1430,1650,720,725,730,750,+1,+1,0},
  {1280,720,60,74.25,1280,1390,1430,1650,720,725,730,750,+1,+1,0},
  {1280,768,60,68.25,1280,1328,1360,1440,768,771,778,790,+1,+1,0},
  {1280,1024,60,108.00,1280,1328,1440,1688,1024,1025,1028,1066,+1,+1,0},
  {1360,768,60,85.50,1360,1424,1536,1792,768,771,778,795,-1,-1,0},
  {1440,480,60,27.00,1440,1478,1602,1716,480,488,494,524,-1,-1,1},
  {1440,576,50,27.00,1440,1464,1590,1728,576,580,586,624,-1,-1,1},
  {1920,1080,24,74.16,1920,2558,2602,2750,1080,1084,1089,1125,+1,+1,0},
  {1920,1080,25,74.25,1920,2448,2492,2640,1080,1084,1089,1125,+1,+1,0},
  {1920,1080,30,89.01,1920,2448,2492,2640,1080,1084,1089,1125,+1,+1,0},
  {1920,1080,50,148.50,1920,2448,2492,2640,1080,1084,1089,1125,+1,+1,0},
  {1920,1080,50,74.25,1920,2448,2492,2640,1080,1084,1094,1124,+1,+1,1},
  {1920,1080,60,148.35,1920,2008,2052,2200,1080,1084,1089,1125,+1,+1,0},
  {1920,1080,60,148.50,1920,2008,2052,2200,1080,1084,1088,1125,-1,-1,0},
  {1920,1080,60,74.18,1920,2008,2052,2200,1080,1084,1094,1124,+1,+1,1},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}
};


#define D_MAX (1048576*64)
int d[3][D_MAX];
int d_len=0;

char bs_out[17];
char *binstr(unsigned int v)
{
  for(int i=9;i>=0;i--) {
    if (v&(1<<i)) bs_out[9-i]='1'; else bs_out[9-i]='0';
  }
  bs_out[16]=0;
  return bs_out;
}

void check_if_same(int *ref,int new)
{
  if (*ref!=new) *ref=-1;
}

void check_value(int *err,int a, int b,char *name)
{
  int d=a-b;
  int e=d*d;
  if (e&&name) {
    fprintf(stdout,"      %s: saw %d, expected %d\n",name,a,b);
  }
  *err+=e;
}

int dvi_is_pixel[1024];
int dvi_is_control[1024];
int dvi_is_data[1024];
int dvi_is_guard[1024];
int dvi_is_valid[1024];
int dvi_terc_value[1024];
int dvi_value[1024];

int dvi_counts[1024]={0};

int count_ones(int v)
{
  int count=0;
  for(int i=1;i<9;i++) {
    int one=(v>>i)&1;
    if (one) count++;
  }
  return count;
}

int flip10(int v)
{
  int o=0;
  for(int n=0;n<10;n++)
    o|=(v&(1<<n))?(1<<(9-n)):0;

  //  fprintf(stdout,"DEBUG: %s flips to",binstr(v));
  //  fprintf(stdout," %s\n",binstr(o));
  
  return o;
}

int decode_10b(int c)
{
  //  fprintf(stderr,"decode_10b(0x%03x)\n",c);
  int d=0;
  int v=c&0xff;
  if (c&0x200) {
    v=(c&0xff)^0x1ff;
  }
  d=v&1;
  if (c&0x100) {
    // XOR
    for(int i=1;i<8;i++) {
      int b=(v&(1<<i))?1:0;
      int db=(v&(1<<(i-1)))?1:0;
      //      fprintf(stderr,"bit %d: %d XOR %d = %d\n",i,b,db,b^db);
      b=b^db;
      d|=(b<<i);
    }
  } else {
    //    fprintf(stderr,"XNOR\n");
    // XNOR
    for(int i=7;i>0;i--) {
      int b=(v&(1<<i))?1:0;
      int db=(v&(1<<(i-1)))?1:0;
      //      fprintf(stderr,"bit %d: %d XNOR %d = %d\n",i,b,db,b^db^1);
      b=(b^db)^1;
      d|=(b<<i);
    }
  }
  return d;
}

int calc_dvi_code_table(void)
{
  // Clear table
  for(int i=0;i<1024;i++) {
    dvi_is_pixel[i]=0;
    dvi_is_control[i]=0;
    dvi_is_data[i]=0;
    dvi_is_guard[i]=0;
    dvi_is_valid[i]=0;
    dvi_terc_value[i]=0;
    dvi_value[i]=0;
  }

  // Guard bands for each channel
  // channel 0 = 1011001100 = $2cc
  // channel 1 = 0100110011 = $133
  // channel 2 = 1011001100 = $2cc
  // (same as channel 0)
  dvi_is_guard[0x2cc]=1; dvi_value[0x2cc]=0;
  dvi_is_guard[0x133]=1; dvi_value[0x133]=0;
  
  
#if 0  
  // 4-bit TERC data words
  // 0 -> $29c = 1010011100
  int terc4_words[16]={0x29c,0x263,0x2e4,0x2e2,0x171,0x11e,0x18e,0x13c,
		       0x2cc,0x139,0x19c,0x2c6,0x28e,0x271,0x163,0x2c3};

  for(int i=0;i<16;i++) {
    int v=terc4_words[i];
    // Don't set valid so that we don't get upset later about the TERC values that
    // are overloading normal pixel byte values
    //    dvi_is_valid[v]=1;
    dvi_is_data[v]=1;
    dvi_terc_value[v]=i;
  }
#endif
  
  // Control words  

  /*
    H V   WORD
    0 0 = 0010101011
    0 1 = 0010101010
    1 0 = 1101010100
    1 1 = 1101010101

  */
  int ctrl_codes[4]={0x2ab,0x0ab,0x154,0x354};
  for(int i=0;i<4;i++)
    {
      int c=flip10(ctrl_codes[i]);
      dvi_is_control[c]=1;
      dvi_is_valid[c]=1;
    }

  // Load the TMDS decode table calculated by Hamster.
  for(int i=0;tmds_refs[i].code!=-1;i++) {
    int c=flip10(tmds_refs[i].code);
    if (tmds_refs[i].is_pixel) {
      dvi_is_valid[c]=1;
      dvi_is_pixel[c]=1;
      dvi_value[c]=tmds_refs[i].value;
    }
    if (tmds_refs[i].is_terc) {
      dvi_is_data[c]=1;
      dvi_terc_value[c]=tmds_refs[i].terc;
    }
  }
  
  int count=0;
  for(int i=0;i<1024;i++)
    if (dvi_is_pixel[i]) {
      count++;
    }
  fprintf(stderr,"DEBUG: %d unique pixel values\n",count);

  return 0;
}

int analyse_bad_10b(int c)
{
  int d=decode_10b(c);
  fprintf(stdout,"                  Code decodes to 0x%02x\n",d);
  for(int j=0;j<1024;j++) {
    if (dvi_is_data[j]&&dvi_value[j]==d)
      fprintf(stdout,"                    Canonical code is 0x%03x (%s)\n",
	      j,binstr(j));
  }
  return 0;
}

int main(int argc,char **argv)
{
  if (argc!=2) {
    fprintf(stderr,"usage: hist <file.cap>\n");
    exit(-1);
  }

  calc_dvi_code_table();
  
  FILE *f=fopen(argv[1],"rb");
  if (!f) {
    perror("fopen");
    exit(-1);
  }
  
  unsigned char buff[8];

  int flip_check[1024]={0};
  for(int i=0;i<1024;i++) {
    int f=flip10(i);
    if (flip_check[f]) {
      fprintf(stderr,"ERROR: Multiple tokens flip to 0x%03x (%s), including 0x%03x\n",f,binstr(f),i);
      exit(-1);
    }
    flip_check[f]++;
  }
  fprintf(stdout,"DEBUG: Flip test passed.\n");
  
  int count=0;
  int n=fread(buff,1,8,f);
  int invalid=0;
  while(n==8) {
    
    if (count<D_MAX) {
      // File format has bit order reversed, so fix that now.
      d[0][count]=flip10((buff[0]<<2)+(buff[1]>>6));
      d[1][count]=flip10(((buff[1]<<6)&0x3c0)+(buff[2]>>2));
      d[2][count]=flip10((buff[3]<<2)+(buff[4]>>6));
      d_len=count;

      if (!dvi_is_valid[d[0][count]]) invalid++;
      if (!dvi_is_valid[d[1][count]]) invalid++;
      if (!dvi_is_valid[d[2][count]]) invalid++;

      dvi_counts[d[0][count]]++;
      dvi_counts[d[1][count]]++;
      dvi_counts[d[2][count]]++;
#if 0
      fprintf(stdout,"%7d: %03x %03x %03x\n",
	      count,d[0][count],d[1][count],d[2][count]);
#endif
    }

    count++;
    
    n=fread(buff,1,8,f);
  }
  fclose(f);
  
  fprintf(stdout,"DEBUG: Read %d records.\n",count);
  if (invalid) {
    fprintf(stdout,"ERROR: %d invalid DVI/TERC4 10-bit words observed:\n",invalid);
    for(int i=0;i<1024;i++) {
      if ((!dvi_is_valid[i])&&(dvi_counts[i])) {
	fprintf(stdout,"       %9dx $%03x (%s)\n",dvi_counts[i],i,binstr(i));
	analyse_bad_10b(i);
      }
    }
  }
	  
  if ((count-1)>d_len) {
    fprintf(stdout,"WARN: Using only the first 64M records (the first 512MiB of the file).\n");
    fprintf(stdout,"WARN: Ignoring the last %d records.\n",count-d_len);
  }

  /* 1. Attempt to identify the video mode by searching for VSYNC, HSYNC and H+VSYNC words.
     
     The DVI control words are:

     H V   WORD
     0 0 = 0010101011 = $0AB
     0 1 = 0010101010 = $0AA
     1 0 = 1101010100 = $354
     1 1 = 1101010101 = $355

     We need to keep track of runs of H-active/inactive and V-active/inactive runs, so
     that we can estimate the frame dimensions.
     
  */

  int ctrlrunlen[4]={0,0,0,0};
  int ctrlruns[4]={0,0,0,0};
  int ctrl_vals[4]={0x0ab,0x0aa,0x354,0x355};
  //int ctrl_vals[4]={0x2ab,0x0ab,0x154,0x354};

  int ctrlrunlen_bins[4][4096];
  for(int c=0;c<4;c++) for(int l=0;l<4096;l++) ctrlrunlen_bins[c][l]=0;
  
  int vsynclowrun=0;
  int vsynchighrun=0;
  int vsynclowbins[65536];
  int vsynchighbins[65536];
  int hsynclowrun=0;
  int hsynchighrun=0;
  int hsynclowbins[65536];
  int hsynchighbins[65536];
  int vsynclow_gap_history[3];
  int vsynchigh_gap_history[3];
  int hsynclow_gap_history[3];
  int hsynchigh_gap_history[3];
  int last_vsynclow_edge=0;
  int last_vsynchigh_edge=0;
  int last_hsynclow_edge=0;
  int last_hsynchigh_edge=0;
  int vsync_low_len=0;
  int vsync_high_len=0;
  int hsync_low_len=0;
  int hsync_high_len=0;
  int vsync_pol=-1;
  int hsync_pol=-1;
  int vsync_rasters=-1;
  int high_max=-1, low_max=-1, frame_count=-1;
  int hsync_len=-1;
  int vsync_len=-1;

  // Track the H/V sync values
  int hsync_state=0;
  int vsync_state=0;
  
  for(int i=0;i<65536;i++) {
    vsynclowbins[i]=0;
    vsynchighbins[i]=0;
    hsynclowbins[i]=0;
    hsynchighbins[i]=0;
  }
  
  for(int i=0;i<d_len;i++) {
    // Track HSYNC/VSYNC state
    if (d[0][i]==ctrl_vals[0]||d[0][i]==ctrl_vals[1]) hsync_state=0;
    if (d[0][i]==ctrl_vals[2]||d[0][i]==ctrl_vals[3]) hsync_state=1;
    if (d[0][i]==ctrl_vals[0]||d[0][i]==ctrl_vals[2]) vsync_state=0;
    if (d[0][i]==ctrl_vals[1]||d[0][i]==ctrl_vals[3]) vsync_state=1;

    for(int c=0;c<4;c++) {

      if (d[0][i]==ctrl_vals[c]) {
	if (!ctrlrunlen[c]) ctrlruns[c]++;
	ctrlrunlen[c]++;
      }
      else {
	if (ctrlrunlen[c]>4095) ctrlrunlen[c]=4095;	
	if (ctrlrunlen[c]) ctrlrunlen_bins[c][ctrlrunlen[c]]++;
	ctrlrunlen[c]=0;
      }
    }
    
    ////////////////////////////////////////////////////////////////
    //  Log VSYNC behaviour
    ////////////////////////////////////////////////////////////////
    if (!vsync_state) {
      // V-sync low
      vsynclowrun++;
      if (vsynchighrun) {

	// Keep track of gaps between VSYNC- edges
	int gap=i-last_vsynclow_edge;
	last_vsynclow_edge=i;
	for(int n=0;n<2;n++) vsynclow_gap_history[n]=vsynclow_gap_history[n+1];
	vsynclow_gap_history[2]=gap;
	
	if (vsynchighrun>0xffff) vsynchighrun=0xffff;
	vsynchighbins[vsynchighrun]++;
	vsynchighrun=0;	
      }
    }
    else if (vsync_state) {
      // V-sync high      
      vsynchighrun++;
      if (vsynclowrun) {
	// Keep track of gaps between VSYNC+ edges
	int gap=i-last_vsynchigh_edge;
	last_vsynchigh_edge=i;
	for(int n=0;n<2;n++) vsynchigh_gap_history[n]=vsynchigh_gap_history[n+1];
	vsynchigh_gap_history[2]=gap;
	
	if (vsynclowrun>0xffff) vsynclowrun=0xffff;
	vsynclowbins[vsynclowrun]++;
	vsynclowrun=0;	
      }
    }
    
    ////////////////////////////////////////////////////////////////
    //  Log HSYNC behaviour
    ////////////////////////////////////////////////////////////////
    if (!hsync_state) {
      // H-sync low
      hsynclowrun++;
      if (hsynchighrun) {
	// Keep track of gaps between HSYNC- edges
	int gap=i-last_hsynclow_edge;
	last_hsynclow_edge=i;
	for(int n=0;n<2;n++) hsynclow_gap_history[n]=hsynclow_gap_history[n+1];
	hsynclow_gap_history[2]=gap;
	
	if (hsynchighrun>0xffff) hsynchighrun=0xffff;
	hsynchighbins[hsynchighrun]++;
	hsynchighrun=0;	
      }
    }
    else if (hsync_state) {
      // H-sync high
      hsynchighrun++;
      if (hsynclowrun) {
	// Keep track of gaps between HSYNC+ edges
	int gap=i-last_hsynchigh_edge;
	last_hsynchigh_edge=i;
	for(int n=0;n<2;n++) hsynchigh_gap_history[n]=hsynchigh_gap_history[n+1];
	hsynchigh_gap_history[2]=gap;
	
	if (hsynclowrun>0xffff) hsynclowrun=0xffff;
	hsynclowbins[hsynclowrun]++;
	hsynclowrun=0;	
      }
    }
    
  }
  fprintf(stdout,"DEBUG: Saw control word counts: %d %d %d %d\n",
	  ctrlruns[0],ctrlruns[1],ctrlruns[2],ctrlruns[3]);
  fprintf(stdout,"DEBUG: Saw most frequent lengths of: ");
  for(int c=0;c<4;c++)
    {
      // H/V sync is only on channel 0
      int max=-1;
      int max_n=-1;
      for(int i=0;i<4096;i++) {
	if (ctrlrunlen_bins[c][i]>max) {
	  max=ctrlrunlen_bins[c][i];
	  max_n=i;
	}
      }
      fprintf(stdout,"%d(x%d) ",max_n,max);
    }
  fprintf(stdout,"\n");
  
  int max,max_n;
  
  fprintf(stdout,"DEBUG: HSYNC+ intervals = %d %d %d\n",
	  hsynchigh_gap_history[0],hsynchigh_gap_history[1],hsynchigh_gap_history[2]);
  fprintf(stdout,"DEBUG: HSYNC- intervals = %d %d %d\n",
	  hsynclow_gap_history[0],hsynclow_gap_history[1],hsynclow_gap_history[2]);
  fprintf(stdout,"DEBUG: VSYNC+ intervals = %d %d %d\n",
	  vsynchigh_gap_history[0],vsynchigh_gap_history[1],vsynchigh_gap_history[2]);
  fprintf(stdout,"DEBUG: VSYNC- intervals = %d %d %d\n",
	  vsynclow_gap_history[0],vsynclow_gap_history[1],vsynclow_gap_history[2]);

  int raster_len=hsynchigh_gap_history[0];
  
  for (int n=0;n<3;n++) {
    check_if_same(&raster_len,hsynchigh_gap_history[n]);
    check_if_same(&raster_len,hsynclow_gap_history[n]);
  }
  if (raster_len>0) {
    fprintf(stdout,"INFO: Raster lines are %d cycles long.\n",raster_len);
  } else {
    fprintf(stdout,"ERROR: Raster lines are not of consistent length.\n");
  }

  int frame_len=vsynchigh_gap_history[0];
  
  for (int n=0;n<3;n++) {
    check_if_same(&frame_len,vsynchigh_gap_history[n]);
    check_if_same(&frame_len,vsynclow_gap_history[n]);
  }
  if (frame_len>0) {
    fprintf(stdout,"INFO: Frames %d cycles long.\n",frame_len);
  } else {
    fprintf(stdout,"ERROR: Frames are not of consistent length.\n");
  }

  int rasters_per_frame=frame_len/raster_len;
  fprintf(stdout,"INFO: Frames consist of %d raster lines\n",rasters_per_frame);
  if ((rasters_per_frame*raster_len)!=frame_len) {
   fprintf(stdout,"ERROR: Frames are not an integer number of rasters long.\n");
  }

  fprintf(stdout,"DEBUG: Most frequent VSYNC low/high length = ");
  
  max=-1;
  max_n=-1;
  for(int i=0;i<65536;i++) {
    if (vsynclowbins[i]>max) {
      max=vsynclowbins[i];
      max_n=i;
    }
  }
  fprintf(stdout,"%d(x%d) ",max_n,max);
  low_max=max;
  vsync_low_len=max_n;

  max=-1;
  max_n=-1;
  for(int i=0;i<65536;i++) {
    if (vsynchighbins[i]>max) {
      max=vsynchighbins[i];
      max_n=i;
    }
  }
  fprintf(stdout,"%d(x%d) ",max_n,max);
  vsync_high_len=max_n;
  high_max=max;
  fprintf(stdout,"\n");

  fprintf(stdout,"DEBUG: VSYNC low -> %d / %d = %.2f,  VSYNC high -> %d / %d = %.2f\n",
       vsync_low_len,raster_len,vsync_low_len*1.0/raster_len,
       vsync_high_len,raster_len,vsync_high_len*1.0/raster_len);

  if (vsync_high_len<vsync_low_len) {
    vsync_pol=1;
    vsync_rasters=vsync_high_len/raster_len;
    vsync_len=vsync_high_len;
    frame_count = high_max;
  } else {
    vsync_pol=0;
    vsync_rasters=vsync_low_len/raster_len; 
    vsync_len=vsync_low_len;
    frame_count = low_max;
  }

  if (vsync_len%raster_len) {
    vsync_pol=-1;
    fprintf(stdout,"ERROR: VSYNC pulses don't seem to be a multiple of the raster length.\n");
  }
  fprintf(stdout,"INFO: VSYNC pulse lasts %d rasters, polarity is %s\n",
          vsync_rasters,(vsync_pol==1)?"POSITIVE":(vsync_pol==0?"NEGATIVE":"UNKNOWN"));
  fprintf(stdout,"INFO: File contains %d frames (first is possibly partial, and a last partial frame may also be present)\n",
          frame_count);

  fprintf(stdout,"DEBUG: Most frequent HSYNC low/high length = ");
  max=-1;
  max_n=-1;
  for(int i=0;i<65536;i++) {
    if (hsynclowbins[i]>max) {
      max=hsynclowbins[i];
      max_n=i;
    }
  }
  hsync_low_len=max_n;
  fprintf(stdout,"%d(x%d) ",max_n,max);

  max=-1;
  max_n=-1;
  for(int i=0;i<65536;i++) {
    if (hsynchighbins[i]>max) {
      max=hsynchighbins[i];
      max_n=i;
    }
  }
  hsync_high_len=max_n;
  fprintf(stdout,"%d(x%d) ",max_n,max);
  fprintf(stdout,"\n");
  if (hsync_high_len<hsync_low_len) hsync_len=hsync_high_len; else hsync_len=hsync_low_len;
  fprintf(stdout,"INFO: HSYNC duration is %d cycles\n",hsync_len);

  int best_mode=-1;
  int mode_error=-1;
  struct mode_line format;
  bzero(&format,sizeof(format));
  format.h_total=raster_len;
  format.v_total=rasters_per_frame;
  format.hsync_pol=hsync_pol?hsync_pol:-1;
  format.vsync_pol=vsync_pol?vsync_pol:-1;
  format.interlaced=0;

  for(int m=0;modelines[m].x_res>0;m++) {
    int this_error=0;
    check_value(&this_error,format.h_total,modelines[m].h_total,NULL);
    check_value(&this_error,format.v_total,modelines[m].v_total,NULL);
    check_value(&this_error,format.hsync_pol,modelines[m].vsync_pol,NULL);
    int mode_hsync_len=modelines[m].hsync_end-modelines[m].hsync_start;
    check_value(&this_error,hsync_len,mode_hsync_len,NULL);
    int mode_vsync_len=modelines[m].vsync_end-modelines[m].vsync_start;
    check_value(&this_error,vsync_rasters,mode_vsync_len,NULL);
#if 0
    fprintf(stdout,"DEBUG: Cumulative error = %d\n",this_error);

    fprintf(stdout,"DEBUG: Comparison to: %dx%d %dHz %sinterlaced (mode error = %d)\n",
	    modelines[m].x_res,modelines[m].y_res,
	    modelines[m].frame_rate,
	    modelines[m].interlaced?"":"non-",
	    this_error);
#endif
    if (mode_error==-1||this_error<mode_error) {
      mode_error=this_error;
      best_mode=m;
    }
  }

  fprintf(stdout,"INFO: Mode most closely matches %dx%d %dHz %sinterlaced (mode error = %d)\n",
	  modelines[best_mode].x_res,modelines[best_mode].y_res,
	  modelines[best_mode].frame_rate,
	  modelines[best_mode].interlaced?"":"non-",
	  mode_error);
  if (mode_error)
  {
    fprintf(stdout,"INFO: The mode differs from the expected mode in the following ways:\n");
    int this_error=0;
    check_value(&this_error,format.h_total,modelines[best_mode].h_total,"h_total");
    check_value(&this_error,format.v_total,modelines[best_mode].v_total,"v_total");
    check_value(&this_error,format.hsync_pol,modelines[best_mode].vsync_pol,"vsync_pol");
    int mode_hsync_len=modelines[best_mode].hsync_end-modelines[best_mode].hsync_start;
    check_value(&this_error,hsync_len,mode_hsync_len,"hsync_len");
    int mode_vsync_len=modelines[best_mode].vsync_end-modelines[best_mode].vsync_start;
    check_value(&this_error,vsync_rasters,mode_vsync_len,"vsync_len");
  }

  
}


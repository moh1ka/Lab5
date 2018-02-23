/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 5: Interrupt I/O

 				            ********* I N T I O. C **********

  Demonstrates inputing and outputing data from the DSK's audio port using interrupts. 

 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for CCS V4 Sept 10
 ************************************************************************************/
/*
 *	You should modify the code so that interrupts are used to service the 
 *  audio port.
 */
/**************************** Pre-processor statements ******************************/

#include <stdlib.h>
//  Included so program can make use of DSP/BIOS configuration tool.  
#include "dsp_bios_cfg.h"

/* The file dsk6713.h must be included in every program that uses the BSL.  This 
   example also includes dsk6713_aic23.h because it uses the 
   AIC23 codec module (audio interface). */
#include "dsk6713.h"
#include "dsk6713_aic23.h"

// math library (trig functions)
#include <math.h>

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>
#include "iir_coef.txt"

/******************************* Global declarations ********************************/
//M = sizeof(a)/sizeof(a[0])-1; //find the order of the filter 
//int N = M+1;
//x = (double*)calloc(N,sizeof(double));
//y = (double*)calloc(N,sizeof(double));
double x[N];
double y[N];
double output = 0.0;

double xs[] = {0.0};
double bs[] = {1/17, 1/17};
double a1 = 15/17;

double sum = 0.0;
int ptr = N-1;
/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /**********************************************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /**********************************************************************/\
    0x0017,  /* 0 LEFTINVOL  Left line input channel volume  0dB                   */\
    0x0017,  /* 1 RIGHTINVOL Right line input channel volume 0dB                   */\
    0x01f9,  /* 2 LEFTHPVOL  Left channel headphone volume   0dB                   */\
    0x01f9,  /* 3 RIGHTHPVOL Right channel headphone volume  0dB                   */\
    0x0011,  /* 4 ANAPATH    Analog audio path control       DAC on, Mic boost 20dB*/\
    0x0000,  /* 5 DIGPATH    Digital audio path control      All Filters off       */\
    0x0000,  /* 6 DPOWERDOWN Power down control              All Hardware on       */\
    0x0043,  /* 7 DIGIF      Digital audio interface format  16 bit                */\
    0x008d,  /* 8 SAMPLERATE Sample rate control             8 KHZ                 */\
    0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
			 /**********************************************************************/
};


// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

 /******************************* Function prototypes ********************************/
void init_hardware(void);     
void init_HWI(void);                  
void lab5(void); 
// ---------- implement filter ------
 
double single_pole_iir (void);

double iir_direct (void);
/********************************** Main routine ************************************/
void main()
{      
	int i;
	for( i=0;i<N;i++)
	{
		x[i]=0.0;
		y[i]=0.0;
	}
	// initialize board and the audio port
  init_hardware();
  /* initialize hardware interrupts */
  init_HWI();
  //sineinit();
  	 		
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
  {};
  
}
        
/********************************** init_hardware() **********************************/  
void init_hardware()
{
    // Initialize the board support library, must be called first 
    DSK6713_init();
    
    // Start the AIC23 codec using the settings defined above in config 
    H_Codec = DSK6713_AIC23_openCodec(0, &Config);

	/* Function below sets the number of bits in word used by MSBSP (serial port) for 
	receives from AIC23 (audio port). We are using a 32 bit packet containing two 
	16 bit numbers hence 32BIT is set for  receive */
	MCBSP_FSETS(RCR1, RWDLEN1, 32BIT);	

	/* Configures interrupt to activate on each consecutive available 32 bits 
	from Audio port hence an interrupt is generated for each L & R sample pair */	
	MCBSP_FSETS(SPCR1, RINTM, FRM);

	/* These commands do the same thing as above but applied to data transfers to  
	the audio port */
	MCBSP_FSETS(XCR1, XWDLEN1, 32BIT);	
	MCBSP_FSETS(SPCR1, XINTM, FRM);	
	
}

/********************************** init_HWI() **************************************/  
void init_HWI(void)
{
	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt (used by the debugger)
	IRQ_map(IRQ_EVT_RINT1,4);		// Maps an event to a physical interrupt (X for transmit, Ex2: R for recieve, Ex1)
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event (X for transmit, Ex2: R for recieve, Ex1)
	IRQ_globalEnable();				// Globally enables interrupts

} 

/******************** WRITE YOUR INTERRUPT SERVICE ROUTINE HERE***********************/  
/*
void lab4(void)
{
	mono_write_16Bit(non_cir());
	//mono_write_16Bit(non_cir_op_1());
	//mono_write_16Bit(non_cir_op_2());
	
	//mono_write_16Bit(cir());
	//mono_write_16Bit(cir_op_1());//original circular 
	//mono_write_16Bit(cir_op_2());//double size buffer 
}
*/
void lab5(void)
{
	mono_write_16Bit(single_pole_iir());
	//mono_write_16Bit(iir_direct());//double size buffer 
}

// ------------------------------------------------------------------------------
 
double single_pole_iir (void)
{	
	xs[1] = mono_read_16Bit();
	output = bs[0]*xs[1] + bs[1]*xs[0] - a1*output;
	xs[0] = xs[1];
	return output;
	
	/*xsing[1] = mono_read_16Bit(); //x[1] is most recent, x[0] oldest sample
	output = bsing[0]*xsing[1] + bsing[1]*xsing[0] + a1*output; //y[n-1] is overwritten by y[n]= y
	xsing[0] = xsing[1];
	return output;*/
	
}

double iir_direct (void)
{
	int i;
	double w = 0.0;
	double sum = 0.0;
	x[ptr] = mono_read_16Bit();
	
	//convolution
	for(i=0; i<N; i++)
	{
		int index = ptr+i;
			//make sure the index does not go below 0. when it does, warp it around
		if( index == N )
		{
			index = 0;
		}
		w += b[i]* x[index];
		sum += a[i]* y[index];  
	}
	
	y[ptr] = sum + w;
	
	if(ptr == 0)
	{
		ptr = N-1;
	}
	else
	{
		ptr--;
	}
	return y[ptr];
}



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
double *p,*w;
int ptr = N-1;

//Declarations for Low pass Filter
double output = 0.0;
double xs[] = {0.0, 0.0};	
double bs[] = {1.0/17.0, 1.0/17.0};	//b taps
double a1 = 15.0/17.0; 

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

// ------------------------------Function Definitions---------------------------
 
double single_pole_iir (void);
double iir_direct (void);
double iir_trans (void);

/********************************** Main routine ************************************/
void main()
{      
    int i;
    p = (double*)calloc(N, sizeof(double));
    w = (double*)calloc(N+1, sizeof(double));
    for(i=0;i<N;i++)
    {
	p[i]=0.0; //initializes p to zeros
  	w[i]=0.0; //initializes w[0] to w[N-1] to zero 
    }
	
     w[N] = 0.0; //now all of w is initialized to zero
	
    // initialize board and the audio port
    init_hardware();
    /* initialize hardware interrupts */
    init_HWI();
  	 		
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
void lab5(void)
{
	//mono_write_16Bit(single_pole_iir());
	//mono_write_16Bit(iir_direct());//double size buffer 
	//mono_write_16Bit(mono_read_16Bit());
	mono_write_16Bit(iir_trans());
}

// ------------------------------------------------------------------------------
double single_pole_iir (void)//y[n] = 1/17*x[n] + 1/17*x[n-1] - 15/17*y[n-1]
{	
	xs[1] =  mono_read_16Bit();
	//difference equaiton:
	output= bs[0]*xs[1] + bs[1]*xs[0] + a1*output;
	//delay the samples:
	xs[0] = xs[1];
	
	return output;	
}

double iir_direct (void)
{
	int i;
	double w = 0.0;
	double v = 0.0;
	double out = 0.0;
	p[ptr] = mono_read_16Bit();
	
	//MAC
	for(i=1; i<N; i++)
	{
		int index = ptr+i;
			//make sure the index does not go below 0. when it does, wrap it around
		if( index >= N )
		{
			index = (index % N);
		}
		v += a[i]*p[index];
		w += b[i]*p[index]; 
	}
	
	p[ptr] -= v;
	out = w + b[0]*p[ptr];
	
	if(ptr == 0)
	{
		ptr = N-1;
	}
	else
	{
		ptr--;
	}
	return out;
}


double iir_trans (void)
{
	int i;
	double xin;
	xin = mono_read_16Bit(); //xin = x(n)
	w[0] = w[1] + b[0]*xin; // w[0] = y(n) -> y(n)=w[1](prev) + b[0]*xin
	for(i=1; i<N; i++)
	{
		w[i] = b[i]*xin - a[i]*w[0] + w[i+1] ; // for i = N-2,...,1  w[i] = b[i]*xin -a[i]y(n) + w[i+1](prev)
		//for i = N-1 w[N-1] = b[N-1]- a[N-1]y(n) + w[N] where w[N]=0.0 always
	}

	return w[0];
}


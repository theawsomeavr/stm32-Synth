/*****************************************************************************/
/* USB MIDI Synth main File                                                  */
/*                                                                           */
/* Copyright (C) 2016 Laszlo Arvai                                           */
/* source: https://github.com/hightower70/MIDIUSB                            */
/* used fork: https://github.com/Cynthetika/MIDIUSB                          */
/* All rights reserved.                                                      */
/* Modified by David Rubio 08/06/2019                                        */
#define CHANNEL_COUNT 8
#include <stdlib.h>
#include <sample.h>
#include <sysMain.h>
#include <halHelpers.h>
#include <sysConfig.h>
#include <midiUSB.h>
#include <midiOutput.h>
#include <midiInput.h>
#include <drum.h>
//this definition is used for enabling an "analog" output suitable for an analog vumeter on PA1
//if not defined PA1 its going to be used as a mono output (with no panning)
#define use_analog_vumeter
//midi note to frequency array
const int freq[128] =
{
  16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41,
  44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110,
  117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233,
  247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047,
  1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729,
  3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040,
  7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544,
  13290, 14080, 14917, 15804, 16744, 17740, 18795, 19912, 21096,
  22351, 23680, 25088
};
//formula
// ((sampling rate of sample/freq of the note)/timer1 interrupt freq)*2^11
const float increment_per_hert=1.565599539;
//variables
long millis;
#ifdef use_analog_vumeter
int t;
bool wait_for_midi;
#endif
long lastDebounceTime;
bool chanused[CHANNEL_COUNT];
uint8_t volume[CHANNEL_COUNT];
uint8_t pan_r[CHANNEL_COUNT];
uint8_t pan_l[CHANNEL_COUNT];
uint8_t note[CHANNEL_COUNT];
uint8_t chanchannel[CHANNEL_COUNT];
uint8_t playednotes[CHANNEL_COUNT];
bool percussionused[2];
uint8_t playedpercussion[2];
long counter[CHANNEL_COUNT];
int count=0;
uint16_t count2=0;
bool sustain=0;
long percussion_counter[2];
unsigned int increment[CHANNEL_COUNT];
void handlechans(uint8_t mididata,uint8_t midichannel) {
  uint32_t maxValue=0;
  uint8_t big=0;
  //check if the note has already been played on any of the channels
  //if true replay it on the same channel and return
  for (int a = 0; a != CHANNEL_COUNT; a++) {
    if (mididata == playednotes[a]) {
      //write the midi channel of this note into the chanchannel variable
      chanchannel[a]=midichannel;
      //write the midinote of this note to the note variable (confusing?)
      note[a]=mididata;
      //set the counter to 0 so that the playback starts again
      counter[a] = 0;
      //write the increment needed for the frequency that we want
      increment[a] = increment_per_hert*freq[mididata];
      //set the chanused boolean
      chanused[a] = 1;
      return;
    }
  }
  //check which channel is free so that we can play a note on it
  for (int a = 0; a != CHANNEL_COUNT; a++) {
    if (!chanused[a]) {
      chanchannel[a]=midichannel;
      note[a]=mididata;
      counter[a] = 0;
      increment[a] = increment_per_hert*freq[mididata];
      chanused[a] = 1;
      playednotes[a] = mididata;
      return;
    }
    //check which note is the oldest that has been played
    if (counter[a] > maxValue) {
      maxValue = counter[a];
      big = a;
    }
  }
  //if no channel is free overwrite the oldest one
  note[big]=mididata;
  chanchannel[big]=midichannel;
  counter[big] = 0;
  increment[big] = increment_per_hert*freq[mididata];
  chanused[big] = 1;
  playednotes[big] = mididata;

}
// same as handlechans but for percussion
void handlepercussion(uint8_t mididata) {
  uint32_t maxValue=0;
  uint8_t big=0;
  //check if the note has already been played on any of the channels
  //if true replay it on the same channel and return
  for (int a = 0; a != 2; a++) {
    if (mididata == playedpercussion[a]) {
    	//i know i could have used a switch function but anyway if conditions still work fine
    	if(mididata==35||mididata==36){
    						drum_sample[a]=kick;
    						drum_length[a]=sizeof(kick);
    					}
    						else if(mididata==37||mididata==39){
    						drum_sample[a]=clap;
    						drum_length[a]=sizeof(clap);
    					}
    						else if(mididata==38||mididata==40||mididata==41||mididata==43){
    						drum_sample[a]=snare;
    						drum_length[a]=sizeof(snare);
    					}
    						else if(mididata==42||mididata==39){
    						drum_sample[a]=close_hi_hat;
    						drum_length[a]=sizeof(close_hi_hat);
    					}
    						else if(mididata==44||mididata==46){
    						drum_sample[a]=open_hi_hat;
    						drum_length[a]=sizeof(open_hi_hat);
    					}
    						else if(mididata==49||mididata==51||mididata==52||mididata==55||mididata==57||mididata==59){
    						drum_sample[a]=splash;
    						drum_length[a]=sizeof(splash);
    					}
    						else{
    						drum_sample[a]=clap;
    						drum_length[a]=sizeof(clap);
    						}
    						percussion_counter[a]=0;
    						percussionused[a] = 1;
      return;
    }
  }
  //check which channel is free so that we can play a note on it
  for (int a = 0; a != 2; a++) {
    if (!percussionused[a]) {
    	if(mididata==35||mididata==36){
    						drum_sample[a]=kick;
    						drum_length[a]=sizeof(kick);
    					}
    						else if(mididata==37||mididata==39){
    						drum_sample[a]=clap;
    						drum_length[a]=sizeof(clap);
    					}
    						else if(mididata==38||mididata==40||mididata==41||mididata==43){
    						drum_sample[a]=snare;
    						drum_length[a]=sizeof(snare);
    					}
    						else if(mididata==42||mididata==39){
    						drum_sample[a]=close_hi_hat;
    						drum_length[a]=sizeof(close_hi_hat);
    					}
    						else if(mididata==44||mididata==46){
    						drum_sample[a]=open_hi_hat;
    						drum_length[a]=sizeof(open_hi_hat);
    					}
    						else if(mididata==49||mididata==51||mididata==52||mididata==55||mididata==57||mididata==59){
    						drum_sample[a]=splash;
    						drum_length[a]=sizeof(splash);
    					}
    						else{
    						drum_sample[a]=clap;
    						drum_length[a]=sizeof(clap);
    						}
    						percussion_counter[a]=0;
    						percussionused[a] = 1;
      playedpercussion[a] = mididata;
      return;
    }
    //check which note is the oldest that has been played
    if (percussion_counter[a] > maxValue) {
      maxValue = percussion_counter[a];
      big = a;
    }
  }
  //if no channel is free overwrite the oldest one

	if(mididata==35||mididata==36){
						drum_sample[big]=kick;
						drum_length[big]=sizeof(kick);
					}
						else if(mididata==37||mididata==39){
						drum_sample[big]=clap;
						drum_length[big]=sizeof(clap);
					}
						else if(mididata==38||mididata==40||mididata==41||mididata==43){
						drum_sample[big]=snare;
						drum_length[big]=sizeof(snare);
					}
						else if(mididata==42||mididata==39){
						drum_sample[big]=close_hi_hat;
						drum_length[big]=sizeof(close_hi_hat);
					}
						else if(mididata==44||mididata==46){
						drum_sample[big]=open_hi_hat;
						drum_length[big]=sizeof(open_hi_hat);
					}
						else if(mididata==49||mididata==51||mididata==52||mididata==55||mididata==57||mididata==59){
						drum_sample[big]=splash;
						drum_length[big]=sizeof(splash);
					}
						else{
						drum_sample[big]=clap;
						drum_length[big]=sizeof(clap);
						}
						percussion_counter[big]=0;
						percussionused[big] = 1;
  playedpercussion[big] = mididata;

}
void appMainTask(void)
{
	//set the percussion counter higher than the sample length (which is 0, so that 1 > 0)
	percussion_counter[0]=1;
	percussion_counter[1]=1;
	//set the volume and panning to its normal value on all channel generators
	for(int a=0;a!=CHANNEL_COUNT;a++){
		volume[a]=128;
		pan_l[a]=64;
		pan_r[a]=64;
		//put channel indicator of the channel generators (thats a lot of channels) out of range so that we do not write data to it accidentally
		chanchannel[a]=255;
	}
	//set the clock signal to all of these registers (GPIO port C,B ; Timer1,2 ; Alternative Function IO)
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_AFIOEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN;
	//Reset GPIO registers
	GPIOC ->CRH=0;
	GPIOB ->CRL=0;
	GPIOB ->CRH=0;
	//set pin PC13 to output push-pull
	//set pin PC14 to an input with internal resistor
	GPIOC ->CRH |=GPIO_CRH_MODE13_0 | GPIO_CRH_CNF14_1;
	//set pin PB11 and PB10 to Alternative Function Output push-pull
	GPIOB ->CRH |= GPIO_CRH_CNF11_1 | GPIO_CRH_MODE11_0 | GPIO_CRH_CNF10_1 | GPIO_CRH_MODE10_0;
	//set pin PA1 as an Alternative Function Output push-pull
	GPIOA ->CRL = GPIO_CRL_CNF1_1 | GPIO_CRL_MODE1_0 ;
	//set the internal pull-up resistor of PC14
	GPIOC ->ODR |= GPIO_ODR_ODR14;
	TIM1->PSC=100; //timer1 prescaler set to 100
	TIM1->ARR=18; //timer1 Auto reload set to 18
	TIM1->DIER = TIM_DIER_UIE; // Timer1 update interrupt enable
	TIM3->PSC=720; //timer1 prescaler set to 720
	TIM3->ARR=100; //timer1 Auto reload set to 100
	TIM3->DIER = TIM_DIER_UIE; // Timer3 interrupt enable
	TIM2->PSC = 1; //timer2 prescaler set to 1
	TIM2->ARR = 1023; //timer1 Auto reload set to 1023 (this is the PWM resolution 10bit)
	AFIO->MAPR = AFIO_MAPR_TIM2_REMAP_1; //use the alternative function io outputs for timer2
	TIM2->CCMR2 = TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1; // PWM mode on channel 4 and 3
	TIM2->CCMR1 = TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1; // PWM mode on channel 2
	TIM2->CCER = TIM_CCER_CC4E | TIM_CCER_CC3E | TIM_CCER_CC2E; // Enable compare on channel 4, 3 and 2
	AFIO ->EXTICR[3] |= AFIO_EXTICR4_EXTI14_PC; //enable the external interrupt function of PC14
	EXTI->IMR|=EXTI_IMR_MR14; //enable interrupt triggering of MR14 (aka PA14,PB14,PC14. etc...)
	EXTI->FTSR |= EXTI_FTSR_TR14;//make it trigger by a falling edge of the MR14 mask (only PC14 is used)
	NVIC_EnableIRQ(TIM3_IRQn);//enable timer3 interrupt
	NVIC_EnableIRQ(EXTI15_10_IRQn);//enable external interrupts from 10-15 interrupt
	NVIC_SetPriority(EXTI15_10_IRQn,0);//set it to the highest priority
	NVIC_EnableIRQ(TIM1_UP_IRQn); // enable timer1 interrupt update
	while(!midiIsConnected())midiUSBTask(); //wait for the device to be recognized by the computer
	TIM1->CR1 = TIM_CR1_CEN;  //enable timer1
	TIM2->CR1 = TIM_CR1_CEN;  //enable timer2
	TIM3->CR1 = TIM_CR1_CEN;  //enable timer3
	while(1){
	uint8_t mididata1[4];
	//check if there is incoming midi data from the USB port
		if (!midiOutputIsEmpty())
		{
			//read the midi data
		midiOutput(&mididata1[0],&mididata1[1],&mididata1[2],&mididata1[3]);
        //pitch bend routine
		//check if this isn´t comming from channel 10
		if(mididata1[0]==CIN_PITCH_BEND && mididata1[1]!=0xE9){
			for(int a=0;a!=CHANNEL_COUNT;a++){
				//remember the chanchannel variable?
				//well, here it is used for checking on which channel generetors we are going to apply the pitch bend
				if(chanchannel[a]==(mididata1[1] & 0x0F)){
					//if the high byte of the pitch bend controller is 64 (at the middle)
					//just write the original freq, while skipping the next mumbo jumbo
					if(note[a]){
					if(mididata1[3]==0x40){
						increment[a]=increment_per_hert*freq[note[a]];
					}
					else{
						//get the difference between the range of the pitch bend according to the note of the channel generetor (range of the pitch bend is +/- 2 semitones/midi notes)
					volatile int difference=freq[note[a]+2]-freq[note[a]-2];
					//oh my, this looks quite complex
					//let me drink some cheetah chug
					//multiply the difference by the pitch bend value (range is from 0-127)
					//then just divided by 127 making this a rule of 3
					//add the lower freq (freq[note[a]-2]) so that we are on the correct one
					//finally multiply this by the increment per hert constant
					increment[a]=increment_per_hert*(((difference*mididata1[3])>>7)+freq[note[a]-2]);
					}
					}
				}
			}
		}
        //control change data from the usb midi (ignore channel 10 aka percussion)
		if(mididata1[0]==CIN_CONTROL_CHANGE && mididata1[1]!=0xB9){
			//sustain
			if(mididata1[2]==0x40){
				//if the sustain knob if higher than 50, turn on sustain
				if(mididata1[3]>50)sustain=1;
				else sustain=0;
			}
			//volume control
			if(mididata1[2]==0x07){
				for(int a=0;a!=CHANNEL_COUNT;a++){
					//check on which channel generetor we are going to apply the volume
					if(chanchannel[a]==(mididata1[1] & 0x0F))volume[a]=mididata1[3];
				}
			}
			//pan control
			if(mididata1[2]==0x0A){
				for(int a=0;a!=CHANNEL_COUNT;a++){
					//check on which channel generetor we are going to apply the pan value
					if(chanchannel[a]==(mididata1[1] & 0x0F)){
						//create a bool that i going to indicate when the pan value is higher that 63
			            bool is_it_higher= (mididata1[3] & 64) >> 6;
			            //lots of simple operations for both pan channels
			            pan_l[a] = 64-((mididata1[3]-64)*is_it_higher);
			            pan_r[a] = (mididata1[3] * (!is_it_higher))+(64*is_it_higher);
					}
				}
			}
			//all notes off
			if(mididata1[2]==0x78){
				//this is self explanatory
				for(int a=0;a!=CHANNEL_COUNT;a++){
				//check on which channel generator to silence the playback of the notes
			    if(chanchannel[a]==(mididata1[1] & 0x0F)){
			    //put it out of range so that we do not write data to it accidentally
			    chanchannel[a]=255;
			    counter[a] = size<<11;
				chanused[a] = 0;
				note[a]=0;
				}
				}
			}
			//all controls reset
			if(mididata1[2]==0x79){
				for(int a=0;a!=CHANNEL_COUNT;a++){
					//check on which channel generator to reset the controls
					//put it out of range so that we do not write data to it accidentally
					chanchannel[a]=255;
					if(chanchannel[a]==(mididata1[1] & 0x0F)){
					volume[a]=128;
					pan_l[a]=64;
					pan_r[a]=64;
					}
			}
				sustain=0;
		  }
		}
		//if there is no sustain, stop the playing of the sample of the specified channel (ignore channel 10 aka percussion)
			if((mididata1[0]==CIN_NOTE_OFF || (mididata1[0]==CIN_NOTE_ON && !mididata1[3])) && mididata1[1]!=0x89 && !sustain){
				  for (int a = 0; a != CHANNEL_COUNT; a++) {
				    if (mididata1[2]-11 == playednotes[a]) {
				    	//Note: there are bit-shifts (<<) in here since the increment per hert variable
				    	//has been multiplied by 2^11 to avoid using floats and thus making the calculations way faster at the interrupt routine
				      counter[a] = size<<11;
				      chanused[a] = 0;
				      break;
				    }
				  }

			}
			// play the incoming midi note (ignore channel 10 aka percussion)
			if(mididata1[0]==CIN_NOTE_ON && mididata1[3]){
				//set the built in led pin (PC13) to low to turn it on
				GPIOC ->ODR &= ~ GPIO_ODR_ODR13;
                #ifdef use_analog_vumeter
                //set the wait_for_midi bool to one
				if(!wait_for_midi)wait_for_midi = 1;
				#endif
				if(mididata1[1]!=0x99){
				handlechans(mididata1[2]-11,(mididata1[1] & 0x0F));
				}

				else handlepercussion(mididata1[2]);

			}


		}
}
}

void EXTI15_10_IRQHandler(){
	if(EXTI->PR & EXTI_PR_PR14){
		//reset the external interrupt flag (PR14)
		EXTI->PR |= EXTI_PR_PR14;
		//check if PC14 is low
		if(!(GPIOC ->IDR & GPIO_IDR_IDR14)){
			//check if the input has stay of for more than 150 millis for de-bouncing
				  if ((millis - lastDebounceTime) > 150)
				  {
				    lastDebounceTime = millis;
				    //change the value of the sustain bool
				    sustain = !sustain;
				  }
				}
	}
}
void TIM3_IRQHandler(){
	if(TIM3->SR & TIM_SR_UIF) // if UIF flag is set
	  {
		 TIM3->SR &= ~TIM_SR_UIF;
		 //20 millisecond "timer" for the USB midi interface
		 count2++;
		 if(count2==20){
			 count2=0;
			 midiUSBTask(); //handle USB stuff
		 }
		 //if the PC13 led is on start counting
		 if(!(GPIOC ->ODR & GPIO_ODR_ODR13)){
			 count++;
		 }
		 //if the led counter reached 80 millis turn off the PC13 led
		 if(count==80){
			 count=0;
			 GPIOC ->ODR |=GPIO_ODR_ODR13;
		 }
         millis++;
	  }
}
void TIM1_UP_IRQHandler(void)
{
	//timer interrupt, this happens 40000 times per second since 72000000(F_CPU)/prescaler/ARR=40000
if(TIM1->SR & TIM_SR_UIF) // if UIF flag is set
  {

	 TIM1->SR &= ~TIM_SR_UIF; //clear the UIF flag for not getting stuck on this interrupt
	 //offsets
	 volatile uint16_t right=512;
	 volatile uint16_t left=512;
     volatile uint16_t no_panning=512;
	 //audio channel mixing and pcm playing
	 for(int a=0;a!=CHANNEL_COUNT;a++){
		 //add the increment of the channel to the counter of itself, the store that value in a volatile variable
	 volatile uint32_t temp=counter[a]+increment[a];
	 //Note: there are bit-shifts (>>) in here since the increment per hert variable has been multiplied by 2^11 to avoid using floats and thus making the calculations way faster
	 //if we still don´t reach the end off the sample
	if((temp>>11)<size){
	//put the previous sum to the channel counter
	counter[a]=temp;
	//remove the dc offset of the sample (from 0 to 255 to -127 to 127)
	//get the value of the sample that we need according to the counter of a channel
	//add that value in to the n variable
	//multiplied it by the volume (0-127) then dividing it by 127 and then by 3 since there are 12 channels so that we get 1/3 of the volume thus it will not clip
	//add the panning to each channel
	//put all of these on a variable so that we do not have to recalculate all of these values
	volatile uint32_t chan_volume=((data[counter[a]>>11]-128)*volume[a]);
 	no_panning+=chan_volume>>8;
	//apply the panning values in to the respective output
	right+=(chan_volume*pan_r[a])>>14;
	left+=(chan_volume*pan_l[a])>>14;
	}

	//if we reach the end of the sample set the chanused boolean to 0 of the specific channel
	else if (chanused[a]){
		chanused[a]=0;
		//reset its pan values
		pan_l[a]=64;
		pan_r[a]=64;
	}
	}
	 //handle the percussion channels (only 2)
	 for(int a=0;a!=2;a++){
    //if the counter the percussion has not reach the end of the sample continue
    if(percussion_counter[a]<drum_length[a]*2){
    	//check if the number is not odd
    	if(!(percussion_counter[a] % 2)){
    		//get the sample value of the pointer (*drumsample)
    		//since the interrupt freq is 40khz and this samples are 20khz just divide the counter by 2
    		//remove the dc offset
    		//make them half of the volume so that it does not clip the audio
    		//put all of these on a variable so that we do not have to recalculate all of these values
    		volatile int percussion_sound =(*(drum_sample[a]+(percussion_counter[a]/2))-128)>>1;
    		//give the same volume for all outputs
    	    no_panning+=percussion_sound;
    		right+=percussion_sound;
    		left+=percussion_sound;
    	}
    	//increase the percussion_counter
    	percussion_counter[a]++;
    }
    //if we reach the end of the sample set the percussionused boolean to 0 of the specific channel
    else if (percussionused[a]) percussionused[a]=0;
	 }
	 //write the n variable in to the pwm chan4 register
	 TIM2->CCR4=right;
	 TIM2->CCR3=left;
#ifdef use_analog_vumeter
	 if(wait_for_midi){
	 t++;
	 if(t==5){
		t=0;
		 volatile int vumeter=no_panning-512;
		 if(vumeter>0){
		 TIM2->CCR2=vumeter<<5;
		 }
		 else TIM2->CCR2=(vumeter*-1)<<5;
		 if(vumeter==0)TIM2->CCR2=0;
	 }
	 }
#else
	 TIM2->CCR2=no_panning;
#endif

  }
}

//end of file
//thanks for reading all the comments (if you did)
//hopefully i make it clear how everything works in here (except for USB)



/*
 *  These math formulas were discovered by Viznut a.k.a. Ville-Matias Heikkilä and his friends.
 *
 *  Find more information at:
 *
 *  http://viznut.fi/texts-en/bytebeat_algorithmic_symphonies.html
 *  http://viznut.fi/en/
 *
 */

/*
if(bytebeat_song==0)
{
	tt[0] = bytebeat_song_ptr*2;

	tt[1] = tt[0]/2;

	#define t tt[0]
	s[0] = ((t*((t>>13&1)+1)*((-t>>16&3)+8)/32&t*((t>>13&1)+1)*((-t>>16&3)+8)/32&64)+(t*((t>>13&1)+1)*((-t>>16&3)+8)/16&t*((t>>13&1)+1)*((-t>>16&3)+8)/16&32)+(t*((-t>>13&3)+1)/13&24)+(t*((((t>>13&t>>14&3)|(-t>>16&1))+4)+1)/13&48));
	#define t tt[1]
	s[1] = ((t*((t>>13&1)+1)*((-t>>16&3)+8)/32&t*((t>>13&1)+1)*((-t>>16&3)+8)/32&64)+(t*((t>>13&1)+1)*((-t>>16&3)+8)/16&t*((t>>13&1)+1)*((-t>>16&3)+8)/16&32)+(t*((-t>>13&3)+1)/13&24)+(t*((((t>>13&t>>14&3)|(-t>>16&1))+4)+1)/13&48));
	#undef t

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
*/
if(bytebeat_song==0)
{
	tt[0] = bytebeat_song_ptr/bit1;//4

	tt[1] = tt[0]/bit2;//2

	s[0] = tt[0]*(tt[0]>>(11)&tt[0]>>(8-var_p[0]/5)&(123-var_p[2])&tt[0]>>(3+var_p[3]/10))*(var_p[1]+1);
	s[1] = tt[1]*(tt[1]>>(11)&tt[1]>>(8-var_p[0]/5)&(123-var_p[2])&tt[1]>>(3+var_p[3]/10))*(var_p[1]+1);

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
else if(bytebeat_song==1)
{
	tt[0] = bytebeat_song_ptr/bit1;//4;///6;

	//t[1] = t[0];
	tt[1] = tt[0]/bit2;//2;
	//t[1] = t[0]/4;

	//tt[2] = bytebeat_song_ptr/(3+var_p[2]/10);
	//tt[3] = tt[1]/2;

	s[0] = (tt[0]*(tt[0]>>(5-var_p[0]/20)|tt[0]>>(8-var_p[1]/20)))>>(tt[0]>>(16-var_p[3]/10));
	s[1] = (tt[1]*(tt[1]>>(5-var_p[0]/20)|tt[1]>>(8-var_p[1]/20)))>>(tt[1]>>(16-var_p[3]/10));
	//s[2] = (tt[2]*(tt[2]>>(5+var_p[0]/10)|tt[2]>>(8+var_p[1]/10)))>>(tt[2]>>(16-var_p[3]/10));
	//s[3] = (tt[3]*(tt[3]>>(5+var_p[0]/10)|tt[3]>>(8+var_p[1]/10)))>>(tt[3]>>(16-var_p[3]/10));

	//mix1 += (float)(s[0]+s[2]);
	//mix2 += (float)(s[1]+s[3]);
	mix1 += (float)(s[0]);
	mix2 += (float)(s[1]);
}
else if(bytebeat_song==2)
{
	tt[0] = bytebeat_song_ptr/bit1;//4;

	//t[1] = t[0];
	tt[1] = tt[0]/bit2;//2;
	//t[1] = t[0]/4;

	//t[2] = t[1]/2;
	//t[3] = t[2]/2;

	s[0] = tt[0]*((tt[0]>>(12-var_p[0]/10)|tt[0]>>(8-var_p[1]/2))&(63-var_p[2])&tt[0]>>(4+var_p[3]/8));
	s[1] = tt[1]*((tt[1]>>(12-var_p[0]/10)|tt[1]>>(8+var_p[1]/2))&(63-var_p[2])&tt[1]>>(4+var_p[3]/8));
	//s[2] = t[2]*((t[2]>>12|t[2]>>8)&63&t[2]>>4);
	//s[3] = t[3]*((t[3]>>12|t[3]>>8)&63&t[3]>>4);

	mix1 += (float)s[0];
	mix2 += (float)s[1];
	//mix1 += (float)(s[0]+s[2]);
	//mix2 += (float)(s[1]+s[3]);
}
else if(bytebeat_song==3)
{
	tt[0] = bytebeat_song_ptr/bit1;//3;

	tt[1] = tt[0]/bit2;//2;

	s[0] = tt[0]*((tt[0]>>(9-var_p[0]/10)|tt[0]>>(13-var_p[1]/8))&(25-var_p[3]/4)&tt[0]>>(6-var_p[2]/20));
	s[1] = tt[1]*((tt[1]>>(9-var_p[0]/10)|tt[1]>>(13-var_p[1]/8))&(25-var_p[3]/4)&tt[1]>>(6-var_p[2]/20));

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
else if(bytebeat_song==4)
{
	tt[0] = bytebeat_song_ptr/bit1;//4

	tt[1] = tt[0]/bit2;//2

	#define t tt[0]
	s[0] = ((-t&4095)*(255&t*(t&t>>(13-var_p[0]/10)))>>(12-var_p[1]/10))+(127&t*(234&t>>8&t>>(3+var_p[2]/10))>>(3&t>>(14-var_p[3]/10)));
	#define t tt[1]
	s[1] = ((-t&4095)*(255&t*(t&t>>(13-var_p[0]/10)))>>(12-var_p[1]/10))+(127&t*(234&t>>8&t>>(3+var_p[2]/10))>>(3&t>>(14-var_p[3]/10)));
	#undef t

	//s[0] = (t[0]*(t[0]>>8*(t[0]>>15|t[0]>>8)&(20|(t[0]>>19)*5>>t[0]|t[0]>>3));
	//s[1] = (t[1]*(t[1]>>8*(t[1]>>15|t[1]>>8)&(20|(t[1]>>19)*5>>t[1]|t[1]>>3));

	//s[0] = ((((5&((3 *(23*(4^t[0])))+t[0]))*(9*((15>>((9&((t[0]&12)^15))>>5))*2)))*((((t[0]*(t[0]*8))>>10)&t[0])>>42))^15);
	//s[1] = ((((5&((3 *(23*(4^t[1])))+t[1]))*(9*((15>>((9&((t[1]&12)^15))>>5))*2)))*((((t[1]*(t[1]*8))>>10)&t[1])>>42))^15);

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
else if(bytebeat_song==5)
{
	tt[0] = bytebeat_song_ptr/bit1;//2;//3;
	tt[1] = tt[0]/bit2;//2;

	/*
	//long gaps of silence but very interesting bits!
	#define t tt[0]
	s[0] = t*(t>>((t>>9|t>>8))&63&t>>4);
	#define t tt[1]
	s[1] = t*(t>>((t>>9|t>>8))&63&t>>4);
	#undef t
	*/

	/*
	//(t^t>>8)|t<<3&56^t
	//that one seems to produce something interesting :D

	//low tone motorboating with quieter high buzzing
	#define t tt[0]
	s[0] = (t^t>>8)|t<<3&56^t;
	#define t tt[1]
	s[1] = (t^t>>8)|t<<3&56^t;
	#undef t
	*/

	//combined:
	#define t tt[0]
	s[0] = (t*(t>>(((t>>(9-var_p[1]/10))|(t>>8)))&(63-var_p[2]/2)&(t>>(4+var_p[3]/10)))) + ((t^(t>>8))|(((t<<3)&(56-var_p[0]/2))^t));
	#define t tt[1]
	s[1] = (t*(t>>(((t>>(9-var_p[1]/10))|(t>>8)))&(63-var_p[2]/2)&(t>>(4+var_p[3]/10)))) + ((t^(t>>8))|(((t<<3)&(56-var_p[0]/2))^t));
	#undef t

	/*
	//not fast enough within this sampling rate timing

	//long gaps of silence but very interesting bits - now compressed
	s[0] = 0;
	s[1] = 0;
	while(s[0]==0 && s[1]==0)
	{
		tt[0] = bytebeat_song_ptr/3;
		tt[1] = tt[0]/2;
		#define t tt[0]
		s[0] = t*(t>>((t>>9|t>>8))&63&t>>4);
		#define t tt[1]
		s[1] = t*(t>>((t>>9|t>>8))&63&t>>4);
		#undef t
		bytebeat_song_ptr++;
	}
	bytebeat_song_ptr--;
	*/

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}

/*
else if(bytebeat_song==6)
{
	tt[0] = bytebeat_song_ptr;///3;
	tt[1] = tt[0]/2;

	//put (t>>1)+(sin(t))*(t>>4)  at 44,100 sample rate
	//it sounds like voices and static

	#define t tt[0]
	s[0] = (t>>1)+(sin(t))*(t>>4);
	#define t tt[1]
	s[1] = (t>>1)+(sin(t))*(t>>4);
	#undef t

	mix1 = (float)s[0];
	mix2 = (float)s[1];
}
else if(bytebeat_song==7)
{
	tt[0] = bytebeat_song_ptr;///3;
	tt[1] = tt[0]/2;

	//try this at 44,100 rate (t>>1)+(cos(t<<1))*(t>>4)

	#define t tt[0]
	s[0] = (t>>1)+(cos(t<<1))*(t>>4);
	#define t tt[1]
	s[1] = (t>>1)+(cos(t<<1))*(t>>4);
	#undef t

	mix1 = (float)s[0];
	mix2 = (float)s[1];
}
*/

else if(bytebeat_song==6)
{
	tt[0] = bytebeat_song_ptr/bit1;//4;
	tt[1] = tt[0]/bit2;//2;

	/*
	//nothing?
	#define t tt[0]
	s[0] = (t>>((t>>(3)|t>>(5)))&(63)&t>>7)*t;
	#define t tt[1]
	s[1] = (t>>((t>>(3)|t>>(5)))&(63)&t>>7)*t;
	#undef t
	*/

	//cool bagpipe-like sound with rhythm
	#define t tt[0]
	s[0] = (var_p[0]+1)*t*t*~((t>>(16)|t>>(12))/(var_p[2]+1)&215&~t>>(8+var_p[1]/2))/(var_p[3]+1);
	//s[0] = t*t*~((t>>(16)|t>>(12))&215&~t>>8);
	#define t tt[1]
	s[1] = (var_p[0]+1)*t*t*~((t>>(16)|t>>(12))/(var_p[2]+1)&215&~t>>(8-var_p[1]/2))/(var_p[3]+1);
	//s[1] = t*t*~((t>>(16)|t>>(12))&215&~t>>8);
	#undef t

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
else if(bytebeat_song==7)
{
	tt[0] = bytebeat_song_ptr/bit1;//4;///3;
	tt[1] = tt[0]/bit2;//2;

	//Here's one that sounds exactly like music.
	//Hasn't repeated once at all during 66 seconds.
	//t * ((t>>14|t>>9)&92&t>>5)
	//even has an ending! :D SWEET!

	#define t tt[0]
	s[0] = t*((t>>(14-var_p[1]/7)|t>>(9-var_p[0]/11))&(92-var_p[3])&t>>(5+var_p[2]/10));
	//s[0] = t*((t>>14|t>>9)&92&t>>5);
	#define t tt[1]
	s[1] = t*((t>>(14-var_p[1]/7)|t>>(9-var_p[0]/11))&(92-var_p[3])&t>>(5+var_p[2]/10));
	//s[1] = t*((t>>14|t>>9)&92&t>>5);
	#undef t

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
else if(bytebeat_song==8)
{
	tt[0] = bytebeat_song_ptr/4;

	tt[1] = tt[0]/2;

	//t*2&t/1 + t>>t&t*t>>6&1?t>>5:-t>>4&t*(t>>9|t>>13)&t>>-1111

	//t*2&t/t>>t&t&1?1:-t>>4&t*(t>>9|t>>13)&t>>-1111
	//t*2&t/t>>t&1?1:-t>>4&t*(t>>9|t>>13)&t>>-1111

	//#define f ((t>>3)&t*(t>>7|t>>13)&t>>7) + ((t>>2)&t*(t>>7|t>>13)&t>>7) + ((t>>4)&t*(t>>7|t>>13)&t>>7)+ ((t>>5)&t*(t>>7|t>>13)&t>>7)+ ((t>>6)&t*(t>>7|t>>13)&t>>7)
	#define f (t*(t>>7|t>>13)&t>>7)

	#define t tt[0]
	//s[0] = t * (2 & (t/(1 + (t>>t&t)))) * ((t>>6&1)?(t>>5):(-t>>4)) & ((t*t)>>9)|(t>>13)&(t>>-1111));
	//s[0] = t*2&t/(t>>t)&1?1:-t>>4&t*(t>>9|t>>13)&t>>-1111;
	//t*2&t/t>>t&t*t>>6&1?t>>5:-t>>4&t*(t>>9|t>>13)&t>>-1111;

	s[0] = f;

	#define t tt[1]
	//s[1] = t * (2 & (t/(1 + (t>>t&t)))) * ((t>>6&1)?(t>>5):(-t>>4)) & ((t*t)>>9)|(t>>13)&(t>>-1111));
	//s[1] = t*2&t/(t>>t)&1?1:-t>>4&t*(t>>9|t>>13)&t>>-1111;
	//t*2&t/t>>t&t*t>>6&1?t>>5:-t>>4&t*(t>>9|t>>13)&t>>-1111;

	s[1] = f;
	#undef t
	#undef f

	mix1 += (float)s[0];
	mix2 += (float)s[1];
}
/*
else if(bytebeat_song==9)
{
	t[0] = tt/6;
	//t[0] = tt/12;
	//t[0] = tt/24;

	//t[1] = t[0];
	t[1] = t[0]/2;
	//t[1] = t[0]/4;

	//t[2] = t[1]/4;
	//t[3] = t[2]/4;

	//s[0] = t[0]*((t[0]>>12|t[0]>>8)&63&t[0]>>4);
	//s[1] = t[1]*((t[1]>>12|t[1]>>8)&63&t[1]>>4);
	//s[2] = t[2]*((t[2]>>12|t[2]>>8)&63&t[2]>>4);
	//s[3] = t[3]*((t[3]>>12|t[3]>>8)&63&t[3]>>4);

	//s[0] = (t[0]*(t[0]>>5|t[0]>>8))>>(t[0]>>16);
	//s[1] = (t[1]*(t[1]>>5|t[1]>>8))>>(t[1]>>16);
	//s[2] = (t[2]*(t[2]>>5|t[2]>>8))>>(t[2]>>16);
	//s[3] = (t[3]*(t[3]>>5|t[3]>>8))>>(t[3]>>16);

	s[0] = t[0]*(t[0]>>11&t[0]>>8&123&t[0]>>3);
	s[1] = t[1]*(t[1]>>11&t[1]>>8&123&t[1]>>3);

	//mix1 = (float)bytebeat_echo(s[0]+s[2]);
	//mix2 = (float)bytebeat_echo(s[1]+s[3]);
	//mix1 = (float)(s[0]+s[2]);
	//mix2 = (float)(s[1]+s[3]);
	mix1 = (float)s[0];
	mix2 = (float)s[1];

	//9
	//t>>t&t*t>>2&1?t>>5:-t>>4&t*(t>>7|t>>13)&t>>-1110
	//t>>t&t*t>>2&1?t>>16:t>>4&t*(t>>9|t>>11)&t>>-1110
	//t>>t&t*t>>4&4?t>>12:t>>4&t*(t>>9|t>>11)&t>>-1110
	//t>>6*t>>5*t>>2*t>>3*t>>4&4?t>>17:t>>4&t*(t>>7|t>>13)&t>>-1110
		//= (t>>6*t>>5*t>>2*t>>3*t>>4)&4?(t>>17):((t>>4)&t*(t>>7|t>>13)&t>>-1110)
		//= ((t>>4)&t*(t>>7|t>>13)&t>>-1110)
	//((t>>2)&t*(t>>7|t>>13)&t>>-1110)
	//((t>>2)&t*(t>>7|t>>13)&t>>3)
	//((t>>2)&t*(t>>7|t>>13)&t>>7) !!!
}
*/

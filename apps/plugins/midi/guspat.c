/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/


extern struct plugin_api * rb;

unsigned int readWord(int file)
{
	return (readChar(file)<<0) | (readChar(file)<<8); // | (readChar(file)<<8) | (readChar(file)<<0);
}

unsigned int readDWord(int file)
{
	return (readChar(file)<<0) | (readChar(file)<<8) | (readChar(file)<<16) | (readChar(file)<<24);
}

struct GWaveform * loadWaveform(int file)
{
	struct GWaveform * wav = (struct GWaveform *)allocate(sizeof(struct GWaveform));
	rb->memset(wav, 0, sizeof(struct GWaveform));

	wav->name=readData(file, 7);
	printf("\nWAVE NAME = [%s]", wav->name);
	wav->fractions=readChar(file);
	wav->wavSize=readDWord(file);
	wav->startLoop=readDWord(file);
	wav->endLoop=readDWord(file);
	wav->sampRate=readWord(file);

	wav->lowFreq=readDWord(file);
	wav->highFreq=readDWord(file);
	wav->rootFreq=readDWord(file);

	wav->tune=readWord(file);

	wav->balance=readChar(file);
	wav->envRate=readData(file, 6);
	wav->envOffset=readData(file, 6);

	wav->tremSweep=readChar(file);
	wav->tremRate==readChar(file);
	wav->tremDepth=readChar(file);
	wav->vibSweep=readChar(file);
	wav->vibRate=readChar(file);
	wav->vibDepth=readChar(file);
	wav->mode=readChar(file);

	wav->scaleFreq=readWord(file);
	wav->scaleFactor=readWord(file);
	printf("\nScaleFreq = %d   ScaleFactor = %d   RootFreq = %d", wav->scaleFreq, wav->scaleFactor, wav->rootFreq);
	wav->res=readData(file, 36);
	wav->data=readData(file, wav->wavSize);

	int a=0;

	//If we have a 16 bit waveform
	if(wav->mode & 1  && (wav->mode & 2))
	{
		for(a=0; a<wav->wavSize; a+=2)	//Convert it to
		{
			//wav->data[a]=wav->data[a]; //+((wav->mode & 2) << 6);
			wav->data[a|1]=wav->data[(a)|1]+(1 << 7);
		}
	}

	return wav;
}



int selectWaveform(struct GPatch * pat, int midiNote)
{
	int tabFreq = gustable[midiNote]/100; //Comparison
	int a=0;
	for(a=0; a<pat->numWaveforms; a++)
	{
		if(pat->waveforms[a]->lowFreq/100 <= tabFreq &&
		   pat->waveforms[a]->highFreq/100 >= tabFreq)
		{
			return a;
		}
	}
	return 0;
}


struct GPatch * gusload(char * filename)
{
	struct GPatch * gp = (struct GPatch *)allocate(sizeof(struct GPatch));
	rb->memset(gp, 0, sizeof(struct GPatch));

	int file = rb->open(filename, O_RDONLY);

	if(file == -1)
	{
		char message[50];
		rb->snprintf(message, 50, "Error opening %s", filename);
		rb->splash(HZ*2, true, message);
		return NULL;
	}

	gp->header=readData(file, 12);
	gp->gravisid=readData(file, 10);
	gp->desc=readData(file, 60);
	gp->inst=readChar(file);
	gp->voc=readChar(file);
	gp->chan=readChar(file);
	gp->numWaveforms=readWord(file); //readWord(file);
	gp->vol=readWord(file);
	gp->datSize=readDWord(file);
	gp->res=readData(file, 36);

	gp->instrID=readWord(file);
	gp->instrName=readData(file,16);
	gp->instrSize=readDWord(file);
	gp->layers=readChar(file);
	gp->instrRes=readData(file,40);


	gp->layerDup=readChar(file);
	gp->layerID=readChar(file);
	gp->layerSize=readDWord(file);
	gp->numWaves=readChar(file);
	gp->layerRes=readData(file,40);

/*	printf("\n%s\n%s\n%s", gp->header, gp->gravisid, gp->desc);
	printf("\nInst = %d", gp->inst);
	printf("\nVoc  = %d", gp->voc);
	printf("\nChan = %d", gp->chan);
	printf("\nWav  = %d", gp->numWaveforms);
	printf("\nVol  = %d", gp->vol);
	printf("\nSize = %d", gp->datSize);

	printf("\n\ninstrID = %d", gp->instrID);
	printf("\ninstrName = %s", gp->instrName);
//	printf("\ninstrSize = %d", gp->instrSize);
//	printf("\nlayers = %d", gp->layers);
*/
	printf("\nFILE: %s", filename);
	printf("\nlayerSamples=%d", gp->numWaves);

	int a=0;
	for(a=0; a<gp->numWaves; a++)
		gp->waveforms[a] = loadWaveform(file);


	printf("\nPrecomputing note table");

	for(a=0; a<128; a++)
	{
		gp->noteTable[a] = selectWaveform(gp, a);
	}
	rb->close(file);

	return gp;
}

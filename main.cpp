#include <iostream>
using namespace std;

#include "olcNoiseMaker.h"

//converter (Hz to angular velocity 'Winkelgeschwindigkeit')
double w(double dHertz)
{
	return dHertz * 2.0 * PI;
}

//osciolator for switching waveforms
double osc(double dHertz, double dTime, int nType)
{
	switch (nType)
	{
		case 0:	//sine wave
			return sin(w(dHertz) * dTime);

		case 1:	//square wave
			return sin(w(dHertz) * dTime) > 0.0 ? 1.0 :-1.0;

		case 2:	//triangle wave
			return asin(sin(w(dHertz) * dTime)) * 2.0 / PI;

		case 3: //saw wave (unoptimised)
		{
			double dOutput = 0.0;

			for (double n = 1.0; n < 100.0; n++)
				dOutput += (sin(n * w(dHertz) * dTime)) / n;

			return dOutput * (2.0 / PI);
		}

		case 4: //saw wave (optimised)
			return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

		case 5:	//random noise
			return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

		default:
			return 0;
	}
}

//structure for Attack, Decay, Sustain and Release of an idividual note played
struct sEnvelopeADSR
{
	double dAttackTime;
	double dDecayTime;
	double dReleaseTime;

	double dSustainAmplitude;
	double dStartAmplitude;

	double dTriggerOnTime;
	double dTriggerOffTime;

	bool bNoteOn;


	sEnvelopeADSR()
	{
		dAttackTime = 0.001;
		dDecayTime = 0.001;
		dStartAmplitude = 1.0;
		dSustainAmplitude = 1.0;
		dReleaseTime = 0.8;
		dTriggerOnTime = 0.0;
		dTriggerOffTime = 0.0;
		bNoteOn = false;
	}

	double GetAmplitude(double dTime)
	{
		double dAmplitude = 0.0;
		double dLifeTime = dTime - dTriggerOnTime;

		if (bNoteOn)
		{
			//ADS
			//Attack
			if (dLifeTime <= dAttackTime)
				dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

			//Decay
			if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
				dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

			//Sustain
			if (dLifeTime > (dAttackTime + dDecayTime))
			{
				dAmplitude = dSustainAmplitude;
			}
		}
		else
		{
			//Release
			dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude;
		}

		//switching off unneccesary noise
		if (dAmplitude <= 0.0001)
		{
			dAmplitude = 0;
		}

		return dAmplitude;
	}

	void NoteOn(double dTimeOn)
	{
		dTriggerOnTime = dTimeOn;
		bNoteOn = true;
	}

	void NoteOff(double dTimeOff)
	{
		dTriggerOffTime = dTimeOff;
		bNoteOn = false;
	}
};

//global variables
atomic<double> dFrequencyOutput = 0.0;			//dominant frequency
//frequency modifier
double dOctaveBaseFrequency = 220.0;			//A3
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);		//12 notes per octave
sEnvelopeADSR envelope;



//"olcNoiseMaker" function
double Synth(double dTime) 
{
	double dOutput = envelope.GetAmplitude(dTime) * //osc(dFrequencyOutput, dTime, 3);
		(
			+ osc(dFrequencyOutput * 0.5, dTime, 3)
			+ osc(dFrequencyOutput * 1.0, dTime, 2)
		);

	return dOutput * 0.2; //Master
}



int main() 
{
	
	//include avalable soundcard(s)
	vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

	//show devices
	for (auto d : devices) wcout << "Found Device: " << d << endl;
	
	//creat synth
	olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	//link "Synth" function
	sound.SetUserFunction(Synth);

	int nCurrentKey = -1;
	bool bKeyPressed = false;
	while (1)
	{
		bool bKeyPressed = false;
		for (int k = 0; k < 15; k++)
		{
			if (GetAsyncKeyState((unsigned char)("AWSDRFTGHUJIKOL"[k])) & 0x8000)
			{
				if (nCurrentKey != k)
				{
					dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
					envelope.NoteOn(sound.GetTime());
					wcout << "\rNote On : " << sound.GetTime() << "s " << dFrequencyOutput;
					nCurrentKey = k;
				}
				bKeyPressed = true;
			}
		}
		if (!bKeyPressed)
		{
			if (nCurrentKey != -1)
			{
				wcout << "\rNote Off: " << sound.GetTime() << "s";
				envelope.NoteOff(sound.GetTime());
				nCurrentKey = -1;
			}
			//dFrequencyOutput = 0.0;
		}
	}

	return 0;
}
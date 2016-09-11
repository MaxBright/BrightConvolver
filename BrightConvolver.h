/**
	BrightConvolver.h
	Copyright (c) 2016 Max Bright. All Rights Reserved.
	
	BrightConvolver is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	BrightConvolver is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with BrightConvolver. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __BRIGHTCONVOLVER__
#define __BRIGHTCONVOLVER__

#include "IPlug_include_in_plug_hdr.h"

class BrightConvolver : public IPlug
{
public:
	BrightConvolver(IPlugInstanceInfo instanceInfo);
	~BrightConvolver();

	void Reset();
	void OnParamChange(int paramIdx);
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

	void onFrameLengthUpdate();
	void convolve(std::vector<double> &frame1, std::vector<double> &frame2, std::vector<double> &convResult);

private:
	// How many previous samples of the input we convolve.
	// Must be a power of 2 since the FFT is only defined for powers of 2
	int frameLength;
	//What we multiply the convolution result by.
	// Must be very low because the convolution produces a high volume signal
	double convGain;
	std::vector<double> previousOverlap[2];
	std::queue<double> inputQueue1[2];
	std::queue<double> inputQueue2[2];
	std::queue<double> outputQueue[2];
	//void CreatePresets();
};

#endif

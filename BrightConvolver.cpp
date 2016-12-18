/**
BrightConvolver.cpp

Takes two incoming signals and performs a convolution on the last user-defined
set of samples. Signal 1 occupies channels 1&2, signal 2 occupies channels 3&4

Copyright (c) 2016 Max Bright. All Rights Reserved.
This file is part of BrightConvolver

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

#include <math.h>
#include <fstream>
#include <queue>
#include <vector>
#include "BrightConvolver.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "OOURA_fft.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
    kgain = 0,
    kFrameLength,
    kNumParams
};

enum ELayout
{
    kWidth = GUI_WIDTH,
    kHeight = GUI_HEIGHT,

    kgainX = 30,
    kgainY = 25,
    kKnobFrames = 60,

    kFrameLengthX = 120,
    kFrameLengthY = 25
};

// Performs a convolution of the last frameLength samples between frame1 and frame2, and sends that to convResult
// Depends on private BrightConvolver members: convGain, frameLength
void BrightConvolver::convolve(std::vector<double> &frame1, std::vector<double> &frame2, std::vector<double> &convResult)
{
    size_t i;

    // FFT library depends on these
    int ip[NMAXSQRT + 2];
    double w[NMAX * 5 / 4];
    ip[0] = 0;

    // Zero pad signals, doubling their length to prevent circular convolution effects
    int fftLength = frameLength * 2;
    for (i = 0; i < frameLength; i++)
    {
        frame1.push_back(0);
        frame2.push_back(0);
    }

    // Take FFT of both signals
    rdft(fftLength, 1, &frame1.at(0), ip, w);
    rdft(fftLength, 1, &frame2.at(0), ip, w);

    // Multiply the two FFTs
    for (i = 0; i < fftLength; i++)
    {
        convResult.push_back(frame1.at(i) * frame2.at(i) * convGain);
    }

    // Take inverse FFT to give us our output
    rdft(fftLength, -1, &convResult.at(0), ip, w);

    // Apply window function to convResult to zero out discontinuities between output frames
    for (i = 0; i < fftLength; i++)
    {
        convResult.at(i) *= sqrt(sin(i*PI / fftLength));
    }
}

// Called to avoid array out of bounds errors when frameLength is changed
// Depends on private variables: previousOverlap
void BrightConvolver::onFrameLengthUpdate()
{
    size_t i;

    // Fill previousOverlap with zeros
    for (i = 0; i < CHANNEL_COUNT; i++)
    {
        previousOverlap[i].assign(frameLength, 0);
    }
}

BrightConvolver::BrightConvolver(IPlugInstanceInfo instanceInfo)
    : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), convGain(.00001), frameLength(1024)
{
    TRACE;

    onFrameLengthUpdate();

    // Arguments are: name, defaultVal, minVal, maxVal, step, label
    GetParam(kgain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
    GetParam(kgain)->SetShape(2.);

    GetParam(kFrameLength)->InitDouble("Frame Length", 10.0, 1., 13.0, 1, "(Power of 2)");
    GetParam(kFrameLength)->SetShape(1.);

    IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
    pGraphics->AttachBackground(BACKGROUND_ID, BACKGROUND_FN);

    IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
    IBitmap frameKnob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);

    pGraphics->AttachControl(new IKnobMultiControl(this, kgainX, kgainY, kgain, &knob));
    pGraphics->AttachControl(new IKnobMultiControl(this, kFrameLengthX, kFrameLengthY, kFrameLength, &frameKnob));

    AttachGraphics(pGraphics);

    //MakePreset("preset 1", ... );
    MakeDefaultPreset((char *) "-", kNumPrograms);
}

BrightConvolver::~BrightConvolver() {}

void BrightConvolver::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
    size_t i, j;

    // Iterate through the two stereo channels of each input
    for (i = 0; i < CHANNEL_COUNT; i++)
    {
        double* input1 = inputs[i];
        double* input2 = inputs[i + 2];
        double* output = outputs[i];

        // Add inputs to input queue
        for (j = 0; j < nFrames; j++)
        {
            inputQueue1[i].push(input1[j]);
            inputQueue2[i].push(input2[j]);
        }

        // Process and send pieces of inputQueues to outputQueue in chunks of frameLength
        while (inputQueue1[i].size() > frameLength)
        {
            std::vector<double> currentFrame1;
            std::vector<double> currentFrame2;
            std::vector<double> convResult;

            for (j = 0; j < frameLength; j++)
            {
                currentFrame1.push_back(inputQueue1[i].front());
                inputQueue1[i].pop();

                currentFrame2.push_back(inputQueue2[i].front());
                inputQueue2[i].pop();
            }

            BrightConvolver::convolve(currentFrame1, currentFrame2, convResult);

            for (j = 0; j < frameLength; j++)
            {
                // Since our convolution gives a signal that is twice the length of the original,
                // we have to overlap each output signal on top of each other, which is
                // what previousOverlap handles
                outputQueue[i].push(convResult.at(j) + previousOverlap[i].at(j));
                previousOverlap[i].at(j) = convResult.at(j + frameLength);
            }
        }

        // Now that we've filled the output queue with processed data, give it to the output
        int latency = nFrames - outputQueue[i].size();
        if (latency < 0)
        {
            latency = 0;
        }
        else if (latency > 0)
        {
            // Add zeros to beginning of signal to compensate for initial size mismatch between nFrames and frameLength
            for (j = 0; j < latency; j++)
            {
                output[j] = 0;
            }
        }
        IPlugVST::SetLatency(latency);
        // Write our outputQueue data to the output
        for (j = latency; j < nFrames; j++)
        {
            output[j] = outputQueue[i].front();
            outputQueue[i].pop();
        }
    }
}

void BrightConvolver::Reset()
{
    TRACE;
    IMutexLock lock(this);
}

void BrightConvolver::OnParamChange(int paramIdx)
{
    IMutexLock lock(this);

    switch (paramIdx)
    {
    case kFrameLength:
        // Set frameLength = 2^kFrameLength
        frameLength = 1 << ((int)GetParam(kFrameLength)->Value());
        onFrameLengthUpdate();
        break;
    case kgain:
        convGain = GetParam(kgain)->Value() * .000001;
        break;


    default:
        break;
    }
}
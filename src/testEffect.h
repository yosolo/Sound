#pragma once

#include <XAPOBase.h>
#include "xapo.h"
#include <assert.h>
#include <math.h>
#include <cstdio>

static XAPO_REGISTRATION_PROPERTIES reg = {
    .clsid = 0,                                          // COM class ID, used with CoCreate
    .FriendlyName = L"Circle effect",                    // friendly name unicode string
    .CopyrightInfo = L"Deine Mum",                          // copyright information unicode string
    .MajorVersion = 1,                                   // major version
    .MinorVersion = 0,                                   // minor version
    .Flags = 0,                                          // XAPO property flags, describes supported input/output configuration
    .MinInputBufferCount = 0,                            // minimum number of input buffers required for processing, can be 0
    .MaxInputBufferCount = 2,                            // maximum number of input buffers supported for processing, must be >= MinInputBufferCount
    .MinOutputBufferCount = 0,                           // minimum number of output buffers required for processing, can be 0, must match MinInputBufferCount when XAPO_FLAG_BUFFERCOUNT_MUST_MATCH used
    .MaxOutputBufferCount = 2                           // maximum number of output buffers supported for processing, must be >= MinOutputBufferCount, must match MaxInputBufferCount when XAPO_FLAG_BUFFERCOUNT_MUST_MATCH used
};

class EffectCircle : public CXAPOBase
{
    WORD m_uChannels, m_uBytesPerSample;
    
    bool isEnabled = false;
    float time = 0.0f;

    void process(void* pvSrc, int size)
    {
        for (int index = 0; index < size; ++index)
        {
            if (index % 2 == 0)
                ((float*)pvSrc)[index] = ((float*)pvSrc)[index] * (sinf(time) + 1.0f);
            else
                ((float*)pvSrc)[index] = ((float*)pvSrc)[index] * (cosf(time) + 1.0f);
        }
    }

public:
    EffectCircle()
        : CXAPOBase(&reg)
    {};

    STDMETHOD(LockForProcess) (UINT32 InputLockedParameterCount,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters,
        UINT32 OutputLockedParameterCount,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pOutputLockedParameters)
    {
        assert(!IsLocked());
        assert(InputLockedParameterCount == 1);
        assert(OutputLockedParameterCount == 1);
        assert(pInputLockedParameters != NULL);
        assert(pOutputLockedParameters != NULL);
        assert(pInputLockedParameters[0].pFormat != NULL);
        assert(pOutputLockedParameters[0].pFormat != NULL);


        m_uChannels = pInputLockedParameters[0].pFormat->nChannels;
        m_uBytesPerSample = (pInputLockedParameters[0].pFormat->wBitsPerSample >> 3);

        return CXAPOBase::LockForProcess(
            InputLockedParameterCount,
            pInputLockedParameters,
            OutputLockedParameterCount,
            pOutputLockedParameters);
    }

    // Process an audio buffer
    STDMETHOD_(void, Process)(UINT32 InputProcessParameterCount,
        const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters,
        UINT32 OutputProcessParameterCount,
        XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters,
        BOOL IsEnabled)
    {
        assert(IsLocked());
        assert(InputProcessParameterCount == 1);
        assert(OutputProcessParameterCount == 1);
        assert(NULL != pInputProcessParameters);
        assert(NULL != pOutputProcessParameters);


        XAPO_BUFFER_FLAGS inFlags = pInputProcessParameters[0].BufferFlags;
        XAPO_BUFFER_FLAGS outFlags = pOutputProcessParameters[0].BufferFlags;

        // assert buffer flags are legitimate
        assert(inFlags == XAPO_BUFFER_VALID || inFlags == XAPO_BUFFER_SILENT);
        assert(outFlags == XAPO_BUFFER_VALID || outFlags == XAPO_BUFFER_SILENT);

        // check input APO_BUFFER_FLAGS
        switch (inFlags)
        {
        case XAPO_BUFFER_VALID:
        {
            void* pvSrc = pInputProcessParameters[0].pBuffer;
            assert(pvSrc != NULL);

            if (IsEnabled)
            {
                isEnabled = true;
                process(pvSrc, pInputProcessParameters[0].ValidFrameCount * m_uChannels);
            }
            else
            {
                isEnabled = false;
            }


            // printf("%d\n", );

            void* pvDst = pOutputProcessParameters[0].pBuffer;
            assert(pvDst != NULL);

            memcpy(pvDst, pvSrc, pInputProcessParameters[0].ValidFrameCount * m_uChannels * m_uBytesPerSample);
            break;
        }

        case XAPO_BUFFER_SILENT:
        {
            // All that needs to be done for this case is setting the
            // output buffer flag to XAPO_BUFFER_SILENT which is done below.
            break;
        }

        }

        // set destination valid frame count, and buffer flags
        pOutputProcessParameters[0].ValidFrameCount = pInputProcessParameters[0].ValidFrameCount; // set destination frame count same as source
        pOutputProcessParameters[0].BufferFlags = pInputProcessParameters[0].BufferFlags;     // set destination buffer flags same as source

    }

    void update(float dt)
    {
        if (isEnabled)
            time += dt;
        printf("toggled, timer: %f\n", time);
    }
};

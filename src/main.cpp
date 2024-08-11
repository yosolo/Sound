#include <Windows.h>

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

#include <xaudio2.h>
#include <xaudio2fx.h>
#include <xapobase.h>
#include "xapo.h"
#include <assert.h>

#include "testEffect.h"

#include <math.h>
#include <cstdio>
#include <thread>

#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

float dt = 0.0f;
float volume = 0.3f;

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;

}

HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

//static XAPO_REGISTRATION_PROPERTIES reg = {
//    .clsid = 0,                                          // COM class ID, used with CoCreate
//    .FriendlyName = L"MyEffect",                    // friendly name unicode string
//    .CopyrightInfo = L"Meddl",                          // copyright information unicode string
//    .MajorVersion = 1,                                   // major version
//    .MinorVersion = 0,                                   // minor version
//    .Flags = 0,                                          // XAPO property flags, describes supported input/output configuration
//    .MinInputBufferCount = 0,                            // minimum number of input buffers required for processing, can be 0
//    .MaxInputBufferCount = 2,                            // maximum number of input buffers supported for processing, must be >= MinInputBufferCount
//    .MinOutputBufferCount = 0,                           // minimum number of output buffers required for processing, can be 0, must match MinInputBufferCount when XAPO_FLAG_BUFFERCOUNT_MUST_MATCH used
//    .MaxOutputBufferCount = 2                           // maximum number of output buffers supported for processing, must be >= MinOutputBufferCount, must match MaxInputBufferCount when XAPO_FLAG_BUFFERCOUNT_MUST_MATCH used
//};

//class MyEffect : public CXAPOBase
//{
//    WORD m_uChannels, m_uBytesPerSample;
//
//public:
//    MyEffect()
//        : CXAPOBase(&reg)
//    {};
//
//    STDMETHOD(LockForProcess) (UINT32 InputLockedParameterCount,
//        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters,
//        UINT32 OutputLockedParameterCount,
//        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pOutputLockedParameters)
//    {
//        assert(!IsLocked());
//        assert(InputLockedParameterCount == 1);
//        assert(OutputLockedParameterCount == 1);
//        assert(pInputLockedParameters != NULL);
//        assert(pOutputLockedParameters != NULL);
//        assert(pInputLockedParameters[0].pFormat != NULL);
//        assert(pOutputLockedParameters[0].pFormat != NULL);
//
//
//        m_uChannels = pInputLockedParameters[0].pFormat->nChannels;
//        m_uBytesPerSample = (pInputLockedParameters[0].pFormat->wBitsPerSample >> 3);
//
//        return CXAPOBase::LockForProcess(
//            InputLockedParameterCount,
//            pInputLockedParameters,
//            OutputLockedParameterCount,
//            pOutputLockedParameters);
//    }
//    STDMETHOD_(void, Process)(UINT32 InputProcessParameterCount,
//        const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters,
//        UINT32 OutputProcessParameterCount,
//        XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters,
//        BOOL IsEnabled)
//    {
//        assert(IsLocked());
//        assert(InputProcessParameterCount == 1);
//        assert(OutputProcessParameterCount == 1);
//        assert(NULL != pInputProcessParameters);
//        assert(NULL != pOutputProcessParameters);
//
//
//        XAPO_BUFFER_FLAGS inFlags = pInputProcessParameters[0].BufferFlags;
//        XAPO_BUFFER_FLAGS outFlags = pOutputProcessParameters[0].BufferFlags;
//
//        // assert buffer flags are legitimate
//        assert(inFlags == XAPO_BUFFER_VALID || inFlags == XAPO_BUFFER_SILENT);
//        assert(outFlags == XAPO_BUFFER_VALID || outFlags == XAPO_BUFFER_SILENT);
//
//        // check input APO_BUFFER_FLAGS
//        switch (inFlags)
//        {
//        case XAPO_BUFFER_VALID:
//        {
//            void* pvSrc = pInputProcessParameters[0].pBuffer;
//            assert(pvSrc != NULL);
//
//            if (IsEnabled)
//            {
//                for (int index = 0; index < pInputProcessParameters[0].ValidFrameCount * m_uChannels; ++index)
//                {
//                    if (index % 2 == 0)
//                        ((float*)pvSrc)[index] = ((float*)pvSrc)[index] * (sinf(dt) + 1.0f);
//                    else
//                        ((float*)pvSrc)[index] = ((float*)pvSrc)[index] * (cosf(dt) + 1.0f);
//                }
//            }
//            
//            // printf("%d\n", );
//
//            void* pvDst = pOutputProcessParameters[0].pBuffer;
//            assert(pvDst != NULL);
//
//            memcpy(pvDst, pvSrc, pInputProcessParameters[0].ValidFrameCount * m_uChannels * m_uBytesPerSample);
//            break;
//        }
//
//        case XAPO_BUFFER_SILENT:
//        {
//            // All that needs to be done for this case is setting the
//            // output buffer flag to XAPO_BUFFER_SILENT which is done below.
//            break;
//        }
//
//        }
//
//        // set destination valid frame count, and buffer flags
//        pOutputProcessParameters[0].ValidFrameCount = pInputProcessParameters[0].ValidFrameCount; // set destination frame count same as source
//        pOutputProcessParameters[0].BufferFlags = pInputProcessParameters[0].BufferFlags;     // set destination buffer flags same as source
//
//    }
//};

int main(int argc, char** argv)
{
    HRESULT hr;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return hr;

    IXAudio2* pXAudio2 = nullptr;
    if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        return hr;

    IXAudio2MasteringVoice* pMasterVoice = nullptr;
    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice)))
        return hr;

    WAVEFORMATEXTENSIBLE wfx = { 0 };
    XAUDIO2_BUFFER buffer = { 0 };

    const char* strFileName = "sound.wav";

    // Open the file
    HANDLE hFile = CreateFile(
        strFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
        return HRESULT_FROM_WIN32(GetLastError());

    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkSize;
    DWORD dwChunkPosition;
    //check the file type, should be fourccWAVE or 'XWMA'
    FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD filetype;
    ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
    if (filetype != fourccWAVE)
        return S_FALSE;

    FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

    //fill out the audio data buffer with the contents of the fourccDATA chunk
    FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE* pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

    buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
    buffer.pAudioData = pDataBuffer;  //buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

    // IUnknown* pXAPO;
    // CreateFX(__uuidof(FXReverb), &pXAPO);
    // if (FAILED(hr = XAudio2CreateReverb(&pXAPO, 0)))
    //     return hr;

    EffectCircle* pEffectCircle = new EffectCircle();

    XAUDIO2_EFFECT_DESCRIPTOR descriptor;
    descriptor.InitialState = true;
    descriptor.OutputChannels = 2;
    descriptor.pEffect = pEffectCircle;

    XAUDIO2_EFFECT_CHAIN chain;
    chain.EffectCount = 1;
    chain.pEffectDescriptors = &descriptor;

    //XAUDIO2FX_REVERB_PARAMETERS reverbParameters;
    //reverbParameters.ReflectionsDelay = XAUDIO2FX_REVERB_DEFAULT_REFLECTIONS_DELAY * 0;
    //// reverbParameters.ReverbDelay = XAUDIO2FX_REVERB_DEFAULT_REVERB_DELAY;
    //// reverbParameters.RearDelay = XAUDIO2FX_REVERB_DEFAULT_REAR_DELAY;
    //// reverbParameters.PositionLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION;
    //// reverbParameters.PositionRight = XAUDIO2FX_REVERB_DEFAULT_POSITION;
    //// reverbParameters.PositionMatrixLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
    //// reverbParameters.PositionMatrixRight = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
    //// reverbParameters.EarlyDiffusion = XAUDIO2FX_REVERB_DEFAULT_EARLY_DIFFUSION;
    //// reverbParameters.LateDiffusion = XAUDIO2FX_REVERB_DEFAULT_LATE_DIFFUSION;
    //// reverbParameters.LowEQGain = XAUDIO2FX_REVERB_DEFAULT_LOW_EQ_GAIN;
    //// reverbParameters.LowEQCutoff = XAUDIO2FX_REVERB_DEFAULT_LOW_EQ_CUTOFF;
    //// reverbParameters.HighEQGain = XAUDIO2FX_REVERB_DEFAULT_HIGH_EQ_GAIN;
    //// reverbParameters.HighEQCutoff = XAUDIO2FX_REVERB_DEFAULT_HIGH_EQ_CUTOFF;
    //reverbParameters.RoomFilterFreq = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_FREQ;
    //reverbParameters.RoomFilterMain = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_MAIN;
    //reverbParameters.RoomFilterHF = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_HF;
    //reverbParameters.ReflectionsGain = XAUDIO2FX_REVERB_DEFAULT_REFLECTIONS_GAIN;
    ////reverbParameters.ReverbGain = XAUDIO2FX_REVERB_DEFAULT_REVERB_GAIN;
    ////reverbParameters.DecayTime = XAUDIO2FX_REVERB_DEFAULT_DECAY_TIME;
    ////reverbParameters.Density = XAUDIO2FX_REVERB_DEFAULT_DENSITY;
    //reverbParameters.RoomSize = XAUDIO2FX_REVERB_DEFAULT_ROOM_SIZE;
    //reverbParameters.WetDryMix = 100;

     
    //if (FAILED(hr = pSourceVoice->SetEffectChain(&chain)))
    //    return hr;
    //
    //pXAPO->Release();

    
    IXAudio2SourceVoice* pSourceVoice;
    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx, 0, 2, 0, 0, &chain))) 
        return hr;

    //if (FAILED(hr = pSourceVoice->SetEffectParameters(0, &reverbParameters, sizeof(reverbParameters))))
    //    return hr;

    pSourceVoice->DisableEffect(0);

    if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer)))
        return hr;

    pSourceVoice->SetVolume(volume);
    
    if (FAILED(hr = pSourceVoice->Start(0)))
        return hr;

    float timer = 0.0f;
    while (1)
    {
        if (GetKeyState(VK_SPACE) < 0 && timer <= 0.0f) 
        {
            static bool enabled = false;
            if (enabled)
                pSourceVoice->DisableEffect(0);
            else
                pSourceVoice->EnableEffect(0);
            enabled = !enabled;
        
            printf("toggled, timer: %f\n", timer);
            
            timer = 10.0f;
        }
        
        if (timer > 0.0f)
            timer -= 1.0f;

        dt += 0.02f;

        pEffectCircle->update(0.02f);

        //pSourceVoice->SetVolume(3 * volume * (cosf(dt/3)+1));

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //printf("Time: %f\n", dt);
    }

    return 0;
}

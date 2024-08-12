#include "daisysp.h"
#include "daisy_patch.h"
#include <string>
#include <stdio.h>


using namespace daisy;
using namespace daisysp;

DaisyPatch             patch;
SdmmcHandler           sdcard;
FatFSInterface         fsi;
WavPlayer              sampler;
SdmmcHandler::Result   resSD;
FatFSInterface::Result resFat;
FRESULT                res;
std::string            str  = "NeoSD";
char*                  cstr = &str[0];

typedef enum
{
    STATE_OK = 0,
    STATE_CONFIG_SD_READER,
    STATE_FAT_INIT_ERROR,
    STATE_MOUNT_ERROR,
    STATE_RUNNING
} eErrorState;


void UpdateControls();

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();
    for(size_t i = 0; i < size; i += 2)
    {
        float sample = s162f(sampler.Stream()) * 0.5f;
        out[0][i]    = sample;
    }
}


void UpdateOled();

void DrawMessage(std::string message, int posx = 0, int posy = 0)
{
    if(posx == 0 && posy == 0)
    {
        patch.display.Fill(false);
    }
    patch.display.SetCursor(posx, posy);
    patch.display.WriteString(message.data(), Font_7x10, true);
    patch.display.Update();
}


eErrorState init()
{
    eErrorState          state = STATE_OK;
    SdmmcHandler::Config sd_cfg;

    DrawMessage("Init NeoSD");
    sd_cfg.Defaults();
    //sd_cfg.speed = SdmmcHandler::Speed::VERY_FAST;

    System::Delay(500);


    DrawMessage("Starting SD");
    resSD = sdcard.Init(sd_cfg);
    System::Delay(500);


    if(resSD != SdmmcHandler::Result::OK)
    {
        DrawMessage("Error SD");
        state = STATE_CONFIG_SD_READER;
        DrawMessage(
            "code " + std::to_string(static_cast<uint32_t>(resSD)), 0, 10);

        return state;
    }

    DrawMessage("Starting FATFS");
    resFat = fsi.Init(FatFSInterface::Config::MEDIA_SD);

    System::Delay(500);
    if(resFat != FatFSInterface::Result::OK)
    {
        DrawMessage("Error Fat");
        state = STATE_FAT_INIT_ERROR;
        DrawMessage(
            "code " + std::to_string(static_cast<uint32_t>(resFat)), 0, 10);

        return state;
    }

    DrawMessage("Mounting SD");

    DrawMessage("Getting FS");

    FATFS& fs = fsi.GetSDFileSystem();
    DrawMessage(
        "System is " + std::to_string(static_cast<uint8_t>(fs.fs_type)), 0, 10);
    DrawMessage(" and fat type is "
                    + std::to_string(static_cast<u_int8_t>(fs.n_fats)),
                0,
                20);
    System::Delay(1500);

    DrawMessage("Mounting FS");

    res = f_mount(&fs, "/", 1);
    System::Delay(500);

    if(res != FR_OK)
    {
        DrawMessage("Error Mount");
        DrawMessage(
            "code " + std::to_string(static_cast<uint32_t>(res)), 0, 10);

        state = STATE_MOUNT_ERROR;
        return state;
    }

    DrawMessage("System charged");

    return state;
}

int main(void)
{
    //size_t blocksize = 48;

    patch.Init(); // Initialize hardware (daisy seed, and patch)

    eErrorState state = init();
    if(state != STATE_OK)
    {
        DrawMessage("Error during init");

        exit(1);
    }
    else
    {
        DrawMessage("Running Sampler");
        System::Delay(500);

        sampler.Init(fsi.GetSDPath());
        sampler.SetLooping(true);

        // Init Audio
        // patch.SetAudioBlockSize(blocksize);
        patch.StartAudio(AudioCallback);
        while(1)
        {
            UpdateOled();
            sampler.Prepare();
        }
    }
}

void UpdateOled()
{
    uint32_t minX       = 0;
    uint32_t minY       = 0;
    uint32_t lineOffset = 8;
    uint16_t lineNumber = 1;

    patch.display.Fill(false);


    str = "Err SD:" + std::to_string(static_cast<uint32_t>(resSD));
    patch.display.SetCursor(minX, lineNumber * lineOffset + minY);
    patch.display.WriteString(cstr, Font_7x10, true);

    lineNumber++;
    str = "Err Fat:" + std::to_string(static_cast<uint32_t>(resFat));
    patch.display.SetCursor(minX, lineNumber * lineOffset + minY);
    patch.display.WriteString(cstr, Font_7x10, true);

    lineNumber++;
    str = "Err Mnt:" + std::to_string(static_cast<uint32_t>(res));
    patch.display.SetCursor(minX, lineNumber * lineOffset + minY);
    patch.display.WriteString(cstr, Font_7x10, true);


    patch.display.Update();
}

void UpdateControls()
{
    bool inc;
    patch.ProcessDigitalControls();


    //encoder
    inc = patch.encoder.Pressed();
    if(inc)
    {
        size_t curfile;
        curfile = sampler.GetCurrentFile();
        if(curfile < sampler.GetNumberFiles() - 1)
        {
            sampler.Open(curfile + 1);
        }
        else // loop back to the first file
        {
            sampler.Open(0);
        }
    }
}

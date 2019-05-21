#include <iostream>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <iomanip>

#include "AudioFile.h"
#include "AudioFile.cpp"

#define VER_ERROR	0
#define VER_3020000	0x03020000
#define VER_0020100 0x00020100

typedef struct
{
	int id;
	char smpName[32];
	int smpLen;
	int smpRate;
	int smpCh;
	unsigned char* data;
} BeatboxSample;

int tab0[256]; // 2^x*0.11811

void displayHelp();
FILE* openPresetFile(char* name);
int checkHeader(FILE* file);
void readSamples(FILE* file, std::vector<std::unique_ptr<BeatboxSample>>& samples, int version);
void writeSample(BeatboxSample* sample, const char* dir, int version);
void writeAllSampleToFolder(std::vector<std::unique_ptr<BeatboxSample>>& samples, const char* dir, int version);
void deallocSample(std::vector<std::unique_ptr<BeatboxSample>>& samples);
void makeTable();
int decodeSample(short* dst, unsigned char* src, size_t srcLen, size_t count);

int main(int argc, char* argv[])
{
	FILE* presetFile = nullptr;
	int presetVersion;
	std::vector<std::unique_ptr<BeatboxSample>> samples;
	int numParams = argc - 1;
	char* outputFolder;

	if (argc > 1)
	{
		outputFolder = argv[numParams];

		if (argc > 2)
		{
			presetFile = openPresetFile(argv[1]);
			if (!presetFile)
				return -1;

			presetVersion = checkHeader(presetFile);
			if (presetVersion == VER_ERROR)
				return -1;

			if (presetVersion != VER_3020000 && presetVersion != VER_0020100)
			{
				std::cout << "Error, unsupported version";
				return -1;
			}

			makeTable();
			readSamples(presetFile, samples, presetVersion);

			if (argc > 3)
			{
				if (argv[2][0] == '-')
				{
					bool found = false;

					switch (argv[2][1])
					{
					case 'n':
						for (auto& s : samples)
						{
							if (std::string(argv[3]) == s->smpName)
							{
								writeSample(s.get(), outputFolder, presetVersion);
								found = true;
								break;
							}
						}

						if (!found)
							std::cout << argv[3] << " sample not found" << std::endl;
					case 'i':
						for (auto& s : samples)
						{
							if (std::stoi(argv[3]) == s->id)
							{
								writeSample(s.get(), outputFolder, presetVersion);
								found = true;
								break;
							}
						}

						if (!found)
							std::cout << "This order is empty" << std::endl;
					default:
						std::cout << "Error, unknown option" << std::endl;
					}
				}
			}
			else
				writeAllSampleToFolder(samples, outputFolder, presetVersion);

			deallocSample(samples);
		}
		else
			displayHelp();
	}
	else
		displayHelp();

	if (presetFile)
		fclose(presetFile);

	return 0;
}

void displayHelp()
{
	std::cout << "Caustic BeatBox sample extractor v1.0" << std::endl;
	std::cout << "Usage: BeatboxExt [beatbox preset file] [options] [file/folder]" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "\t-n <sample name>\tExtract single sample by name. (If the name contain spaces," << std::endl;
	std::cout << "\t\t\t\tyou must use two double quote like this \"My sample\")" << std::endl;
	std::cout << "\t-i <sample order>\tExtract single sample by order." << std::endl;
	std::cout << "Example:" << std::endl;
	std::cout << "\tExtract all samples\tBeatboxExt 606.beatbox /path/to/output/folder/ (output folder must be a valid folder)" << std::endl;
	std::cout << "\tExtract sample by name\tBeatboxExt 606.beatbox -n \"Kick\" /path/to/file.wav" << std::endl;
	std::cout << "\tExtract sample by order\tBeatboxExt 606.beatbox -i 1 /path/to/file.wav" << std::endl;
}

FILE* openPresetFile(char* name)
{
	FILE* f;
	fopen_s(&f, name, "rb");

	if (!f)
		std::cout << "Failed to open preset file!" << std::endl;

	return f;
}

int checkHeader(FILE* file)
{
	/*
		im not sure why.
		but there are two file version in the header.
	*/
	char sig[5] = { 0 };
	int ver;

	fseek(file, 4, SEEK_CUR); // caustic version?
	fread(sig, 4, 1, file); // read signature

	if (strncmp(sig, "BBOX", 4) != 0)
	{
		std::cout << "Error, invalid signature" << std::endl;
		return VER_ERROR;
	}

	fseek(file, 4, SEEK_CUR); // skip this...
	fread((void*)&ver, sizeof(int), 1, file); // preset version?

	std::cout << "Detected preset version " << std::hex << ver << std::dec << std::endl;

	return ver;
}

void readSamples(FILE* file, std::vector<std::unique_ptr<BeatboxSample>>& samples, int version)
{
	fseek(file, 0x3D0, SEEK_SET); // skip over 0x3D0

	for (int i = 0; i < 8; i++)
	{
		uint8_t* tmp;
		BeatboxSample* smp = new BeatboxSample;

		smp->id = i + 1;
		fread(smp->smpName, 1, 32, file);
		fread((void*)&smp->smpLen, 4, 1, file);
		fread((void*)&smp->smpRate, 4, 1, file);

		if (version == VER_3020000)
			fread((void*)&smp->smpCh, sizeof(int), 1, file);

		if (smp->smpLen == 0)
			continue;

		switch (version)
		{
		case VER_3020000:
			smp->data = (unsigned char*)malloc(sizeof(short) * smp->smpLen * smp->smpCh);
			fread(smp->data, sizeof(short) * smp->smpLen * smp->smpCh, 1, file);
			break;
		case VER_0020100:
			// Decode compressed sample
			tmp = (uint8_t*)malloc(smp->smpLen);
			smp->data = (unsigned char*)malloc(sizeof(short) * smp->smpLen);
			fread(tmp, smp->smpLen, 1, file);
			smp->smpCh = 1;
			decodeSample((short*)smp->data, tmp, smp->smpLen, smp->smpCh);
			delete tmp;
			break;
		default:
			break;
		}

		samples.push_back(std::unique_ptr<BeatboxSample>(smp));
	}
}

void writeSample(BeatboxSample* sample, const char* dir, int version)
{
	AudioFile<float> file;

	file.setNumChannels(2);
	file.setSampleRate(sample->smpRate);
	file.setBitDepth(16);

	if (version == VER_0020100)
	{
		short* data = (short*)sample->data;
		int smpLen = sample->smpLen;

		while (smpLen)
		{
			float s = *data / 32768.f;
			file.samples[0].push_back(s);
			file.samples[1].push_back(s);
			data++;
			smpLen--;
		}
	}
	else if (version == VER_3020000)
	{
		short* data0 = (short*)sample->data;
		short* data1 = data0 + 1;
		int smpLen = sample->smpLen;

		while (smpLen)
		{
			if (sample->smpCh == 2) // stereo sample
			{
				file.samples[0].push_back((float)*data0 / 32768.f);
				file.samples[1].push_back((float)*data1 / 32768.f);
			}
			else
			{
				file.samples[0].push_back((float)*data0 / 32768.f);
				file.samples[1].push_back((float)*data0 / 32768.f);
			}

			data0 += sample->smpCh;
			data1 += 2;
			smpLen--;
		}
	}

	if (file.save(dir))
		std::cout << "File written: " << dir << " (16bit, " << sample->smpRate << "hz)" << std::endl;
	else
		std::cout << "Failed to write file: " << dir << std::endl;
}

void writeAllSampleToFolder(std::vector<std::unique_ptr<BeatboxSample>>& samples, const char* dir, int version)
{
	for (auto& s : samples)
	{
		std::string filename = dir + std::string("/") + std::string(s->smpName) + ".wav";
		writeSample(s.get(), filename.c_str(), version);
	}
}

void deallocSample(std::vector<std::unique_ptr<BeatboxSample>>& samples)
{
	for (auto& s : samples)
	{
		delete s->data;
		s->data = nullptr;
		s.reset();
	}
}

void makeTable()
{
	for (int i = 1; i < 128; i++)
	{
		float pw = powf(2.0f, (float)i * 0.11811f);
		tab0[i] = (int)pw;
		tab0[i + 127] = -((int)pw);
	}
}

int decodeSample(short* dst, unsigned char* src, size_t srcLen, size_t count)
{
	short* tmpDst0 = dst, *tmpDst1;
	unsigned char* tmpSrc0 = src, *tmpSrc1;
	int c0 = 0;
	int c1 = 0;
	int ret;

	if (srcLen && src)
	{
		tmpDst1 = tmpDst0 + 1;
		tmpSrc1 = tmpSrc0 + 1;

		while (srcLen)
		{
			c0 += tab0[*tmpSrc0];

			if (c0 >= -32767)
			{
				if (c0 > 32767)
					c0 = 32767;
			}
			else
				c0 = 32767;

			ret = count;
			*tmpDst0 = c0;

			if (count == 2)
			{
				c1 += tab0[*tmpSrc1];

				if (c1 >= -32767)
				{
					if (c1 > 32767)
						c1 = 32767;
				}
				else
					c1 = 32767;

				ret = 2;
				*tmpDst1 = c1;
			}

			tmpDst0 += count;
			tmpSrc0 += ret;
			tmpDst1 += 2;
			tmpSrc1 += 2;

			srcLen--;
		}
	}

	return ret;
}
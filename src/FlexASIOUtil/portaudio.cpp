#include "portaudio.h"

#include <pa_win_wasapi.h>

#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include <cctype>
#include <mutex>
#include <stdexcept>
#include <string_view>

#include "log.h"
#include "string.h"

// From pa_debugprint.h. The PortAudio DLL exports this function, but sadly it is not exposed in a public header file.
extern "C" {
	typedef void(*PaUtilLogCallback) (const char *log);
	extern void PaUtil_SetDebugPrintFunction(PaUtilLogCallback cb);
}

// From src/common/pa_hostapi.h, which is not exposed publicly but is nonetheless useful here.
//
/** The common header for all data structures whose pointers are passed through
 the hostApiSpecificStreamInfo field of the PaStreamParameters structure.
 Note that in order to keep the public PortAudio interface clean, this structure
 is not used explicitly when declaring hostApiSpecificStreamInfo data structures.
 However, some code in pa_front depends on the first 3 members being equivalent
 with this structure.
 @see PaStreamParameters
*/
typedef struct PaUtilHostApiSpecificStreamInfoHeader
{
	unsigned long size;             /**< size of whole structure including this header */
	PaHostApiTypeId hostApiType;    /**< host API for which this data is intended */
	unsigned long version;          /**< structure version */
} PaUtilHostApiSpecificStreamInfoHeader;

namespace flexasio {

	namespace {
		std::mutex loggerMutex;
		size_t loggerReferenceCount;

		void DebugPrint(const char* log) {
			std::string_view logline(log);
			while (!logline.empty() && isspace(logline.back())) logline.remove_suffix(1);
			Log() << "[PortAudio] " << logline;
		}
	}

	PortAudioLogger::PortAudioLogger() {
		Log() << "PortAudio version: " << Pa_GetVersionText();

		std::scoped_lock loggerLock(loggerMutex);
		if (loggerReferenceCount++ > 0) return;
		Log() << "Enabling PortAudio debug output redirection";
		PaUtil_SetDebugPrintFunction(DebugPrint);
	}

	PortAudioLogger::~PortAudioLogger() {
		std::scoped_lock loggerLock(loggerMutex);
		if (--loggerReferenceCount > 0) return;
		Log() << "Disabling PortAudio debug output redirection";
		PaUtil_SetDebugPrintFunction(NULL);
	}

	std::string GetHostApiTypeIdString(PaHostApiTypeId hostApiTypeId) {
		return EnumToString(hostApiTypeId, {
			{ paInDevelopment, "In development" },
			{ paDirectSound, "DirectSound" },
			{ paMME, "MME" },
			{ paASIO, "ASIO" },
			{ paSoundManager, "SoundManager" },
			{ paCoreAudio, "CoreAudio" },
			{ paOSS, "OSS" },
			{ paALSA, "ALSA" },
			{ paAL, "AL" },
			{ paBeOS, "BeOS" },
			{ paWDMKS, "WDMKS" },
			{ paJACK, "JACK" },
			{ paWASAPI, "WASAPI" },
			{ paAudioScienceHPI, "AudioScienceHPI" },
			});
	}

	std::string GetSampleFormatString(PaSampleFormat sampleFormat) {
		auto result = BitfieldToString(sampleFormat, {
			{ paFloat32, "Float32" },
			{ paInt32, "Int32" },
			{ paInt24, "Int24" },
			{ paInt16, "Int16" },
			{ paInt8, "Int8" },
			{ paUInt8, "UInt8" },
			{ paCustomFormat, "CustomFormat" },
			{ paNonInterleaved, "NonInterleaved" },
			});
		return result;
	}

	std::string GetStreamFlagsString(PaStreamFlags streamFlags) {
		return BitfieldToString(streamFlags, {
			{ paClipOff, "ClipOff" },
			{ paDitherOff, "DitherOff" },
			{ paNeverDropInput, "NeverDropInput" },
			{ paPrimeOutputBuffersUsingStreamCallback, "PrimeOutputBuffersUsingStreamCallback" },
			});
	}

	std::string GetWasapiFlagsString(PaWasapiFlags wasapiFlags) {
		return BitfieldToString(wasapiFlags, {
			{ paWinWasapiExclusive, "Exclusive" },
			{ paWinWasapiRedirectHostProcessor, "RedirectHostProcessor" },
			{ paWinWasapiUseChannelMask, "UseChannelMask" },
			{ paWinWasapiPolling, "Polling" },
			{ paWinWasapiThreadPriority, "ThreadPriority" },
			});
	}

	std::string GetWasapiThreadPriorityString(PaWasapiThreadPriority threadPriority) {
		return EnumToString(threadPriority, {
			{ eThreadPriorityNone, "None" },
			{ eThreadPriorityAudio, "Audio" },
			{ eThreadPriorityCapture, "Capture" },
			{ eThreadPriorityDistribution, "Distribution" },
			{ eThreadPriorityGames, "Games" },
			{ eThreadPriorityPlayback, "Playback" },
			{ eThreadPriorityProAudio, "ProAudio" },
			{ eThreadPriorityWindowManager, "WindowManager" },
			});
	}

	std::string GetWasapiStreamCategoryString(PaWasapiStreamCategory streamCategory) {
		return EnumToString(streamCategory, {
			{ eAudioCategoryOther, "Other" },
			{ eAudioCategoryCommunications, "Communications" },
			{ eAudioCategoryAlerts, "Alerts" },
			{ eAudioCategorySoundEffects, "SoundEffects" },
			{ eAudioCategoryGameEffects, "GameEffects" },
			{ eAudioCategoryGameMedia, "GameMedia" },
			{ eAudioCategoryGameChat, "GameChat" },
			{ eAudioCategorySpeech, "Speech" },
			{ eAudioCategoryMovie, "Movie" },
			{ eAudioCategoryMedia, "Media" },
			});
	}

	std::string GetWasapiStreamOptionString(PaWasapiStreamOption streamOption) {
		return EnumToString(streamOption, {
			{ eStreamOptionNone, "None" },
			{ eStreamOptionRaw, "Raw" },
			{ eStreamOptionMatchFormat, "MatchFormat" },
			});
	}

	std::string GetStreamCallbackFlagsString(PaStreamCallbackFlags streamCallbackFlags) {
		return BitfieldToString(streamCallbackFlags, {
			{ paInputUnderflow, "InputUnderflow" },
			{ paInputOverflow, "InputOverflow" },
			{ paOutputUnderflow, "OutputUnderflow" },
			{ paOutputOverflow, "OutputOverflow" },
			{ paPrimingOutput, "PrimingOutput" },
			});
	}

	std::ostream& operator<<(std::ostream& os, const HostApi& hostApi) {
		os << "PortAudio host API index " << hostApi.index
			<< " (name: '" << hostApi.info.name
			<< "', type: " << GetHostApiTypeIdString(hostApi.info.type)
			<< ", default input device: " << hostApi.info.defaultInputDevice
			<< ", default output device: " << hostApi.info.defaultOutputDevice << ")";
		return os;
	}

	const PaHostApiInfo& HostApi::GetInfo(PaHostApiIndex index) {
		const auto info = Pa_GetHostApiInfo(index);
		if (info == nullptr) throw std::runtime_error(std::string("Unable to get host API info for host API index ") + std::to_string(index));
		return *info;
	}

	std::ostream& operator<<(std::ostream& os, const Device& device) {
		os << "PortAudio device index " << device.index
			<< " (name: '" << device.info.name
			<< "', host API: " << device.info.hostApi
			<< ", default sample rate: " << device.info.defaultSampleRate
			<< ", max input channels: " << device.info.maxInputChannels
			<< ", max output channels: " << device.info.maxOutputChannels
			<< ", input latency: " << device.info.defaultLowInputLatency << " (low) " << device.info.defaultHighInputLatency << " (high)"
			<< ", output latency: " << device.info.defaultLowOutputLatency << " (low) " << device.info.defaultHighOutputLatency << " (high)" << ")";
		return os;
	}

	const PaDeviceInfo& Device::GetInfo(PaDeviceIndex index) {
		const auto info = Pa_GetDeviceInfo(index);
		if (info == nullptr) throw std::runtime_error(std::string("Unable to get device info for device index ") + std::to_string(index));
		return *info;
	}

	WAVEFORMATEXTENSIBLE GetWasapiDeviceDefaultFormat(PaDeviceIndex index) {
		WAVEFORMATEXTENSIBLE format = { 0 };
		const auto result = PaWasapi_GetDeviceDefaultFormat(&format, sizeof(format), index);
		if (result <= 0) throw std::runtime_error(std::string("Unable to get WASAPI device default format for device ") + std::to_string(index) + ": " + Pa_GetErrorText(result));
		return format;
	}

	std::string GetWaveFormatTagString(WORD formatTag) {
		return EnumToString(formatTag, {
			{ WAVE_FORMAT_EXTENSIBLE, "EXTENSIBLE" },
			{ WAVE_FORMAT_MPEG, "MPEG" },
			{ WAVE_FORMAT_MPEGLAYER3, "MPEGLAYER3" },
			});
	}

	std::string GetWaveFormatChannelMaskString(DWORD channelMask) {
		return BitfieldToString(channelMask, {
			{SPEAKER_FRONT_LEFT, "Front Left"},
			{SPEAKER_FRONT_RIGHT, "Front Right"},
			{SPEAKER_FRONT_CENTER, "Front Center"},
			{SPEAKER_LOW_FREQUENCY, "Low Frequency"},
			{SPEAKER_BACK_LEFT, "Back Left"},
			{SPEAKER_BACK_RIGHT, "Back Right"},
			{SPEAKER_FRONT_LEFT_OF_CENTER, "Front Left of Center"},
			{SPEAKER_FRONT_RIGHT_OF_CENTER, "Front Right of Center"},
			{SPEAKER_BACK_CENTER, "Back Center"},
			{SPEAKER_SIDE_LEFT, "Side Left"},
			{SPEAKER_SIDE_RIGHT, "Side Right"},
			{SPEAKER_TOP_CENTER, "Top Center"},
			{SPEAKER_TOP_FRONT_LEFT, "Top Front Left"},
			{SPEAKER_TOP_FRONT_CENTER, "Top Front Center"},
			{SPEAKER_TOP_FRONT_RIGHT, "Top Front Right"},
			{SPEAKER_TOP_BACK_LEFT, "Top Back Left"},
			{SPEAKER_TOP_BACK_CENTER, "Top Back Center"},
			{SPEAKER_TOP_BACK_RIGHT, "Top Back Right"},
			});
	}

	std::string GetWaveSubFormatString(const GUID& subFormat) {
		return EnumToString(subFormat, {
			{ KSDATAFORMAT_SUBTYPE_ADPCM, "ADPCM" },
			{ KSDATAFORMAT_SUBTYPE_ALAW, "A-law" },
			{ KSDATAFORMAT_SUBTYPE_DRM, "DRM" },
			{ KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL_PLUS, "IEC61937 Dolby Digital Plus" },
			{ KSDATAFORMAT_SUBTYPE_IEC61937_DOLBY_DIGITAL, "IEC61937 Dolby Digital" },
			{ KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, "IEEE Float" },
			{ KSDATAFORMAT_SUBTYPE_MPEG, "MPEG-1" },
			{ KSDATAFORMAT_SUBTYPE_MULAW, "Mu-law" },
			{ KSDATAFORMAT_SUBTYPE_PCM, "PCM" },
			}, [](const GUID& guid) {
			char str[128];
			// Shamelessly stolen from https://stackoverflow.com/a/18555932/172594
			snprintf(str, sizeof(str), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
			return std::string(str);
		});
	}

	std::string DescribeWaveFormat(const WAVEFORMATEXTENSIBLE& waveFormatExtensible) {
		const auto& waveFormat = waveFormatExtensible.Format;

		std::stringstream result;
		result << "WAVEFORMAT with format tag " << GetWaveFormatTagString(waveFormat.wFormatTag) << ", "
			<< waveFormat.nChannels << " channels, "
			<< waveFormat.nSamplesPerSec << " samples/second, "
			<< waveFormat.nAvgBytesPerSec << " average bytes/second, "
			<< "block alignment " << waveFormat.nBlockAlign << " bytes, "
			<< waveFormat.wBitsPerSample << " bits per sample";

		if (waveFormat.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
			result << ", " << waveFormatExtensible.Samples.wValidBitsPerSample << " valid bits per sample, "
				<< "channel mask " << GetWaveFormatChannelMaskString(waveFormatExtensible.dwChannelMask) << ", "
				<< "format " << GetWaveSubFormatString(waveFormatExtensible.SubFormat);
		}

		return result.str();
	}

	std::string DescribeStreamParameters(const PaStreamParameters& parameters) {
		std::stringstream result;

		result << "PortAudio stream parameters for device index " << parameters.device << ", "
			<< parameters.channelCount << " channels, sample format "
			<< GetSampleFormatString(parameters.sampleFormat) << ", suggested latency "
			<< parameters.suggestedLatency << "s";

		if (parameters.hostApiSpecificStreamInfo != nullptr) {
			const auto hostApiSpecificHeader = static_cast<const PaUtilHostApiSpecificStreamInfoHeader*>(parameters.hostApiSpecificStreamInfo);
			result << ", host API specific: " << hostApiSpecificHeader->size << " bytes structure, type "
				<< GetHostApiTypeIdString(hostApiSpecificHeader->hostApiType) << ", version "
				<< hostApiSpecificHeader->version;
			if (hostApiSpecificHeader->hostApiType == paWASAPI) {
				const auto wasapiSpecific = static_cast<const PaWasapiStreamInfo*>(parameters.hostApiSpecificStreamInfo);
				result << ", WASAPI specific: flags " << GetWasapiFlagsString(PaWasapiFlags(wasapiSpecific->flags)) << ", channel mask "
					<< GetWaveFormatChannelMaskString(wasapiSpecific->channelMask) << ", host processor output "
					<< wasapiSpecific->hostProcessorOutput << ", host processor input "
					<< wasapiSpecific->hostProcessorInput << ", thread priority "
					<< GetWasapiThreadPriorityString(wasapiSpecific->threadPriority) << ", stream category "
					<< GetWasapiStreamCategoryString(wasapiSpecific->streamCategory) << ", stream option "
					<< GetWasapiStreamOptionString(wasapiSpecific->streamOption);
			}
		}

		return result.str();
	}

	std::string DescribeStreamInfo(const PaStreamInfo& info) {
		std::stringstream result;
		result << "PortAudio stream info version " << info.structVersion << ", input latency "
			<< info.inputLatency << "s, output latency "
			<< info.outputLatency << "s, sample rate "
			<< info.sampleRate << " Hz";
		return result.str();
	}

	void StreamDeleter::operator()(PaStream* stream) throw() {
		Log() << "Closing PortAudio stream " << stream;
		const auto error = Pa_CloseStream(stream);
		if (error != paNoError)
			Log() << "Unable to close PortAudio stream: " << Pa_GetErrorText(error);
	}

	Stream OpenStream(const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters, double sampleRate, unsigned long framesPerBuffer, PaStreamFlags streamFlags, PaStreamCallback *streamCallback, void *userData) {
		Log() << "Opening PortAudio stream with...";
		Log() << "...input parameters: " << (inputParameters == nullptr ? "none" : DescribeStreamParameters(*inputParameters));
		Log() << "...output parameters: " << (outputParameters == nullptr ? "none" : DescribeStreamParameters(*outputParameters));
		Log() << "...sample rate: " << sampleRate << " Hz";
		Log() << "...frames per buffer: " << framesPerBuffer;
		Log() << "...stream flags: " << GetStreamFlagsString(streamFlags);
		Log() << "...stream callback: " << streamCallback << " (user data " << userData << ")";
		PaStream* stream = nullptr;
		const auto error = Pa_OpenStream(&stream, inputParameters, outputParameters, sampleRate, framesPerBuffer, streamFlags, streamCallback, userData);
		if (error != paNoError) throw std::runtime_error(std::string("unable to open PortAudio stream: ") + Pa_GetErrorText(error));
		if (stream == nullptr)throw std::runtime_error("Pa_OpenStream() unexpectedly returned null");
		Log() << "PortAudio stream opened: " << stream;
		return Stream(stream);
	}

	void StreamStopper::operator()(PaStream* stream) throw() {
		Log() << "Stopping PortAudio stream " << stream;
		const auto error = Pa_StopStream(stream);
		if (error != paNoError)
			Log() << "Unable to stop PortAudio stream: " << Pa_GetErrorText(error);
	}

	ActiveStream StartStream(PaStream* const stream) {
		Log() << "Starting PortAudio stream " << stream;
		const auto error = Pa_StartStream(stream);
		if (error != paNoError) throw std::runtime_error(std::string("unable to start PortAudio stream: ") + Pa_GetErrorText(error));
		Log() << "PortAudio stream started";
		return ActiveStream(stream);
	}

	std::string DescribeStreamCallbackTimeInfo(const PaStreamCallbackTimeInfo& streamCallbackTimeInfo) {
		std::stringstream result;
		result << "PortAudio stream callback time info with input buffer ADC time " << streamCallbackTimeInfo.inputBufferAdcTime << ", current time "
			<< streamCallbackTimeInfo.currentTime << ", output buffer DAC time "
			<< streamCallbackTimeInfo.outputBufferDacTime;
		return result.str();
	}

}

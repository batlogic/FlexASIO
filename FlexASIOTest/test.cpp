#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "..\ASIOSDK2.3.1\host\ginclude.h"
#include "..\ASIOSDK2.3.1\common\asio.h"
#include "..\FlexASIO\flexasio.h"

// The global ASIO driver pointer that the ASIO host library internally uses.
extern IASIO* theAsioDriver;

namespace flexasio_test {
	namespace {

		std::string_view GetASIOErrorString(ASIOError error) {
			switch (error) {
			case ASE_OK: return "ASE_OK";
			case ASE_SUCCESS: return "ASE_SUCCESS";
			case ASE_NotPresent: return "ASE_NotPresent";
			case ASE_HWMalfunction: return "ASE_HWMalfunction";
			case ASE_InvalidParameter: return "ASE_InvalidParameter";
			case ASE_InvalidMode: return "ASE_InvalidMode";
			case ASE_SPNotAdvancing: return "ASE_SPNotAdvancing";
			case ASE_NoClock: return "ASE_NoClock";
			case ASE_NoMemory: return "ASE_NoMemory";
			default: return "(unknown ASE error code)";
			}
		}

		std::string_view GetASIOSampleTypeString(ASIOSampleType sampleType) {
			switch (sampleType) {
			case ASIOSTInt16MSB: return "ASIOSTInt16MSB";
			case ASIOSTInt24MSB: return "ASIOSTInt24MSB";
			case ASIOSTInt32MSB: return "ASIOSTInt32MSB";
			case ASIOSTFloat32MSB: return "ASIOSTFloat32MSB";
			case ASIOSTFloat64MSB: return "ASIOSTFloat64MSB";
			case ASIOSTInt32MSB16: return "ASIOSTInt32MSB16";
			case ASIOSTInt32MSB18: return "ASIOSTInt32MSB18";
			case ASIOSTInt32MSB20: return "ASIOSTInt32MSB20";
			case ASIOSTInt32MSB24: return "ASIOSTInt32MSB24";
			case ASIOSTInt16LSB: return "ASIOSTInt16LSB";
			case ASIOSTInt24LSB: return "ASIOSTInt24LSB";
			case ASIOSTInt32LSB: return "ASIOSTInt32LSB";
			case ASIOSTFloat32LSB: return "ASIOSTFloat32LSB";
			case ASIOSTFloat64LSB: return "ASIOSTFloat64LSB";
			case ASIOSTInt32LSB16: return "ASIOSTInt32LSB16";
			case ASIOSTInt32LSB18: return "ASIOSTInt32LSB18";
			case ASIOSTInt32LSB20: return "ASIOSTInt32LSB20";
			case ASIOSTInt32LSB24: return "ASIOSTInt32LSB24";
			case ASIOSTDSDInt8LSB1: return "ASIOSTDSDInt8LSB1";
			case ASIOSTDSDInt8MSB1: return "ASIOSTDSDInt8MSB1";
			case  ASIOSTDSDInt8NER8: return "ASIOSTDSDInt8NER8";
			default: return "(unknown ASIO sample type)";
			}
		}

		ASIOError PrintError(ASIOError error) {
			std::cout << "-> " << GetASIOErrorString(error) << std::endl;
			return error;
		}

		std::optional<ASIODriverInfo> Init() {
			ASIODriverInfo asioDriverInfo = { 0 };
			asioDriverInfo.asioVersion = 2;
			std::cout << "ASIOInit(asioVersion = " << asioDriverInfo.asioVersion << ")" << std::endl;
			const auto initError = PrintError(ASIOInit(&asioDriverInfo));
			std::cout << "asioVersion = " << asioDriverInfo.asioVersion << " driverVersion = " << asioDriverInfo.asioVersion << " name = " << asioDriverInfo.name << " errorMessage = " << asioDriverInfo.errorMessage << " sysRef = " << asioDriverInfo.sysRef << std::endl;
			if (initError != ASE_OK) return std::nullopt;
			return asioDriverInfo;
		}

		std::pair<long, long> GetChannels() {
			std::cout << "ASIOGetChannels()" << std::endl;
			long numInputChannels, numOutputChannels;
			const auto error = PrintError(ASIOGetChannels(&numInputChannels, &numOutputChannels));
			if (error != ASE_OK) return { 0, 0 };
			std::cout << "Channel count: " << numInputChannels << " input, " << numOutputChannels << " output" << std::endl;
			return { numInputChannels, numOutputChannels };
		}

		struct BufferSize {
			long min = LONG_MIN;
			long max = LONG_MIN;
			long preferred = LONG_MIN;
			long granularity = LONG_MIN;
		};

		std::optional<BufferSize> GetBufferSize() {
			std::cout << "ASIOGetBufferSize()" << std::endl;
			BufferSize bufferSize;
			const auto error = PrintError(ASIOGetBufferSize(&bufferSize.min, &bufferSize.max, &bufferSize.preferred, &bufferSize.granularity));
			if (error != ASE_OK) return std::nullopt;
			std::cout << "Buffer size: min " << bufferSize.min << " max " << bufferSize.max << " preferred " << bufferSize.preferred << " granularity " << bufferSize.granularity << std::endl;
			return bufferSize;
		}

		std::optional<ASIOSampleRate> GetSampleRate() {
			std::cout << "ASIOGetSampleRate()" << std::endl;
			ASIOSampleRate sampleRate = NAN;
			const auto error = PrintError(ASIOGetSampleRate(&sampleRate));
			if (error != ASE_OK) return std::nullopt;
			std::cout << "Sample rate: " << sampleRate << std::endl;
			return sampleRate;
		}

		bool CanSampleRate(ASIOSampleRate sampleRate) {
			std::cout << "ASIOCanSampleRate(" << sampleRate << ")" << std::endl;
			return PrintError(ASIOCanSampleRate(sampleRate)) == ASE_OK;
		}

		bool SetSampleRate(ASIOSampleRate sampleRate) {
			std::cout << "ASIOSetSampleRate(" << sampleRate << ")" << std::endl;
			return PrintError(ASIOSetSampleRate(sampleRate)) == ASE_OK;
		}

		bool OutputReady() {
			std::cout << "ASIOOutputReady()" << std::endl;
			return PrintError(ASIOOutputReady()) == ASE_OK;
		}

		std::optional<ASIOChannelInfo> GetChannelInfo(long channel, ASIOBool isInput) {
			std::cout << "ASIOGetChannelInfo(channel = " << channel << " isInput = " << isInput << ")" << std::endl;
			ASIOChannelInfo channelInfo;
			channelInfo.channel = channel;
			channelInfo.isInput = isInput;
			if (PrintError(ASIOGetChannelInfo(&channelInfo)) != ASE_OK) return std::nullopt;
			std::cout << "isActive = " << channelInfo.isActive << " channelGroup = " << channelInfo.channelGroup << " type = " << GetASIOSampleTypeString(channelInfo.type) << " name = " << channelInfo.name << std::endl;
			return channelInfo;
		}

		void GetAllChannelInfo(std::pair<long, long> ioChannelCounts) {
			for (long inputChannel = 0; inputChannel < ioChannelCounts.first; ++inputChannel) GetChannelInfo(inputChannel, true);
			for (long outputChannel = 0; outputChannel < ioChannelCounts.second; ++outputChannel) GetChannelInfo(outputChannel, false);
		}

		// TODO: we should also test with not all channels active.
		std::vector<ASIOBufferInfo> CreateBuffers(std::pair<long, long> ioChannelCounts, long bufferSize, ASIOCallbacks callbacks) {
			std::vector<ASIOBufferInfo> bufferInfos;
			for (long inputChannel = 0; inputChannel < ioChannelCounts.first; ++inputChannel) {
				auto& bufferInfo = bufferInfos.emplace_back();
				bufferInfo.isInput = true;
				bufferInfo.channelNum = inputChannel;
			}
			for (long outputChannel = 0; outputChannel < ioChannelCounts.second; ++outputChannel) {
				auto& bufferInfo = bufferInfos.emplace_back();
				bufferInfo.isInput = false;
				bufferInfo.channelNum = outputChannel;
			}

			std::cout << "ASIOCreateBuffers(";
			for (const auto& bufferInfo : bufferInfos) {
				std::cout << "isInput = " << bufferInfo.isInput << " channelNum = " << bufferInfo.channelNum << " ";
			}
			std::cout << ", bufferSize = " << bufferSize << ", bufferSwitch = " << (void*)(callbacks.bufferSwitch) << " sampleRateDidChange = " << (void*)(callbacks.sampleRateDidChange) << " asioMessage = " << (void*)(callbacks.asioMessage) << " bufferSwitchTimeInfo = " << (void*)(callbacks.bufferSwitchTimeInfo) << ")" << std::endl;

			if (PrintError(ASIOCreateBuffers(bufferInfos.data(), long(bufferInfos.size()), bufferSize, &callbacks)) != ASE_OK) return {};
			return bufferInfos;
		}

		bool Run() {
			if (!Init()) return false;

			std::cout << std::endl;

			const auto ioChannelCounts = GetChannels();
			if (ioChannelCounts.first == 0 && ioChannelCounts.second == 0) return false;

			std::cout << std::endl;

			const auto bufferSize = GetBufferSize();
			if (!bufferSize.has_value()) return false;

			std::cout << std::endl;

			GetSampleRate();

			std::cout << std::endl;

			for (const auto sampleRate : { 44100, 96000, 192000, 48000 }) {
				if (!(CanSampleRate(sampleRate) && SetSampleRate(sampleRate) && GetSampleRate() == sampleRate) && sampleRate == 48000) return false;
			}

			std::cout << std::endl;

			OutputReady();

			std::cout << std::endl;

			GetAllChannelInfo(ioChannelCounts);

			std::cout << std::endl;

			ASIOCallbacks callbacks;
			callbacks.bufferSwitch = [](long, ASIOBool) {};
			callbacks.sampleRateDidChange = [](ASIOSampleRate) {};
			callbacks.asioMessage = [](long, long, void*, double*) -> long { return 0; };
			callbacks.bufferSwitchTimeInfo = [](ASIOTime*, long, ASIOBool) -> ASIOTime* { return nullptr; };
			const auto buffers = CreateBuffers(ioChannelCounts, bufferSize->preferred, callbacks);
			if (buffers.size() == 0) return false;

			std::cout << std::endl;

			GetSampleRate();
			GetAllChannelInfo(ioChannelCounts);

			// Note: we don't call ASIOExit() because it gets confused by our driver setup trickery (see InitAndRun()).
			// That said, this doesn't really matter because ASIOExit() is basically a no-op in our case, anyway.
			return true;
		}

		bool InitAndRun() {
			// This basically does an end run around the ASIO host library driver loading system, simulating what loadAsioDriver() does.
			// This allows us to trick the ASIO host library into using a specific instance of an ASIO driver (the one this program is linked against),
			// as opposed to whatever ASIO driver might be currently installed on the system.
			theAsioDriver = CreateFlexASIO();

			const bool result = Run();

			ReleaseFlexASIO(theAsioDriver);
			theAsioDriver = nullptr;

			return result;
		}

	}
}

int main(int, char**) {
	if (!::flexasio_test::InitAndRun()) return 1;
	return 0;
}
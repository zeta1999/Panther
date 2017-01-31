
#include "encoding.h"
#include "utils.h"
#include "logger.h"

#include <malloc.h>
#include <algorithm>

#include "unicode.h"
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

struct PGEncoder {
	PGFileEncoding source_encoding;
	PGFileEncoding target_encoding;
	UConverter* source = nullptr;
	UConverter* target = nullptr;
};

std::string PGEncodingToString(PGFileEncoding encoding) {
	switch (encoding) {
	case PGEncodingUTF8:
		return "UTF-8";
	case PGEncodingUTF8BOM:
		return "UTF-8 with BOM";
	case PGEncodingUTF16:
		return "UTF-16";
	case PGEncodingUTF16Platform:
		return "UTF-16 Platform";
	case PGEncodingUTF32:
		return "UTF-32";
	case PGEncodingUTF16BE:
		return "UTF-16 BE";
	case PGEncodingUTF16BEBOM:
		return "UTF-16 BE with BOM";
	case PGEncodingUTF16LE:
		return "UTF-16 LE";
	case PGEncodingUTF16LEBOM:
		return "UTF-16 LE with BOM";
	case PGEncodingUTF32BE:
		return "UTF-32 BE";
	case PGEncodingUTF32BEBOM:
		return "UTF-32 BE with BOM";
	case PGEncodingUTF32LE:
		return "UTF-32 LE";
	case PGEncodingUTF32LEBOM:
		return "UTF-32 LE with BOM";
	default:
		return "Unknown";
	}
	return "";
}

static const char* GetEncodingName(PGFileEncoding encoding) {
	switch (encoding) {
	case PGEncodingUTF8:
	case PGEncodingUTF8BOM:
		return "UTF-8";
	case PGEncodingUTF16:
		return "UTF-16";
	case PGEncodingUTF16Platform:
		return "UTF16_PlatformEndian";
	case PGEncodingUTF32:
		return "UTF-32";
	case PGEncodingUTF16BE:
	case PGEncodingUTF16BEBOM:
		return "UTF-16BE";
	case PGEncodingUTF16LE:
	case PGEncodingUTF16LEBOM:
		return "UTF-16LE";
	case PGEncodingUTF32BE:
	case PGEncodingUTF32BEBOM:
		return "UTF-16BE";
	case PGEncodingUTF32LE:
	case PGEncodingUTF32LEBOM:
		return "UTF-32LE";
	case PGEncodingWesternISO8859_1:
		return "ISO-8859-1";
	case PGEncodingNordicISO8859_10:
		return "iso-8859_10-1998";
	case PGEncodingCelticISO8859_14:
		return "iso-8859_14-1998";
	default:
		assert(0);
	}
	return nullptr;
}

static PGFileEncoding GetEncodingFromName(std::string encoding) {
	if (encoding == "UTF-8") {
		return PGEncodingUTF8;
	} else if (encoding == "UTF-16") {
		return PGEncodingUTF16;
	} else if (encoding == "UTF16_PlatformEndian") {
		return PGEncodingUTF16Platform;
	} else if (encoding == "UTF-32") {
		return PGEncodingUTF32;
	} else if (encoding == "UTF-16BE") {
		return PGEncodingUTF16BE;
	} else if (encoding == "UTF-16LE") {
		return PGEncodingUTF16LE;
	} else if (encoding == "UTF-32BE") {
		return PGEncodingUTF32BE;
	} else if (encoding == "UTF-32LE") {
		return PGEncodingUTF32LE;
	} else if (encoding == "ISO-8859-1") {
		return PGEncodingWesternISO8859_1;
	} else if (encoding == "iso-8859_10-1998") {
		return PGEncodingNordicISO8859_10;
	} else if (encoding == "iso-8859_14-1998") {
		return PGEncodingCelticISO8859_14;
	}
	assert(0);
	return PGEncodingUTF8;
}

void LogAvailableEncodings() {
	/* Returns count of the number of available names */
	int count = ucnv_countAvailable();

	/* get the canonical name of the 36th available converter */
	for (int i = 0; i < count; i++) {
		Logger::WriteLogMessage(std::string(ucnv_getAvailableName(i)));
	}
}

PGEncoderHandle PGCreateEncoder(PGFileEncoding source_encoding, PGFileEncoding target_encoding) {
	UErrorCode error = U_ZERO_ERROR;

	// assure that both encodings are implemented
	assert(GetEncodingName(source_encoding) && GetEncodingName(target_encoding));

	PGEncoderHandle handle = new PGEncoder();
	handle->source_encoding = source_encoding;
	handle->target_encoding = target_encoding;

	// create the converters
	handle->source = ucnv_open(GetEncodingName(source_encoding), &error);
	if (U_FAILURE(error)) {
		// failed to create a converter
		delete handle;
		return nullptr;
	}
	handle->target = ucnv_open(GetEncodingName(target_encoding), &error);
	if (U_FAILURE(error)) {
		// failed to create a converter
		ucnv_close(handle->source);
		delete handle;
		return nullptr;
	}

	return handle;
}

lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {
	return PGConvertText(encoder, input.c_str(), input.size(), output, output_size, intermediate_buffer, intermediate_size);
}

lng PGConvertText(PGEncoderHandle encoder, const char* input_text, size_t input_size, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {
	lng return_size = -1;
	char* result_buffer = nullptr;
	UChar* buffer = nullptr;
	UErrorCode error = U_ZERO_ERROR;

	// first we convert the source encoding to the internal ICU representation (UChars)
	size_t targetsize = *intermediate_size;
	buffer = *((UChar**)intermediate_buffer);
	targetsize = ucnv_toUChars(encoder->source, buffer, targetsize, input_text, input_size, &error);
	if (error == U_BUFFER_OVERFLOW_ERROR) {
		error = U_ZERO_ERROR;
		if (buffer)
			free(buffer);
		buffer = (UChar*)malloc(targetsize * sizeof(UChar));
		*intermediate_buffer = (char*)buffer;
		*intermediate_size = targetsize;
		targetsize = ucnv_toUChars(encoder->source, buffer, targetsize, input_text, input_size, &error);
	}
	if (U_FAILURE(error)) {
		// failed source conversion
		return -1;
	}
	// now convert the source to the target encoding
	size_t result_size = targetsize * 4;
	if (*output_size < result_size) {
		result_buffer = (char*)malloc(result_size);
		*output_size = result_size;
		*output = result_buffer;
	} else {
		result_buffer = *output;
	}
	result_size = ucnv_fromUChars(encoder->target, result_buffer, result_size, buffer, targetsize, &error);
	if (U_FAILURE(error)) {
		// failed source conversion
		return -1;
	}
	return result_size;
}


lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output) {
	lng output_size = 0;
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;

	lng result_size = PGConvertText(encoder, input, output, &output_size, &intermediate_buffer, &intermediate_size);
	if (intermediate_buffer) free(intermediate_buffer);
	return result_size;
}

void PGDestroyEncoder(PGEncoderHandle handle) {
	if (handle) {
		if (handle->source) {
			ucnv_close(handle->source);
		}
		if (handle->target) {
			ucnv_close(handle->target);
		}
		delete handle;
	}
}

lng PGConvertText(std::string input, char** output, PGFileEncoding source_encoding, PGFileEncoding target_encoding) {
	PGEncoderHandle encoder = PGCreateEncoder(source_encoding, target_encoding);
	if (!encoder) return -1;
	lng return_size = PGConvertText(encoder, input, output);
	PGDestroyEncoder(encoder);
	return return_size;
}

bool PGTryConvertToUTF8(char* input_text, size_t input_size, char** output_text, lng* output_size, PGFileEncoding* result_encoding) {
	UCharsetDetector* csd = nullptr;
	const UCharsetMatch *ucm = nullptr;
	UErrorCode status = U_ZERO_ERROR;
	
	*output_text = nullptr;
	*output_size = 0;

	csd = ucsdet_open(&status);
	if (U_FAILURE(status)) {
		return false;
	}
	ucsdet_setText(csd, input_text, std::min((size_t) 1024, input_size), &status);
	if (U_FAILURE(status)) {
		return false;
	}
	ucm = ucsdet_detect(csd, &status);
	if (U_FAILURE(status)) {
		return false;
	}
	const char* encoding = ucsdet_getName(ucm, &status);
	if (U_FAILURE(status) || encoding == nullptr) {
		return false;
	}
	ucsdet_close(csd);
	PGFileEncoding source_encoding = GetEncodingFromName(encoding);
	auto encoder = PGCreateEncoder(source_encoding, PGEncodingUTF8);
	if (!encoder) {
		return false;
	}
	*result_encoding = source_encoding;
	if (source_encoding == PGEncodingUTF8) {
		*output_text = input_text;
		*output_size = input_size;
		return true;
	}
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;
	if (PGConvertText(encoder, input_text, input_size, output_text, output_size, &intermediate_buffer, &intermediate_size) > 0) {
		return true;
	}
	return false;
}

std::string utf8_tolower(std::string str) {
	UErrorCode status = U_ZERO_ERROR;
	UCaseMap* casemap = ucasemap_open(nullptr, 0, &status);

	size_t destsize = str.size() * sizeof(char) + 1;
	char* dest = (char*)malloc(destsize);
	size_t result_size = ucasemap_utf8ToLower(casemap, dest, destsize, str.c_str(), str.size(), &status);
	assert(result_size <= destsize);
	std::string result = std::string(dest, result_size);
	free(dest);
	ucasemap_close(casemap);
	return result;
}
#pragma once

#include "utils.h"

#include <string>

typedef enum {
	PGEncodingUTF8,
	PGEncodingUTF8BOM,
	PGEncodingUTF16,
	PGEncodingUTF32,
	PGEncodingUTF16LE,
	PGEncodingUTF16LEBOM,
	PGEncodingUTF16BE,
	PGEncodingUTF16BEBOM,
	PGEncodingUTF16Platform,
	PGEncodingUTF32LE,
	PGEncodingUTF32LEBOM,
	PGEncodingUTF32BE,
	PGEncodingUTF32BEBOM,
	PGEncodingWesternWindows1252,
	PGEncodingWesternISO8859_1,
	PGEncodingWesternISO8859_3,
	PGEncodingWesternISO8859_15,
	PGEncodingWesternMacRoman,
	PGEncodingDOS,
	PGEncodingArabicWindows1256,
	PGEncodingArabicISO8859_6,
	PGEncodingBalticWindows1257,
	PGEncodingBalticISO8859_4,
	PGEncodingCelticISO8859_14,
	PGEncodingCentralEuropeanWindows1250,
	PGEncodingCentralEuropeanISO8859_2,
	PGEncodingCyrillicWindows1251,
	PGEncodingCyrillicWindows866,
	PGEncodingCyrillicISO8859_5,
	PGEncodingCyrillicKOI8_R,
	PGEncodingCyrillicKOI8_U,
	PGEncodingEstonianISO8859_13,
	PGEncodingGreekWindows1253,
	PGEncodingGreekISO8859_7,
	PGEncodingHebrewWindows1255,
	PGEncodingHebrewISO8859_8,
	PGEncodingNordicISO8859_10,
	PGEncodingRomanianISO8859_16,
	PGEncodingTurkishWindows1254,
	PGEncodingTurkishISO8859_9,
	PGEncodingVietnameseWindows1258
} PGFileEncoding;

//! Convert PGFileEncoding to a human-readable string
std::string PGEncodingToString(PGFileEncoding);

struct PGEncoder;
typedef PGEncoder* PGEncoderHandle;

// Tools for incremental conversion, can be used if you want to repeatedly encode chunks of text
PGEncoderHandle PGCreateEncoder(PGFileEncoding source_encoding, PGFileEncoding target_encoding);
lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output);
void PGDestroyEncoder(PGEncoderHandle);

lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size);
// Performs a single conversion, useful if you just want to convert some text
lng PGConvertText(std::string input, char** output, PGFileEncoding source_encoding, PGFileEncoding target_encoding);

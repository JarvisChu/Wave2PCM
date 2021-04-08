#include <stdio.h>
#include <stdint.h>

/* Wave Header Format

// format 子块
struct SubChunkFormat
{
	uint32_t chunk;           // 固定为fmt ，大端存储
	uint32_t subchunk_size;   // 子块的大小，不包含chunk+subchunk_size字段
	uint16_t audio_format;    // 编码格式(Audio Format)，1代表PCM无损格式
	uint16_t channels;        // 声道数(Channels)，1或2
	uint32_t sample_rate;     // 采样率(Sample Rate)
	uint32_t byte_rate;       // 传输速率(Byte Rate)，每秒数据字节数，SampleRate * Channels * BitsPerSample / 8
	uint16_t block_align;     // 每个采样所需的字节数BlockAlign，BitsPerSample*Channels / 8
	uint16_t bits_per_sample; // 单个采样位深(Bits Per Sample)，可选8、16或32
	uint16_t ex_size;         // 扩展块的大小，附加块的大小
};

// data 子块
struct SubChunkData
{
	uint32_t chunk;         // 固定为data，大端存储
	uint32_t subchunk_size; // 子块的大小，不包含chunk+subchunk_size字段
};

struct WaveHeader
{
	uint32_t fourcc;     // 固定为RIFF，大端存储，所以需要使用MAKE_FOURCC宏处理
	uint32_t chunk_size; // 文件长度，不包含fourcc和chunk_size，即总文件长度-8字节
	uint32_t form_type;  // 固定为WAVE，大端存储，类型码(Form Type)，WAV文件格式标记，即"WAVE"四个字母

	SubChunkFormat subchunk_format; // fmt  子块
	SubChunkData subchunk_data;     // data 子块
};
*/

static void print_usage(char* argv[]) {
    printf( "\nusage: %s in.wav out.pcm\n", argv[ 0 ] );
    printf( "\nin.wav       : wave file to input" );
    printf( "\nout.pcm      : pcm file to output" );
    printf( "\n" );
}

int Wave2PCM(const char* wav_file, const char* pcm_file)
{
	FILE *fp_wav = nullptr;
	errno_t err = fopen_s(&fp_wav, wav_file, "rb");
	if (err != 0) {
		printf("open file failed, %s\n", wav_file);
		return -1;
	}

	FILE *fp_pcm = nullptr;
	err = fopen_s(&fp_pcm, pcm_file, "wb");
	if (err != 0) {
		printf("open file failed, %s\n", pcm_file);
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	// parse wave file

	// Get file size
	fseek(fp_wav, 0L, SEEK_END);
	long file_size = ftell(fp_wav);
	fseek(fp_wav, 0L, SEEK_SET);

	// Chunk RIFF
	uint8_t fourcc[4];
	size_t read_cnt = fread_s(fourcc, 4, sizeof(uint8_t), 4, fp_wav);
	if (fourcc[0] != 'R' || fourcc[1] != 'I' || fourcc[2] != 'F' || fourcc[3] != 'F') {
		printf("invalid wave file, %s\n", pcm_file);
		return -1;
	}

	// Chunk RIFF size
	uint32_t riff_chunk_size = 0;
	read_cnt = fread_s(&riff_chunk_size, sizeof(uint32_t), sizeof(uint32_t), 1, fp_wav);
	if (read_cnt < 1){
		printf("invalid wave file, %s\n", pcm_file);
		return -1;
	}

	if ((uint32_t)file_size != riff_chunk_size + sizeof(fourcc) + sizeof(riff_chunk_size)) {
		printf("invalid wave file, size not match, %s\n", pcm_file);
		return -1;
	}

	// Chunk RIFF form type
	uint8_t form_type[4];
	read_cnt = fread_s(form_type, 4, sizeof(uint8_t), 4, fp_wav);
	if (form_type[0] != 'W' || form_type[1] != 'A' || form_type[2] != 'V' || form_type[3] != 'E') {
		printf("invalid wave file, %s\n", pcm_file);
		return -1;
	}

	// Find sub chunk "data"
	while (true) {
		uint8_t sub_chunk_name[4];
		read_cnt = fread_s(sub_chunk_name, 4, sizeof(uint8_t), 4, fp_wav);
		if (read_cnt < 4) {
			printf("read sub chunk failed\n");
			return -1;
		}
		printf("find sub chunk: %c%c%c%c\n", sub_chunk_name[0], sub_chunk_name[1], sub_chunk_name[2], sub_chunk_name[3]);
		if (sub_chunk_name[0] == 'd' && sub_chunk_name[1] == 'a' && sub_chunk_name[2] == 't' && sub_chunk_name[3] == 'a') {
			break;
		}

		uint32_t sub_chunk_size = 0;
		read_cnt = fread_s(&sub_chunk_size, sizeof(uint32_t), sizeof(uint32_t), 1, fp_wav);
		if (read_cnt < 1) {
			printf("invalid wave file, %s\n", pcm_file);
			return -1;
		}

		// if not data sub chunk, skip it
		fseek(fp_wav, sub_chunk_size, SEEK_CUR);
	}

	uint32_t data_sub_chunk_size = 0;
	read_cnt = fread_s(&data_sub_chunk_size, sizeof(uint32_t), sizeof(uint32_t), 1, fp_wav);
	if (read_cnt < 1) {
		printf("invalid wave file, %s\n", pcm_file);
		return -1;
	}

	printf("data sub chunk size: %d\n", data_sub_chunk_size);

	while (data_sub_chunk_size > 0) {
		uint8_t buff[2048];
		int len = (data_sub_chunk_size <= 2048) ? data_sub_chunk_size:2048;
		read_cnt = fread_s(buff, len, sizeof(uint8_t), len, fp_wav);
		if (read_cnt != len) {
			printf("invalid wave file, %s\n", pcm_file);
			return -1;
		}

		fwrite(buff, sizeof(uint8_t), len, fp_pcm);

		data_sub_chunk_size -= len;
	}

	fclose(fp_wav);
	fclose(fp_pcm);
	printf("convert %s to %s success!\n", wav_file, pcm_file);
    return 0;
}

int main(int argc, char* argv[])
{
    if(argc < 3){
        print_usage(argv);
        return -1;
    }

	printf("convert %s to %s\n", argv[1], argv[2]);
	return Wave2PCM(argv[1], argv[2]);
}
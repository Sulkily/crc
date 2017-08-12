#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

//x^16 + x^15 + x^2 + 1
//#define POLYNOM16 49155 // falsches polynom
#define POLYNOM16 0x18005
#define BITMASK_HIGHBIT 4611686018427387904

__uint64_t getFileSize(FILE *fileDescriptor)
{
	__uint64_t number = 0;
	/// Check for real filedescriptor
	if (fileDescriptor != NULL)
	{
		// take a file descriptor and walk over the file until the pointer reaches the end
		if(fseek(fileDescriptor, 0L, SEEK_END) == -1)
		{
			fprintf(stderr, "ERROR: Seeking end failed. Errno information: %s.\n", strerror(errno));
		}
		// get the position of the pointer to calculate the length of the file and then reset the pointer
		
		int i = -1;
		if ((i = ftell(fileDescriptor)) == -1)
		{
			fprintf(stderr, "Error: Calculating filesize failed. Errno information: %s.\n", strerror(errno));
		}
		else 
		{
			number = i;
		}
		rewind(fileDescriptor);
	}
	return number;
}


int getOffset(__uint64_t number)
{
	__uint64_t max = BITMASK_HIGHBIT;

	/// Check for number getting to big
	// Leftshifting until the most significant bit is 1
	for (int i = 0; i < 64 ; number = number << 1)
	{
		// return numbe of shifts needed
		if(!(number < max))
			return i;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	FILE *fileDescriptor = NULL;
	__uint64_t *buffer = NULL;

	__uint64_t number = 0;
	__uint64_t polynom = POLYNOM16;

	int CRC16 = 1, TXT = 1;
	
	/// Check for no file given
	if (argc > 1)
	{
		/// Check for invalid file given
		// Open file
		if ((fileDescriptor = fopen(argv[1], "rb")) != NULL)
		{

			// Separate into files.txt and files.crc
			if (CRC16 == 1)
			{


				while (!feof(fileDescriptor))
				{
					// fill's the buffer with a 32bit number
					while (fread(buffer, 4, 1, fileDescriptor) != 0)
					{
						// Move the number to the left to make room for new digits from the file to extend it with
						number = number << 32;
						number += *buffer;
						// Move the polynomial to the left to keep it at a level with the number
						polynom = polynom << getOffset(number);

						// Divide the number by the polynomial
						number = number^polynom;
					}
				}
				// reset pointer
				rewind(fileDescriptor);
				/// Check for matching file and checksum
				if (number != 0)
				{
					fprintf(stderr, "Error: File got changed after the checksum was calculated.\n");
				}
				else
				{
					/// Check for truncating failure
					// cut the checksum off
					if (ftruncate(fileno(fileDescriptor), (off_t)(getFileSize(fileDescriptor) - 2)) == -1)
					{
						fprintf(stderr, "Error: Truncating file failed. Errno information: %s.\n", strerror(errno));
					}
					else
					{
						FILE *fileDescriptorWithoutChecksum = fopen("test.txt", "wb");

						// write to file
						while (!feof(fileDescriptor))
						{
							unsigned char buffer[8];

							fread(buffer, sizeof(buffer), 1, fileDescriptor);
							fwrite(buffer, sizeof(buffer), 1, fileDescriptorWithoutChecksum);
						}
						fclose(fileDescriptorWithoutChecksum);
					}
				}
			}
			else if (TXT == 1)
			{


				while (!feof(fileDescriptor))
				{
					// fill's the buffer with a 32bit number
					while (fread(buffer, 4, 1, fileDescriptor) != 0)
					{
						// Move the number to the left to make room for new digits from the file to extend it with
						number = number << 32;
						number += *buffer;
						// Move the polynomial to the left to keep it at a level with the number
						polynom = polynom << getOffset(number);

						// Divide the number by the polynomial
						number = number^polynom;
					}
				}
				// reset pointer
				rewind(fileDescriptor);

				// CRC16 Algorithm extends the number with zeros
				number = number << 16;
				polynom = polynom << getOffset(number);

				// Divide the number by the polynomial
				number = number^polynom;



				FILE *fileDescriptorChecksum = fopen("test.crc", "wb");
				
				// write to file
				while (!feof(fileDescriptor))
				{
					unsigned char buffer[8];

					fread(buffer, sizeof(buffer), 1, fileDescriptor);
					fwrite(buffer, sizeof(buffer), 1, fileDescriptorChecksum);
				}
				// write Checksum after the file
				__uint16_t number16 = (__uint16_t)number;
				fwrite(&number16, sizeof(number16), 1, fileDescriptorChecksum);
				fclose(fileDescriptorChecksum);
			}
			else
			{
				fprintf(stderr, "Error: Wrong fileextension.\n");
			}
		}
		else
		{
			fprintf(stderr, "Error: Could not open file. Errno information: %s.\n", strerror(errno));
		}
	}
	else
	{
		fprintf(stderr, "Error: No file given.\n");
	}

	return 0;
}

// Author : Bilal Mahmud
// MD5 FPGA Implementation And Statistics

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <fcntl.h> 
#include <time.h> 
#include <sys/mman.h> 
#include "hwlib.h" 
#include "socal/socal.h" 
#include "socal/hps.h" 
#include "socal/alt_gpio.h" 
#include "hps_0.h" 
 
#define LW_SIZE 0x00200000 
#define LWHPS2FPGA_BASE 0xff200000 
 
// MD5 Control and Data Registers
volatile uint32_t *md5_group_control = NULL; 
volatile uint32_t *md5_group_data = NULL; 
int success, total; 

// MD5 Formulas for each round
uint32_t func(int input,  uint32_t B,  uint32_t C,  uint32_t D){
    if (input == 1) return (B & C) | (~B & D);
    if (input == 2) return (B & D) | (C & ~D);
    if (input == 3) return B ^ C ^ D;
    if (input == 4) return C ^ (B | ~D);
}

// Left Rotate
uint32_t left_rotate (uint32_t input , int rot_amnt){
    uint32_t rotate_bits = 32 - rot_amnt;
    uint32_t rotate_ans = input >> rotate_bits;
    return (input << rot_amnt) + rotate_ans;
}

// MD% Software Implementation
void md5_calc_soft(){

    // Variable Setup
    uint32_t addr_data = 0x0, A = 0x67452301, B = 0xefcdab89, C = 0x98badcfe, D = 0x10325476;
    uint32_t AA = A, BB = B, CC = C, DD = D, temp1, temp2, temp3, temp4, final_temp, temp;
    int core, i, j, k, engine, hash_rate = 0;
    struct timespec start_time, stop_time;
    double elapsed_time, total_elapsed_time = 0, TIME1, TIME2, rate;
    char *word = "hello", *word2 = "SystemOnChip";
    uint32_t correct_hash[4] = {0x5d41402a,0xbc4b2a76,0xb9719d91,0x1017c592};
    uint32_t correct_hash2[4] = {0xbe7f51b1,0xae4e17d9,0xf5b9670f,0xa5a6fbc4};
    int rotate[4][4] = {{7, 12, 17, 22}, {5, 9, 14, 20}, {4, 11, 16, 23}, {6, 10, 15, 21}};


    uint32_t md5_input[16] = {  // hello
            0x6c6c6568, 0x0000806f, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000028, 0x00000000}; 
    uint32_t md5_input2[16] = { //SystemOnChip
                0x74737953, 0x6E4F6D65, 0x70696843, 0x00000080, 
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                0x00000000, 0x00000000, 0x00000060, 0x00000000}; 
    uint32_t  T[64];
    
    for (i  = 0 ; i < 64; i++){
            T[i] = floor(pow(2,32) * fabs(sin(i + 1)));
    }



    // For all 32 engines 
     for (engine = 0; engine < 32; engine ++){ 
        A = 0x67452301;
        B = 0xefcdab89;
        C = 0x98badcfe;
        D = 0x10325476;

        clock_gettime(CLOCK_MONOTONIC, &start_time); 
        TIME1 = start_time.tv_sec * (1000000000) + start_time.tv_nsec; 

        for (i  = 0 ; i < 64; i++){
            if (i >= 0 && i <= 15){
                k = i;
                if (engine % 2 == 0){
                    temp = (A + func(1, B, C, D) + md5_input[k] + T[i]);
                }
                else{
                     temp = (A + func(1, B, C, D) + md5_input2[k] + T[i]);
                }
                temp = left_rotate(temp, rotate[0][i % 4]) + B;

            }
            else if (i >= 16 && i <= 31){
                k = (5*i + 1) % 16;
                if (engine % 2 == 0){
                    temp = (A + func(2, B, C, D) + md5_input[k] + T[i]);
                }
                else{
                     temp = (A + func(2, B, C, D) + md5_input2[k] + T[i]);
                }
                temp = left_rotate(temp, rotate[1][i % 4]) + B;

            }
            else if (i >= 32 && i <= 47){
                k = (3*i + 5) % 16;
                if (engine % 2 == 0){
                    temp = (A + func(3, B, C, D) + md5_input[k] + T[i]);
                }
                else{
                     temp = (A + func(3, B, C, D) + md5_input2[k] + T[i]);
                }
                temp = left_rotate(temp, rotate[2][i % 4]) + B;

            }
            else{
                k = (7*i) % 16;
                if (engine % 2 == 0){
                    temp = (A + func(4, B, C, D) + md5_input[k] + T[i]);
                }
                else{
                     temp = (A + func(4, B, C, D) + md5_input2[k] + T[i]);
                }
                temp = left_rotate(temp, rotate[3][i % 4]) + B;

            }

            A = D;
            D = C;
            C = B;
            B = temp;
        }

        A = A + AA;
        B = B + BB;
        C = C + CC;
        D = D + DD;

        uint32_t output[4] = {A, B, C, D};

        // Taking Calculation Time
        clock_gettime(CLOCK_MONOTONIC, &stop_time); 
        TIME2 = stop_time.tv_sec * (1000000000) + stop_time.tv_nsec; 
        elapsed_time = TIME2 - TIME1; 
        total_elapsed_time += elapsed_time;

        // Output is of Little Endian, oinvert to Big Endian
        for (i = 0; i < 4; i++){
            temp1 = output[i] & 0xFF000000; 
            temp2 = output[i] & 0x00FF0000; 
            temp3 = output[i] & 0x0000FF00; 
            temp4 = output[i] & 0x000000FF; 
            final_temp = (temp4 << 24)|(temp3 << 8)| (temp2 >> 8) | (temp1 >> 24); 
            output[i] = final_temp; 
        }


        // Outputting Results and Statistics
        printf("\n---------------------------- ENGINE %d ---------------------------------\n\n", engine + 1);  
        if (engine % 2 ==0){
            printf("Digest for '%s' is %08x %08x %08x %08x\nCalculation Time: %0.8f nsec", word, output[0], output[1], output[2], output[3], elapsed_time);
             if (correct_hash[0] == output[0] && correct_hash[1] == output[1] && correct_hash[2] == output[2] && correct_hash[3] == output[3]){ 
                    printf("\nThis matches the correct hash value for: %s\n", word); 
                    hash_rate++;
            } 
            else{ 
                    printf("\nThis does not match the correct hash value for: %s\n\n", word); 
            } 
        }
        else{
            printf("Digest for '%s' is %08x %08x %08x %08x\nCalculation Time: %0.8f nsec", word2, output[0], output[1], output[2], output[3], elapsed_time);
            if (correct_hash2[0] == output[0] && correct_hash2[1] == output[1] && correct_hash2[2] == output[2] && correct_hash2[3] == output[3]){ 
                    printf("\nThis matches the correct hash value for: %s\n", word2); 
                    hash_rate++;
            } 
            else{ 
                    printf("\nThis does not match the correct hash value for: %s\n\n", word2); 
            } 
        }

        }
        rate = (hash_rate / total_elapsed_time) * (1e9);
        printf("\n\n---------------------------- STATISTICS --------------------------------\n\n"); 
        printf("Number of hashes: %d", hash_rate + (32- hash_rate)); 
        printf("\nTotal correct hashes: %d       hash rate:  %.2f hashes/sec\nTotal incorrect hashes: %d", hash_rate, rate, 32-hash_rate); 
        printf("\nTotal calculation time: %0.8f usec\n\n", (total_elapsed_time / 1000)); 
    
    }
 
void md5_calc_seq(){ 
 
    // Variable Setup
    int i = 0; 
    int engine = 0; 
    struct timespec start_time, stop_time; 
    double elapsed_time; 
    double total_elapsed_time = 0.0; 
    char *word = "hello"; 
    char *word2 = "SystemOnChip"; 
    double TIME1, TIME2; 
    int hash_rate = 0; 
    uint32_t temp1; 
    uint32_t temp2; 
    uint32_t temp3; 
    uint32_t temp4; 
    uint32_t final_temp; 
 
 
    uint32_t writeaddr = 0x0; 
    uint32_t readaddr = 0x0; 
    uint32_t correct_hash[4] = {0x5d41402a, 0xbc4b2a76, 0xb9719d91, 0x1017c592}; 
    uint32_t correct_hash2[4] = {0xbe7f51b1, 0xae4e17d9, 0xf5b9670f, 0xa5a6fbc4}; 
    uint32_t hashed[32][4]; 
    uint32_t engine_bit = 0x0; 
    uint32_t md5_input[16] = {  // hello
            0x6c6c6568, 0x0000806f, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000028, 0x00000000}; 
    uint32_t md5_input2[16] = {  // SystemOnChip
                0x74737953, 0x6E4F6D65, 0x70696843, 0x00000080, 
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                0x00000000, 0x00000000, 0x00000060, 0x00000000}; 

    // Resetting System and writing to bus
    for (engine = 0; engine < 32; engine ++){ 
        alt_write_word(md5_group_control +1, 0xFFFFFFFF); 
        alt_write_word(md5_group_control +1, 0x00000000); 
 
        for (i =0; i<16;i++){ 
                readaddr = (readaddr + engine) << 4; 
                alt_write_word(md5_group_control + 2, 0x0); 
                if (engine % 2 ==0){ 
                    alt_write_word(md5_group_data, md5_input[i]); // writedata 
                } 
                else{ 
                    alt_write_word(md5_group_data, md5_input2[i]); // writedata 
                } 
                alt_write_word(md5_group_data + 1, readaddr + i); // writeaddr 
                alt_write_word(md5_group_control + 2, 0x1); // wr 
                readaddr = 0x0; 
        } 
 
        alt_write_word(md5_group_control + 2, 0x0); // wr 
 
        clock_gettime(CLOCK_REALTIME, &start_time); 
        TIME1 = start_time.tv_sec * (1000000000) + start_time.tv_nsec; 
 
        engine_bit =  0x1 << engine; 
 
        alt_write_word(md5_group_control, engine_bit); 
        alt_write_word(md5_group_control, 0x0); 
 
        while (!(alt_read_word(md5_group_control + 2) &engine_bit)); // loop till done 
 
        clock_gettime(CLOCK_REALTIME, &stop_time); 
        TIME2 = stop_time.tv_sec * (1000000000) + stop_time.tv_nsec; 
        elapsed_time = TIME2 - TIME1; 
        total_elapsed_time = total_elapsed_time + elapsed_time; 
 
        readaddr =0x0; 
        for (i =0; i<4;i++){ 
                readaddr = (readaddr + engine) << 2; 
                alt_write_word(md5_group_data + 2, readaddr + i); // get a,b,c,d digest adress 
                hashed[engine][i] = alt_read_word(md5_group_data + 3); // get the values 
                temp1 = hashed[engine][i] & 0xFF000000; 
                temp2 = hashed[engine][i] & 0x00FF0000; 
                temp3 = hashed[engine][i] & 0x0000FF00; 
                temp4 = hashed[engine][i] & 0x000000FF; 
                final_temp = (temp4 << 24)|(temp3 << 8)| (temp2 >> 8) | (temp1 >> 24); 
                hashed[engine][i] = final_temp; 
                readaddr =0x0; 
        } 

        // Outputting Results and Statistics
        printf("\n---------------------------- ENGINE %d ---------------------------------\n\n", engine + 1); 
        if (engine % 2 ==0){ 
            printf("Digest for '%s' is %08x %08x %08x %08x\nCalculation Time: %0.8f nsec", word, hashed[engine][0], hashed[engine][1], hashed[engine][2], hashed[engine][3], elapsed_time); 
            if (correct_hash[0] == hashed[engine][0] && correct_hash[1] == hashed[engine][1] && correct_hash[2] == hashed[engine][2] && correct_hash[3] == hashed[engine][3]){ 
                    printf("\nThis matches the correct hash value for: %s\n", word); 
                    hash_rate ++; 
            } 
            else{ 
                    printf("\nThis does not match the correct hash value for: %s\n\n", word); 
            } 
        } 
 
        else{ 
            printf("Digest for '%s' is %08x %08x %08x %08x\nCalculation Time: %0.8f nsec", word, hashed[engine][0], hashed[engine][1], hashed[engine][2], hashed[engine][3], elapsed_time); 
            if (correct_hash2[0] == hashed[engine][0] && correct_hash2[1] == hashed[engine][1] && correct_hash2[2] == hashed[engine][2] && correct_hash2[3] == hashed[engine][3]){ 
                    printf("\nThis matches the correct hash value for: %s\n", word2); 
                    hash_rate ++; 
            } 
            else{ 
                    printf("\nThis does not match the correct hash value for: %s\n\n", word2); 
            } 
        } 
    } 
    double rate = (hash_rate / total_elapsed_time) * (1e9); 
    printf("\n\n---------------------------- STATISTICS --------------------------------\n\n"); 
    printf("Number of hashes: %d", hash_rate + (32- hash_rate)); 
    printf("\nTotal correct hashes: %d       hash rate:  %.2f hashes/sec\nTotal incorrect hashes: %d", hash_rate, rate, 32-hash_rate); 
    printf("\nTotal calculation time: %0.8f usec\n\n", (total_elapsed_time / 1000)); 
 
} 
 
 
 
void md5_calc_par(){ 

 
    // Variable Calculation
    int i = 0; 
    int engine = 0; 
    struct timespec start_time, stop_time; 
    double elapsed_time; 
    double total_elapsed_time = 0.0; 
    char *word = "hello"; 
    char *word2 = "SystemOnChip"; 
    double TIME1, TIME2; 
    int hash_rate = 0; 
    uint32_t temp1; 
    uint32_t temp2; 
    uint32_t temp3; 
    uint32_t temp4; 
    uint32_t final_temp; 
 
 
    uint32_t writeaddr = 0x0; 
    uint32_t readaddr = 0x0; 
    uint32_t correct_hash[4] = {0x5d41402a, 0xbc4b2a76, 0xb9719d91, 0x1017c592}; 
    uint32_t correct_hash2[4] = {0xbe7f51b1, 0xae4e17d9, 0xf5b9670f, 0xa5a6fbc4}; 
    uint32_t hashed[32][4]; 
    uint32_t engine_bit = 0x0; 
    uint32_t md5_input[16] = { //hello
            0x6c6c6568, 0x0000806f, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000000, 0x00000000, 
            0x00000000, 0x00000000, 0x00000028, 0x00000000}; 
    uint32_t md5_input2[16] = { //SystemOnChip
                0x74737953, 0x6E4F6D65, 0x70696843, 0x00000080, 
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                0x00000000, 0x00000000, 0x00000060, 0x00000000}; 

	alt_write_word(md5_group_control +1, 0xFFFFFFFF); // reset 
    alt_write_word(md5_group_control +1, 0x00000000); 
 
    for (engine = 0; engine < 32; engine ++){ 
        for (i =0; i<16;i++){ 
                readaddr = (readaddr + engine) << 4; 
                if (engine % 2 ==0){ 
                    alt_write_word(md5_group_data, md5_input[i]); // writedata 
                } 
                else{ 
                    alt_write_word(md5_group_data, md5_input2[i]); // writedata 
                } 
                alt_write_word(md5_group_data + 1, readaddr + i); // writeaddr 
                alt_write_word(md5_group_control + 2, 0x1); // wr 
                readaddr = 0x0; 
        } 
    } 

	alt_write_word(md5_group_control + 2, 0x0); // wr 

	clock_gettime(CLOCK_REALTIME, &start_time); 
    TIME1 = start_time.tv_sec * (1000000000) + start_time.tv_nsec; 

	alt_write_word(md5_group_control, 0xFFFFFFFF); 
    alt_write_word(md5_group_control, 0x0); 

	while (!(alt_read_word(md5_group_control + 2) == 0xFFFFFFFF)); // loop till done 
 
    clock_gettime(CLOCK_REALTIME, &stop_time); 
    TIME2 = stop_time.tv_sec * (1000000000) + stop_time.tv_nsec; 
    elapsed_time = TIME2 - TIME1; 
    total_elapsed_time = total_elapsed_time + elapsed_time; 

	for (engine = 0; engine < 32; engine ++){ 
        readaddr =0x0; 
        for (i =0; i<4;i++){ 
                readaddr = (readaddr + engine) << 2; 
                alt_write_word(md5_group_data + 2, readaddr + i); // get a,b,c,d digest adress 
                hashed[engine][i] = alt_read_word(md5_group_data + 3); // get the values 
                temp1 = hashed[engine][i] & 0xFF000000; 
                temp2 = hashed[engine][i] & 0x00FF0000; 
                temp3 = hashed[engine][i] & 0x0000FF00; 
                temp4 = hashed[engine][i] & 0x000000FF; 
                final_temp = (temp4 << 24) | (temp3 << 8) | (temp2 >> 8) | (temp1 >> 24); 
                hashed[engine][i] = final_temp; 
                readaddr =0x0; 
        } 

        // Outputting Results and Statistics
		printf("\n---------------------------- ENGINE %d ---------------------------------\n\n", engine + 1); 
        if (engine % 2 ==0){ 
            printf("Digest for '%s' is %08x %08x %08x %08x\nCalculation Time: %0.8f nsec", word, hashed[engine][0], hashed[engine][1], hashed[engine][2], hashed[engine][3], elapsed_time); 
            if (correct_hash[0] == hashed[engine][0] && correct_hash[1] == hashed[engine][1] && correct_hash[2] == hashed[engine][2] && correct_hash[3] == hashed[engine][3]){ 
                    printf("\nThis matches the correct hash value for: %s\n", word); 
                    hash_rate ++; 
            } 
            else{ 
                    printf("\nThis does not match the correct hash value for: %s\n\n", word); 
            } 
        } 
 
        else{ 
            printf("Digest for '%s' is %08x %08x %08x %08x\nCalculation Time: %0.8f nsec", word2, hashed[engine][0], hashed[engine][1], hashed[engine][2], hashed[engine][3], elapsed_time); 
            if (correct_hash2[0] == hashed[engine][0] && correct_hash2[1] == hashed[engine][1] && correct_hash2[2] == hashed[engine][2] && correct_hash2[3] == hashed[engine][3]){ 
                    printf("\nThis matches the correct hash value for: %s\n", word2); 
                    hash_rate ++; 
            } 
            else{ 
                    printf("\nThis does not match the correct hash value for: %s\n\n", word2); 
            } 
        } 
    } 



    double rate = (hash_rate / total_elapsed_time) * (1e9);
    printf("\n\n---------------------------- STATISTICS --------------------------------\n\n"); 
    printf("Number of hashes: %d", hash_rate + (32- hash_rate)); 
    printf("\nTotal correct hashes: %d       hash rate:  %.2f hashes/sec\nTotal incorrect hashes: %d", hash_rate, rate, 32-hash_rate); 
    printf("\nTotal calculation time: %0.8f usec\n\n", (total_elapsed_time / 1000)); 
 
} 
 
int main(int argc, char **argv){ 
    int fd, i; 
    void *virtual_base;  
    int choose;
     
    //map address space of fpga for software to access here 
    if((fd = open("/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) { 
        printf( "ERROR: could not open \"/dev/mem\"...\n" ); 
        return( 1 ); 
    } 
 
    virtual_base =  mmap( NULL, LW_SIZE, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, LWHPS2FPGA_BASE); 
 
    if( virtual_base == MAP_FAILED ) { 
        printf( "ERROR: mmap() failed...\n" ); 
        close( fd ); 
        return(1); 
    } 
 
    //initialize the addresses 
    md5_group_data = virtual_base + ((uint32_t)(MD5_GROUP_DATA_0_BASE) ); 
    md5_group_control = virtual_base + ((uint32_t)( MD5_GROUP_CONTROL_0_BASE) ); 
 

    // Choosing the calculation method
	printf("\nChoose MD5 Engines to run Sequentially (0), Parallel (1): or softwate solution (2)");
	scanf("%d", &choose);

	printf("\n\n");

	if (choose == 0){
		md5_calc_seq(); 
	}

	else if (choose == 1){
		md5_calc_par(); 
	}

    else if (choose == 2){
		md5_calc_soft(); 
	}

     
 
    // clean up our memory mapping and exit 
    if( munmap( virtual_base, LW_SIZE) != 0 ) { 
        printf( "ERROR: munmap() failed...\n" ); 
        close( fd ); 
        return( 1 ); 
    } 
 
    close( fd ); 
 
     
    return 0; 
 
} 



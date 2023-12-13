#include <stdio.h>  // Standard input and output
#include <errno.h>  // Access to errno and Exxx macros
#include <stdint.h> // Extra fixed-width data types
#include <string.h> // String utilities
#include <err.h>    // Convenience functions for error reporting (non-standard)

int main(int argc, char *argv[]){

  static char const b64_alphabet[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "+/";

  FILE * file_stream ;
  size_t char_written = 0 ;  // to count for wraps at 76 chars written 

  if (argc > 2) {
      fprintf(stderr, "Usage: %s [FILE]\n", argv[0]);
      errx(1, "Too many arguments");
  } else if (argc == 2 && strcmp(argv[1], "-")) {
      file_stream = fopen(argv[1],"r");
      if (file_stream == NULL)
        errx(1,"%s","Error no file found with that name");
  } else {
      file_stream = stdin ;
  }

  while(1){
    uint8_t input_bytes[3] = {0}; 
    size_t n_read = fread(input_bytes,1,3,file_stream);

    if (n_read == 0){
      if (feof(file_stream)){
        if (char_written < 76  && char_written != 0) // if eof and no char written no newline
          putchar('\n'); 
        fclose(file_stream);  // close stream since no more char to read
        break ; 
      }
      if (ferror(file_stream))
        errx(1,"%s","Error in reading file stream"); 
    }

    int alpha_ind[4] = {0} ;
    alpha_ind[0] = input_bytes[0] >> 2 ; 
    alpha_ind[1] = (input_bytes[0]  << 4 | input_bytes[1] >> 4) & 0x3Fu ; 
    alpha_ind[2] = (input_bytes[1]  << 2 | input_bytes[2] >> 6) & 0x3Fu  ; 
    alpha_ind[3] = input_bytes[2] & 0x3Fu ; 

    char output[4] = {0};    
    output[0] = b64_alphabet[alpha_ind[0]];
    output[1] = b64_alphabet[alpha_ind[1]];
    output[2] = b64_alphabet[alpha_ind[2]];
    output[3] = b64_alphabet[alpha_ind[3]];

    if (feof(file_stream) == 1){
      if(n_read == 2)
        output[3] = 61 ;
      if(n_read == 1){
          output[3] = 61 ; 
          output[2] = 61 ;
      }
    }
    char_written += fwrite(output,1,4,stdout);
    if (ferror(file_stream))
        errx(1,"%s","error in writing to file stream"); 
    if (char_written % 76 == 0){
        char_written = 0 ; 
        putchar('\n'); 
    }
  }
}

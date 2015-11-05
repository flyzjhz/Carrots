#ifndef _CT_BASE64_H
#define _CT_BASE64_H


struct base64_decode_context {
        unsigned int i;
        char buf[4];
};


/* Allocate a buffer and store zero terminated base64 encoded data
   from array IN of size INLEN, returning BASE64_LENGTH(INLEN), i.e.,
   the length of the encoded data, excluding the terminating zero.  On
   return, the OUT variable will hold a pointer to newly allocated
   memory that must be deallocated by the caller.  If output string
   length would overflow, 0 is returned and OUT is set to NULL.  If
   memory allocation failed, OUT is set to NULL, and the return value
   indicates length of the requested memory block, i.e.,
   BASE64_LENGTH(inlen) + 1. */
size_t base64_encode_alloc(const char *in, size_t inlen, char **out);





/* Initialize decode-context buffer, CTX.  */
void base64_decode_ctx_init(struct base64_decode_context *ctx);



/* Allocate an output buffer in *OUT, and decode the base64 encoded
   data stored in IN of size INLEN to the *OUT buffer.  On return, the
   size of the decoded data is stored in *OUTLEN.  OUTLEN may be NULL,
   if the caller is not interested in the decoded length.  *OUT may be
   NULL to indicate an out of memory error, in which case *OUTLEN
   contains the size of the memory block needed.  The function returns
   true on successful decoding and memory allocation errors.  (Use the
   *OUT and *OUTLEN parameters to differentiate between successful
   decoding and memory error.)  The function returns false if the
   input was invalid, in which case *OUT is NULL and *OUTLEN is
   undefined. */
bool
base64_decode_alloc_ctx(struct base64_decode_context * ctx,
                        const char *in, size_t inlen, char **out,
                        size_t * outlen);



#endif

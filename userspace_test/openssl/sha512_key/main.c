#include <string.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h> 
#include <openssl/crypto.h> 
 
#define PUBLIC_KEY_PATH 	("/hd1/program/config/ca/client_sunell_public.pem")
#define PRIVATE_KEY_PATH 	("/hd1/program/config/ca/client_sunell_private.pem")
 
void printHash(unsigned char *md, int len)
{
 
	int i = 0;
    for (i = 0; i < len; i++)
	{
		printf("%02x", md[i]);
	}
 
	printf("\n");
}
 
 
/*读取私钥*/
RSA* ReadPrivateKey(char* p_KeyPath)
{	
	FILE *fp = NULL; 
	char szKeyPath[1024];
	RSA  *priRsa = NULL, *pubRsa = NULL, *pOut = NULL;
	
	printf("PrivateKeyPath[%s] \n", p_KeyPath);
 
	/*	打开密钥文件 */
	if(NULL == (fp = fopen(p_KeyPath, "r")))
	{
		printf( "fopen[%s] failed \n", p_KeyPath);
		return NULL;
	}
	/*	获取私密钥 */
	priRsa = PEM_read_RSAPrivateKey(fp, NULL, NULL,NULL);
	if(NULL == priRsa)
	{
		ERR_print_errors_fp(stdout);
		printf( "PEM_read_RSAPrivateKey\n");
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	
	pOut = priRsa;
	return pOut;
}
 
/*读取公匙*/
RSA* ReadPublicKey(char* p_KeyPath)
{	
	FILE *fp = NULL; 
	char szKeyPath[1024];
	RSA  *priRsa = NULL, *pubRsa = NULL, *pOut = NULL;
	
	printf("PublicKeyPath[%s]\n", p_KeyPath);
 
	/*	打开密钥文件 */
	if(NULL == (fp = fopen(p_KeyPath, "r")))
	{
		printf( "fopen[%s] \n", p_KeyPath);
		return NULL;
	}
	/*	获取私密钥 */
	if(NULL == (priRsa = PEM_read_RSA_PUBKEY(fp, NULL, NULL,NULL)))
	{
		printf( "PEM_read_RSAPrivateKey error\n");
		fclose(fp);
		return NULL;
	}
	fclose(fp);
 
	pOut = priRsa;
	return pOut;
}
 
int main()
{
        char *ct = "55dsd421fd4df1x21c1d4sd21sd51s5";
	char *buf;   
	char *buf2;
  	RSA *pubKey;
  	RSA *privKey;
	int len;
 
	buf = malloc(520);
        buf2 = malloc(520);
 
	//对数据进行sha512算法摘要
	SHA512_CTX c;
	unsigned char md[SHA512_DIGEST_LENGTH];
 
	SHA512((unsigned char *)ct, strlen(ct), md);
	printHash(md, SHA512_DIGEST_LENGTH);
 
	/*下面这个方法和上面方法得到结果是一样的*/
	//SHA512_Init(&c);
	//SHA512_Update(&c, ct, strlen(ct));
 
	//SHA512_Final(md, &c);
	//OPENSSL_cleanse(&c, sizeof(c));
 
	//printHash(md, SHA512_DIGEST_LENGTH);
 
        privKey = ReadPrivateKey(PRIVATE_KEY_PATH);
        if (!privKey) 
	{  
		ERR_print_errors_fp (stderr);    
		exit (1);  
	}
 
        pubKey = ReadPublicKey(PUBLIC_KEY_PATH);  
	if(!pubKey)
	{
	   RSA_free(privKey);   
       printf("Error: can't load public key");
	   exit(1);
	}
 
        /*签名:私钥加密*/
	int nRet = RSA_sign(NID_sha512, md, SHA512_DIGEST_LENGTH, buf, &nOutLen, privKey);
	if(nRet != 1)
	{
		printf("RSA_sign err !!! \n");    
		exit(1);  
	}
	printf("RSA_sign len = %d:", nOutLen);
	printHash(buf, nOutLen);
    
	//len = RSA_private_encrypt(SHA512_DIGEST_LENGTH, md, buf,     
        //privKey,RSA_PKCS1_PADDING);
	//if (len != 256)
	//{
	    //printf("Error: ciphertext should match length of key len = %d \n", len);
	    //exit(1);
	//}
	
	//printf("RSA_private_encrypt:");
	//printHash(buf, strlen(buf));
 
	/*公钥解密*/
	//RSA_public_decrypt(len, (const unsigned char*)buf, (unsigned char*)buf2, 
        //pubKey,RSA_PKCS1_PADDING);	
	//printf("RSA_public_decrypt:");
	//printHash(buf2, strlen(buf2));
 
	RSA_free(privKey);
	RSA_free(pubKey);
	free(buf);
	free(buf2);
	
    return 0;
}

